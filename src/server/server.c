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
#include <pthread.h>
#include <sys/mman.h>

#include "options.h"
#include "queue.h"
#include "synch.h"
#include "../sope.h"
#include "../types.h"
#include "../constants.h"

#define READ 0
#define WRITE 1
#define PIPE_ERROR_RETURN -1
#define FORK_ERROR_RETURN -1

pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

static queue_t * requestsQueue;

typedef struct {
    pthread_t tid;
    tlv_request_t * request;
    tlv_reply_t  *reply;
    int fdReply;
    int orderNr;
    bank_account_t **bankAccounts; /*array de contas*/
} BankOffice_t;


typedef struct
{
    bank_account_t ** bankAccounts; /*array de contas*/
    BankOffice_t ** eletronicCounter; /*array de bankoffices*/
    int bankOfficesNo;
    int sLogFd;
    int fifoFd;
    bool up;
} Server_t;

//====================================================================================================================================
Server_t * serverWrapper(Server_t * server)
{
    static Server_t *compServer;
    if (server == NULL)
    {
    }
    else
    {
        compServer = server;
    }

    return compServer;
}
//====================================================================================================================================
int closeLogText(Server_t *server)
{
    if (close(server->sLogFd) < 0)
    {
        perror("Server Log File");
        return -1;
    }
    return 0;
}
//====================================================================================================================================
void closeServerFifo()
{
    if (unlink(SERVER_FIFO_PATH) < 0)
    {
        perror("FIFO");
        exit(2);
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
int closeServer(Server_t *server)
{
    if (close(server->fifoFd) == -1) {
        perror("Couldnt close fifo\n");
        return 1;
    }

    if (closeLogText(server) == -1) {
        perror("Couldnt close log text\n");
        return 1;
    }

    freeBankAccounts(server->bankAccounts);

    free(server);

    closeServerFifo();

    return 0;
}
//====================================================================================================================================
int openFifoReply(BankOffice_t * bankOffice, char * prefixName) {
    char replyFifo[64];
    sprintf(replyFifo, "%s%d",prefixName, bankOffice->request->value.header.pid);

    printf("fifo reply = %s\n", replyFifo);
    int fd = open(replyFifo,O_WRONLY);

    if (fd == -1 ) {
        printf("cant open fifo\n");
        bankOffice->reply->value.header.ret_code = RC_USR_DOWN;
        return -1;
    } 
    else {
        bankOffice->fdReply=fd;
        return 0;
    }
}
//====================================================================================================================================
int closeFifoReply(BankOffice_t * bankOffice) {
    if (close(bankOffice->fdReply) < 0) 
        return -1;

    return 0;
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
int sendReply(BankOffice_t * bankOffice,char *prefixName) {
    if (openFifoReply(bankOffice,prefixName) == -1) 
        return -1;
    
    if (write(bankOffice->fdReply,bankOffice->reply,sizeof(tlv_reply_t))== -1)
        return -1;

    if (closeFifoReply(bankOffice) == -1)
        return -1;

    return 0;
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
void generateHash(const char *name, char *fileHash, char *algorithm)
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
        
        int n = strlen(name);
        write(fd1[WRITE], name, n);
        close(fd1[WRITE]);
        n = read(fd2[READ], fileHash, HASH_LEN);
        char * fileHashCopy = strtok(fileHash, " ");
        fileHashCopy[HASH_LEN] = '\0';
        copyString(fileHash, fileHashCopy);
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
void createBankAccount(Server_t *server, int id, int balance, char *password)
{
    bank_account_t *account = (bank_account_t *)malloc(sizeof(bank_account_t));
    account->account_id = id;
    account->balance = balance;
    generateSalt(account->salt);
    char hashInput[HASH_LEN + MAX_PASSWORD_LEN + 1];
    snprintf(hashInput, HASH_LEN + MAX_PASSWORD_LEN + 1, "%s%s",password, account->salt);
    generateHash(hashInput, account->hash, "sha256sum");
    server->bankAccounts[id] = account;
    logAccountCreation(server->sLogFd, id, account);

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
    Server_t * server = serverWrapper(NULL);

    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_CONSUMER,id);
    bankAccountLock(id);
    logSyncDelay(server->sLogFd,bankOffice->tid,id,bankOffice->request->value.header.op_delay_ms);
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    int newBalance = bankOffice->bankAccounts[id]->balance + amount;

    if(newBalance > (signed int)MAX_BALANCE) {
        return -1;
    }
    bankOffice->bankAccounts[id]->balance = newBalance;
    bankAccountUnlock(id);   
    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,id);  

    return 0;
}
//====================================================================================================================================
int subtractBalance(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.header.account_id;
    int amount = bankOffice->request->value.transfer.amount;
    Server_t * server = serverWrapper(NULL);

    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_CONSUMER,id);
    bankAccountLock(id);   
    logSyncDelay(server->sLogFd,bankOffice->tid,id,bankOffice->request->value.header.op_delay_ms);
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    int newBalance = bankOffice->bankAccounts[id]->balance - amount;

    if (newBalance < (signed int)MIN_BALANCE) {
        return -1;
    }
    bankOffice->bankAccounts[id]->balance = newBalance;
    bankAccountUnlock(id);   
    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,id);  
    
    return 0;
}
//====================================================================================================================================
int transference(BankOffice_t *bankOffice)
{
    if (subtractBalance(bankOffice) < 0) {
        bankOffice->reply->value.header.ret_code = RC_NO_FUNDS;
        return -1;
    }

    if (addBalance(bankOffice) < 0) {
        bankOffice->reply->value.header.ret_code = RC_TOO_HIGH;
        return -1;
    }

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
    Server_t * server = serverWrapper(NULL);

    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);
    bankAccountLock(bankOffice->request->value.header.account_id);
    logSyncDelay(server->sLogFd,bankOffice->tid,bankOffice->request->value.header.account_id,bankOffice->request->value.header.op_delay_ms);
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    createBankAccount(serverWrapper(NULL), create.account_id, create.balance, create.password);    
    bankAccountUnlock(bankOffice->request->value.header.account_id);   
    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);  
    bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id; 
}
//====================================================================================================================================
void OPBalance(BankOffice_t * bankOffice) {
    if (validateOPBalance(bankOffice) != 0)
        return; 

    bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;
    Server_t * server = serverWrapper(NULL);

    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);
    bankAccountLock(bankOffice->request->value.header.account_id);
    logSyncDelay(server->sLogFd,bankOffice->tid,bankOffice->request->value.header.account_id,bankOffice->request->value.header.op_delay_ms);
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    bankOffice->reply->value.balance.balance = checkBalance(bankOffice);  
    bankAccountUnlock(bankOffice->request->value.header.account_id); 
    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);  
    
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

    Server_t * server = serverWrapper(NULL);
    bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;
    logDelay(server->sLogFd,bankOffice->tid,bankOffice->request->value.header.op_delay_ms);
    usleep(bankOffice->request->value.header.op_delay_ms * 1000);
    chmod(SERVER_FIFO_PATH,0444);
    bankOffice->reply->value.shutdown.active_offices = server->bankOfficesNo;
    bankOffice->reply->length += sizeof(rep_shutdown_t);
    server->up = false;
}
//====================================================================================================================================
void removeNewLine(char *line)
{
    char *pos;
    if ((pos = strchr(line, '\n')) != NULL)
        *pos = '\0';
}
//====================================================================================================================================
void validateRequest(BankOffice_t *bankOffice) {
    bankOffice->reply->type = bankOffice->request->type;
    bankOffice->reply->length = sizeof(rep_header_t);
    Server_t * server = serverWrapper(NULL);

    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);
    bankAccountLock(bankOffice->request->value.header.account_id);
    if(!validateLogin(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_LOGIN_FAIL;
        bankAccountUnlock(bankOffice->request->value.header.account_id);
        logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);  
        return;
    }
    bankAccountUnlock(bankOffice->request->value.header.account_id);
    logSyncMech(server->sLogFd,bankOffice->tid,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.account_id);  

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
}
//====================================================================================================================================
void resetBankOffice(BankOffice_t *bankOffice) {
}
//====================================================================================================================================
void *runBankOffice(void *arg)
{
    BankOffice_t *bankOffice = (BankOffice_t *)arg;
    Server_t *server = serverWrapper(NULL);
 
    while (1) {
        waitNotEmpty();
        printf("checking if\n");
        if (!server->up && requestsQueue->itemsNo == 0) {
            break;
        }
        readRequest(requestsQueue, &bankOffice->request);
        logSyncMech(server->sLogFd, bankOffice->orderNr,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.pid);
        logRequest(server->sLogFd, bankOffice ->orderNr, bankOffice ->request);
        postNotFull();
        logSyncMechSem(server->sLogFd,bankOffice->orderNr,SYNC_OP_SEM_POST,SYNC_ROLE_CONSUMER,bankOffice->request->value.header.pid,getvalueNotFull());
        validateRequest(bankOffice);
        sendReply(bankOffice ,USER_FIFO_PATH_PREFIX);
        logReply(server->sLogFd, bankOffice ->request->value.header.pid, bankOffice ->reply);
        resetBankOffice(bankOffice);
    }

    printf("bye thread\n");
    return NULL;
}

