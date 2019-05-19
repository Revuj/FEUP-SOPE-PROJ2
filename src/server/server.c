#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/mman.h>

#include "options.h"
#include "queue.h"
#include "synch.h"
#include "../sope.h"
#include "../constants.h"
#include "server.h"

static queue_t * requestsQueue; /*buffer with the resquests*/
static int serverDown; /*boolean that identifies if the server is up or down*/
static int activeBankOffices; /*active bankoffices counter*/


//====================================================================================================================================
void closeServerFifo()
{
    if (unlink(SERVER_FIFO_PATH) < 0)
    {
        perror("Server fifo");
        exit(EXIT_FAILURE);
    }
}
//====================================================================================================================================
void freeBankAccounts(bank_account_t **bankAccounts) {
    for(int i=0;i<MAX_BANK_ACCOUNTS;i++) {
        if(!bankAccounts[i])
            free(bankAccounts[i]);
    }
    free(bankAccounts);
}
//====================================================================================================================================
void openFifoReply(BankOffice_t * bankOffice, char * prefixName) {
    char replyFifo[64];
    sprintf(replyFifo, "%s%0*d",prefixName,WIDTH_ID, bankOffice->request->value.header.pid);

    printf("fifo reply = %s\n", replyFifo);

    if ((bankOffice->fdReply = open(replyFifo,O_WRONLY)) < 0) {
        bankOffice->reply->value.header.ret_code = RC_USR_DOWN;
    } 
}
//====================================================================================================================================
void closeBankOffices(Server_t *server)
{
    for (int i = 0; i < server->bankOfficesNo; i++) {
        printf("wake up threads\n");
        postNotEmpty(); //"wake up" threads
    }

    for(int i = 0; i < server->bankOfficesNo; i++) {
        printf("calling thread_join %d\n", server->eletronicCounter[i]->orderNr);
        printf("thread id = %ld\n", server->eletronicCounter[i]->tid);
        pthread_join(server->eletronicCounter[i]->tid,NULL);
        printf("Thread joined %d\n", server->eletronicCounter[i]->orderNr);
        logBankOfficeClose(server->sLogFd, i+1, server->eletronicCounter[i]->tid);
        printf("After log\n");
        free(server->eletronicCounter[i]);
    }
    free(server->eletronicCounter);
}
//====================================================================================================================================
void closeServer(int status,void* arg)
{
    printf("boas\n");
    Server_t *server = (Server_t *)arg;

    destroyMutex(MAX_BANK_ACCOUNTS);
    destroySems();
    
    if(server->sLogFd) 
        close(server->sLogFd);
    
    if(server->fifoFd) {
        close(server->fifoFd);
        closeServerFifo();
    }
    
    freeBankAccounts(server->bankAccounts);
    closeBankOffices(server);
        printf("closed banks\n");

    free(server);
}
//====================================================================================================================================
void sendReply(BankOffice_t * bankOffice,char *prefixName) {
    openFifoReply(bankOffice,prefixName);
    
    write(bankOffice->fdReply,bankOffice->reply,sizeof(op_type_t)+sizeof(uint32_t)+bankOffice->reply->length);

    if(bankOffice->fdReply > 0)    
        close(bankOffice->fdReply);
}
//====================================================================================================================================
bool accountExists(BankOffice_t * bankOffice, int id)
{
    if (!bankOffice->bankAccounts[id]) {
        return false;
    }

    return true;
}
//====================================================================================================================================
void generateSalt(char *string)
{
    char salt[SALT_LEN];
    for (int i = 0; i < SALT_LEN; i++)
    {
        sprintf(salt + i, "%x", rand() % 16);
    }
    strcpy(string, salt);
}
//====================================================================================================================================
void copyString(char * destiny, char * source) {
    int c;
    for(c = 0; source[c] != '\0'; c++)
    {
        destiny[c] = source[c];
    }
    destiny[c] = '\0';
}
//====================================================================================================================================
void generateHash(const char * input, char * output, char * algorithm)
{
    int fd1[2];
    int fd2[2];

    pid_t pid;

    if (pipe(fd1) == PIPE_ERROR_RETURN)
    {
        perror("Pipe Error");
        exit(1);
    }

    if (pipe(fd2) == PIPE_ERROR_RETURN)
    {
        perror("Pipe Error");
        exit(1);
    }

    pid = fork();
        
    if (pid > 0)
    {
        close(fd2[WRITE]);
        close(fd1[READ]);
        
        int n = strlen(input);
        write(fd1[WRITE], input, n);
        close(fd1[WRITE]);
        n = read(fd2[READ], output, HASH_LEN);
        char * fileHashCopy = strtok(output, " ");
        fileHashCopy[HASH_LEN] = '\0';
        copyString(output, fileHashCopy);
        close(fd2[READ]);
        
    }
    else if (pid == FORK_ERROR_RETURN)
    {
        perror("Fork error");
        exit(2);
    }
    else
    {
        close(fd2[READ]);
        close(fd1[WRITE]);
        dup2(fd1[READ], STDIN_FILENO);
        dup2(fd2[WRITE], STDOUT_FILENO);
        execlp("sha256sum", "sha256sum", "-", NULL);
        close(fd2[WRITE]);
        close(fd1[READ]);
    }
}
//====================================================================================================================================
void createBankAccount(bank_account_t **bankAccounts, int id, int balance, char *password)
{
    bank_account_t *account = (bank_account_t *)malloc(sizeof(bank_account_t));
    account->account_id = id;
    account->balance = balance;
    generateSalt(account->salt);
    char hashInput[HASH_LEN + MAX_PASSWORD_LEN + 1];
    snprintf(hashInput, HASH_LEN + MAX_PASSWORD_LEN + 1, "%s%s",password, account->salt);
    generateHash(hashInput, account->hash, "sha256sum");
    bankAccounts[id] = account;
}
//====================================================================================================================================
bool validateLogin(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.header.account_id;

    if (!accountExists(bankOffice, id))
        return false;

    char hash[HASH_LEN + 1];
    char hashInput[MAX_PASSWORD_LEN + SALT_LEN + 1];
    strcpy(hashInput, bankOffice->request->value.header.password);
    strcat(hashInput,  bankOffice->bankAccounts[id]->salt);
    generateHash(hashInput, hash, "sha256sum");

    if (!strcmp(hash, bankOffice->bankAccounts[id]->hash)){
        return true;
    }

    return false;
}
//====================================================================================================================================
int checkBalance(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.header.account_id;
    return bankOffice->bankAccounts[id]->balance;
}
//====================================================================================================================================
int addBalance(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.transfer.account_id;
    int amount = bankOffice->request->value.transfer.amount;
    
    int newBalance = bankOffice->bankAccounts[id]->balance + amount;

    if(newBalance > (signed int)MAX_BALANCE) {
        return -1;
    }
    
    bankOffice->bankAccounts[id]->balance = newBalance; 
    return 0;
}
//====================================================================================================================================
int subtractBalance(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.header.account_id;
    int amount = bankOffice->request->value.transfer.amount;

    int newBalance = bankOffice->bankAccounts[id]->balance - amount;

    if (newBalance < (signed int)MIN_BALANCE) {
        return -1;
    }

    bankOffice->bankAccounts[id]->balance = newBalance;  
    return 0;
}
//====================================================================================================================================
void lockAccount(BankOffice_t *bankOffice,int id) {
    logSyncMech(bankOffice->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_ACCOUNT,id);
    bankAccountLock(id);
    
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    logSyncDelay(bankOffice->sLogFd,bankOffice->tid,id,bankOffice->request->value.header.op_delay_ms);
}
//====================================================================================================================================
void lockAccountsTranfer(BankOffice_t *bankOffice) {
    int subtract_id = bankOffice->request->value.header.account_id; 
    int add_id = bankOffice->request->value.transfer.account_id;

    if(subtract_id < add_id) {
        lockAccount(bankOffice,subtract_id);

        lockAccount(bankOffice,add_id);
    }
    else {
        lockAccount(bankOffice,add_id);

        lockAccount(bankOffice,subtract_id);
    }
}
//====================================================================================================================================
void unlockAccountsTranfer(BankOffice_t *bankOffice) {
    int subtract_id = bankOffice->request->value.header.account_id; 
    int add_id = bankOffice->request->value.transfer.account_id;

    bankAccountUnlock(subtract_id);   
    logSyncMech(bankOffice->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_ACCOUNT,subtract_id); 

    bankAccountUnlock(add_id);   
    logSyncMech(bankOffice->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_ACCOUNT,add_id); 
} 
//====================================================================================================================================
int transference(BankOffice_t *bankOffice)
{
    lockAccountsTranfer(bankOffice);

    if (subtractBalance(bankOffice) < 0) {
        bankOffice->reply->value.header.ret_code = RC_NO_FUNDS;
        return -1;
    }

    if (addBalance(bankOffice) < 0) {
        bankOffice->reply->value.header.ret_code = RC_TOO_HIGH;
        return -1;
    }

    unlockAccountsTranfer(bankOffice);

    return 0;
}
//====================================================================================================================================
bool checkAdminOperation(BankOffice_t *bankOffice) {
    return (bankOffice->request->value.header.account_id == 0);
}
//====================================================================================================================================
int validateCreateAccount(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;
 
    if(!checkAdminOperation(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
        return -1;
    }

    if (accountExists(bankOffice, bankOffice->request->value.create.account_id)) {
        bankOffice->reply->value.header.ret_code = RC_ID_IN_USE;
        return -2;
    }

    return 0;
}
//====================================================================================================================================
int validateOPBalance(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;

    if(checkAdminOperation(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
        return -1;
    }

    if (!accountExists(bankOffice, bankOffice->request->value.header.account_id)) {
        bankOffice->reply->value.header.ret_code = RC_OTHER;
        return -2;
    }

    return 0;
}
//====================================================================================================================================
int validateOPTransfer(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;

    if(checkAdminOperation(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
        return -1;
    }
    
    if (!accountExists(bankOffice, bankOffice->request->value.transfer.account_id)) {
        bankOffice->reply->value.header.ret_code = RC_ID_NOT_FOUND;
        return -2;
    }
 
    if (bankOffice->request->value.transfer.account_id == bankOffice->request->value.header.account_id) {
        bankOffice->reply->value.header.ret_code = RC_SAME_ID;
        return -3;
    }

    return 0;
}
//====================================================================================================================================
int validateShutDown(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;

    if(!checkAdminOperation(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
        return -1;
    }

    return 0;
}
//====================================================================================================================================
void OPCreateAccount(BankOffice_t * bankOffice) {
    if (validateCreateAccount(bankOffice) != 0)
        return;

    req_create_account_t create =  bankOffice->request->value.create;

    lockAccount(bankOffice,create.account_id);
    
    createBankAccount(bankOffice->bankAccounts, create.account_id, create.balance, create.password);
    logAccountCreation(bankOffice->sLogFd, create.account_id, bankOffice->bankAccounts[create.account_id]);    
    
    bankAccountUnlock(create.account_id);   
    logSyncMech(bankOffice->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_ACCOUNT,create.account_id);  
    
    bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id; 
}
//====================================================================================================================================
void OPBalance(BankOffice_t * bankOffice) {
    if (validateOPBalance(bankOffice) != 0)
        return; 

    bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;

    lockAccount(bankOffice,bankOffice->request->value.header.account_id);
    
    bankOffice->reply->value.balance.balance = checkBalance(bankOffice);  
    
    bankAccountUnlock(bankOffice->request->value.header.account_id); 
    logSyncMech(bankOffice->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_ACCOUNT,bankOffice->request->value.header.account_id);  
    
    bankOffice->reply->length += sizeof(rep_balance_t);
}
//====================================================================================================================================
void OPTransfer(BankOffice_t * bankOffice) {
    bankOffice->reply->length += sizeof(rep_transfer_t);
    int id = bankOffice->request->value.header.account_id;
    
    bankOffice->reply->value.header.account_id = id;
    bankOffice->reply->value.transfer.balance = bankOffice->request->value.transfer.amount;
    
    if (validateOPTransfer(bankOffice) != 0)
        return;    

    if (transference(bankOffice) == 0) 
        bankOffice->reply->value.transfer.balance = bankOffice->bankAccounts[id]->balance;
}
//====================================================================================================================================
void OPShutDown(BankOffice_t * bankOffice) {
    if (validateShutDown(bankOffice) != 0)
        return;  

    bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;
    
    logDelay(bankOffice->sLogFd,bankOffice->tid,bankOffice->request->value.header.op_delay_ms);
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    chmod(SERVER_FIFO_PATH,0444);
    
    bankOffice->reply->value.shutdown.active_offices = activeBankOffices;
    bankOffice->reply->length += sizeof(rep_shutdown_t);
    serverDown = 1;
}
//====================================================================================================================================
int loginClient(BankOffice_t * bankOffice) {
    
    if(!validateLogin(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_LOGIN_FAIL;
        return -1;
    }
    

    return 0;
}
//====================================================================================================================================
void processRequest(BankOffice_t *bankOffice) {
    activeBankOffices++;
    bankOffice->reply->type = bankOffice->request->type;
    bankOffice->reply->length = sizeof(rep_header_t);
    
    if(loginClient(bankOffice) < 0)
        return;

    switch(bankOffice->request->type) {
        case OP_CREATE_ACCOUNT:
            OPCreateAccount(bankOffice);
            break;
        case OP_BALANCE:
            OPBalance(bankOffice);
            break;
        case OP_TRANSFER:
            OPTransfer(bankOffice);
            break;
        case OP_SHUTDOWN:
            OPShutDown(bankOffice);
            break;
        default:
            break;
    }
    activeBankOffices--;
}
//====================================================================================================================================
void *runBankOffice(void *arg)
{
    BankOffice_t *bankOffice = (BankOffice_t *)arg;
 
    while (1) {
        printf("ola1\n");
        logSyncMechSem(bankOffice->sLogFd,bankOffice->orderNr,SYNC_OP_SEM_WAIT,SYNC_ROLE_CONSUMER,0,getvalueNotEmpty());

        waitNotEmpty();
                printf("ola2\n");

        
        if (serverDown && requestsQueue->itemsNo == 0) {
            break;
        }
        
        logSyncMech(bankOffice->sLogFd, bankOffice->orderNr,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_CONSUMER,0);
        readRequest(requestsQueue, &bankOffice->request);
        logSyncMech(bankOffice->sLogFd, bankOffice->orderNr,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.pid);
        logRequest(bankOffice->sLogFd, bankOffice ->orderNr, bankOffice ->request);
        
        postNotFull();
        logSyncMechSem(bankOffice->sLogFd,bankOffice->orderNr,SYNC_OP_SEM_POST,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.pid,getvalueNotFull());
        
        processRequest(bankOffice);
        
        sendReply(bankOffice ,USER_FIFO_PATH_PREFIX);
        logReply(bankOffice->sLogFd, bankOffice ->request->value.header.pid, bankOffice ->reply);
    }

    printf("bye thread\n");
    return NULL;
}

//====================================================================================================================================
void allocateBankOffice(BankOffice_t * bankOffice) {
    bankOffice->reply=(tlv_reply_t *) malloc(sizeof(tlv_reply_t));
    bankOffice->request=(tlv_request_t *)malloc(sizeof(tlv_request_t));
}
//====================================================================================================================================
void createBankOffices(Server_t *server)
{
    for (int i = 0; i < server->bankOfficesNo; i++)
    {
        server->eletronicCounter[i] = (BankOffice_t *)malloc(sizeof(BankOffice_t));
        allocateBankOffice(server->eletronicCounter[i]); 
        
        server->eletronicCounter[i]->orderNr = i + 1;
        server->eletronicCounter[i]->bankAccounts=server->bankAccounts;
        server->eletronicCounter[i]->sLogFd=server->sLogFd;
        
        pthread_create(&(server->eletronicCounter[i]->tid), NULL,runBankOffice, server->eletronicCounter[i]);
        logBankOfficeOpen(server->sLogFd, i+1, server->eletronicCounter[i]->tid);
        printf("create id = %ld\n", server->eletronicCounter[i]->tid);
    }
}
//====================================================================================================================================
void createFifo(char *fifoName)
{
    if (mkfifo(fifoName, S_IRUSR | S_IWUSR) < 0)
    {
        if (errno == EEXIST)
        {
            if(unlink(fifoName) < 0){
                printf("1\n");
                perror("Server fifo");
                exit(EXIT_FAILURE);
            }
            if(mkfifo(fifoName, S_IRUSR | S_IWUSR) < 0) {
                                printf("2\n");

                perror("Server fifo");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
                            printf("3\n");

            perror("Server fifo");
            exit(EXIT_FAILURE);
        }
    }
}
//====================================================================================================================================
int openFifo(char *fifoName)
{
    int fd;

    if ((fd = open(fifoName, O_RDONLY/* | O_NONBLOCK*/)) < 0)
    {
                        printf("4\n");

        perror("Server fifo");
        exit(EXIT_FAILURE);
    }

    return fd;
}
//====================================================================================================================================
int openLogText(char *logFileName)
{
    int fd;
    if ((fd = open(logFileName, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU)) < 0)
    {
        perror("Server log file");
        return -1;
    }
    return fd;
}
//====================================================================================================================================
void createAdminAccount(Server_t *server,char *adminPassword) {
    logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_ACCOUNT,ADMIN_ACCOUNT_ID);
    bankAccountLock(ADMIN_ACCOUNT_ID);
    
    createBankAccount(server->bankAccounts,ADMIN_ACCOUNT_ID,ADMIN_ACCOUNT_BALLANCE,adminPassword);
    logAccountCreation(server->sLogFd,MAIN_THREAD_ID,server->bankAccounts[ADMIN_ACCOUNT_ID]);
    
    bankAccountUnlock(ADMIN_ACCOUNT_ID);
    logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_ACCOUNT,ADMIN_ACCOUNT_ID);
}
//====================================================================================================================================
void initSynch(Server_t * server) {
    if (initializeMutex(MAX_BANK_ACCOUNTS) == 1 )
        exit(EXIT_FAILURE);

    logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_INIT,SYNC_ROLE_PRODUCER,0,server->bankOfficesNo);
    if (initializeSemNotFull(server->bankOfficesNo) == 1)
        exit(EXIT_FAILURE);

    logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_INIT,SYNC_ROLE_PRODUCER,0,0);
    if (initializeSemNotEmpty() == 1)
        exit(EXIT_FAILURE); 
}
//====================================================================================================================================
Server_t * initServer(char *logFileName, char *fifoName, int bankOfficesNo,char *adminPassword)
{ 
    Server_t *server = (Server_t *)malloc(sizeof(Server_t)); 
    server->bankAccounts = (bank_account_t **)malloc(sizeof(bank_account_t *) * MAX_BANK_ACCOUNTS);
    server->eletronicCounter = (BankOffice_t **)malloc(sizeof(BankOffice_t *) * bankOfficesNo);

    for (int i = 0; i < MAX_BANK_ACCOUNTS; i++)
        server->bankAccounts[i] = NULL;

    createFifo(fifoName);

    server->sLogFd = openLogText(logFileName);

    server->bankOfficesNo = bankOfficesNo;
    requestsQueue = createQueue(server->bankOfficesNo);
    initSynch(server);
    createAdminAccount(server,adminPassword);
    logSyncDelay(server->sLogFd, MAIN_THREAD_ID, ADMIN_ACCOUNT_ID, 0);
    createBankOffices(server);

    server->fifoFd = openFifo(fifoName);

    on_exit(closeServer,server);

    return server;
}
//====================================================================================================================================
void processRequestServer(Server_t *server, tlv_request_t *request) {
    logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_WAIT,SYNC_ROLE_PRODUCER,request->value.header.pid,getvalueNotFull());
    waitNotFull();
    
    logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_PRODUCER,request->value.header.pid);
    writeRequest(requestsQueue,request);
    logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_PRODUCER,request->value.header.pid);
    
    postNotEmpty();
    logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_POST,SYNC_ROLE_PRODUCER,request->value.header.pid,getvalueNotEmpty());
}
//====================================================================================================================================
void readRequestServer(Server_t *server) {
    int n;
    tlv_request_t request;
    while (!serverDown)
    {
        if ((n = read(server->fifoFd, &request.type, sizeof(op_type_t))) > 0) {
            if ((n = read(server->fifoFd, &request.length, sizeof(uint32_t))) > 0) {
                if((n = read(server->fifoFd, &request.value, request.length)) > 0) {
                    processRequestServer(server,&request);
                }
            }
        }
    }
    while(requestsQueue->itemsNo) {
        printf("ola111111111\n");
        logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_WAIT,SYNC_ROLE_PRODUCER,request.value.header.pid,getvalueNotFull());
        waitNotFull();

        postNotEmpty();
        logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_POST,SYNC_ROLE_PRODUCER,request.value.header.pid,getvalueNotEmpty());
    }
}
//====================================================================================================================================
int main(int argc, char **argv)
{
    option_t *options = init_options();

    parse_args(argc,argv,options);
 
    srand(time(NULL));
 
    Server_t *server = initServer(SERVER_LOGFILE, SERVER_FIFO_PATH, options->bankOfficesNo, options->password);
    
    readRequestServer(server);

    printf("boas1\n");

    exit(EXIT_SUCCESS);
}