//====================================================================================================================================
void allocateBankOffice(BankOffice_t * th) {
    th->reply=(tlv_reply_t *) malloc(sizeof(tlv_reply_t));
    th->request=(tlv_request_t *)malloc(sizeof(tlv_request_t));
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
        pthread_create(&(server->eletronicCounter[i]->tid), NULL,runBankOffice, server->eletronicCounter[i]);
        logBankOfficeOpen(server->sLogFd, i+1, server->eletronicCounter[i]->tid);
        printf("create id = %ld\n", server->eletronicCounter[i]->tid);
    }
}
//====================================================================================================================================
int createFifo(char *fifoName)
{
    if (mkfifo(fifoName, S_IRUSR | S_IWUSR))
    {
        if (errno == EEXIST)
        {

            unlink(fifoName);
            mkfifo(fifoName, S_IRUSR | S_IWUSR);
            return 0;
        }
        else
        {
            perror("FIFO");
            return -1;
        }
    }
    return 0;
}
//====================================================================================================================================
int openFifo(char *fifoName)
{
    int fd = open(fifoName, O_RDONLY | O_NONBLOCK);

    if (fd < 0)
    {
        perror("Opening fifo");
        return -1;
    }

    return fd;
}

//====================================================================================================================================
int openLogText(char *logFileName)
{
    int fd = open(logFileName, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
    if (fd == -1)
    {
        perror("Server Log File");
        return -1;
    }
    return fd;
}
//====================================================================================================================================
Server_t * initServer(char *logFileName, char *fifoName, int bankOfficesNo,char *adminPassword)
{
    int fifoFd, logFd;
 
    Server_t *server = (Server_t *)malloc(sizeof(Server_t));
    server->up = true;
 
    server->bankAccounts = (bank_account_t **)malloc(sizeof(bank_account_t *) * MAX_BANK_ACCOUNTS);

    for (int i = 0; i < MAX_BANK_ACCOUNTS; i++)
        server->bankAccounts[i] = NULL;
 
    if (createFifo(fifoName) == -1)
        return NULL;
 
    if ((fifoFd=openFifo(fifoName)) < 0) {
        return NULL;
    }
 
    if ((logFd=openLogText(logFileName)) == -1) {
        return NULL;
    }

    server->eletronicCounter = (BankOffice_t **)malloc(sizeof(BankOffice_t *) * bankOfficesNo);
 
    server->sLogFd = logFd;
    server->fifoFd = fifoFd;
    server->bankOfficesNo = bankOfficesNo;

    logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_PRODUCER,ADMIN_ACCOUNT_ID);
    bankAccountLock(ADMIN_ACCOUNT_ID);
    createBankAccount(server,ADMIN_ACCOUNT_ID,ADMIN_ACCOUNT_BALLANCE,adminPassword);
    bankAccountUnlock(ADMIN_ACCOUNT_ID);
    logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_PRODUCER,ADMIN_ACCOUNT_ID);

    return server;
}
//====================================================================================================================================
void readRequestServer(Server_t *server) {
    int n;
    while (server->up)
    {
        tlv_request_t request;
        if ((n = read(server->fifoFd, &request, sizeof(tlv_request_t))) != -1) {
            if (n == 0) {
                close(server->fifoFd);
                openFifo(SERVER_FIFO_PATH);
            }
            else {
                logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_WAIT,SYNC_ROLE_PRODUCER,request.value.header.pid,getvalueNotFull());
                waitNotFull();
                logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_LOCK,SYNC_ROLE_PRODUCER,request.value.header.pid);
                writeRequest(requestsQueue,&request);
                logSyncMech(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_MUTEX_UNLOCK,SYNC_ROLE_PRODUCER,request.value.header.pid);
                postNotEmpty();
                logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_POST,SYNC_ROLE_PRODUCER,request.value.header.pid,getvalueNotEmpty());
            }
        }
    }
}
//====================================================================================================================================
int initSynch(Server_t * server) {
    if (initializeMutex(MAX_BANK_ACCOUNTS)== 1 ) {
        return 1;
    }
    logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_INIT,SYNC_ROLE_PRODUCER,0,server->bankOfficesNo);
    if (initializeSemNotFull(server->bankOfficesNo) == 1) {
        return 1;
    } 
    logSyncMechSem(server->sLogFd,MAIN_THREAD_ID,SYNC_OP_SEM_INIT,SYNC_ROLE_PRODUCER,0,0);
    if (initializeSemNotFull(server->bankOfficesNo) == 1) {
        return 1;
    } 
    return 0;
}
//====================================================================================================================================
int main(int argc, char **argv)
{
    option_t *options=init_options();
    parse_args(argc,argv,options);
 
    srand(time(NULL));
 
    Server_t *server = initServer(SERVER_LOGFILE, SERVER_FIFO_PATH, options->bankOfficesNo, options->password);
 
    if (server == NULL)
    {
        perror("Server Initialization");
        exit(1);
    }

    serverWrapper(server);
    
    requestsQueue = createQueue(server->bankOfficesNo);
    
    if (initSynch(server) == 1) {
        perror("Synchronization Mechanisms Initialization");
        exit(2);
    }

    createBankOffices(server);

    readRequestServer(server);

    printf("closing banks\n");
    closeBankOffices(server);
    printf("closed banks\n");
    destroyMutex(MAX_BANK_ACCOUNTS);
    destroySems();
    freeQueue(requestsQueue);
    closeServer(server);
}