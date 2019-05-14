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

// typedef struct {
//     pthread_mutex_t bufferLock;
//     pthread_cond_t full;
//     pthread_cond_t empty;
// } Shared_memory;

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
} Server_t;

//====================================================================================================================================
int openFifoReply(BankOffice_t * bankOffice, char * prefixName) {
    char replyFifo[64];
    sprintf(replyFifo, "%s%d",prefixName, bankOffice->request->value.header.pid);

    int fd = open(replyFifo,O_WRONLY);

    if (fd == -1 ) {
        return -1;
    } 
    else {
        bankOffice->fdReply=fd;
        return 0;
    }
}
//====================================================================================================================================
int closeFifoReply(BankOffice_t * bankOffice) {
    if (close(bankOffice->fdReply) < 0) {
        perror("Error while closing reply fifo\n");
        return -1;
    }
    return 0;
}
//====================================================================================================================================
int sendReply(BankOffice_t * bankOffice,char *prefixName) {
    if (openFifoReply(bankOffice,prefixName) == -1) {
        perror("Cant open reply fifo\n");
        return -1;
    }

    if (write(bankOffice->fdReply,bankOffice->reply,sizeof(tlv_reply_t))== -1) {
        perror("Cant write to reply fifo\n");
        return -1;
    }

    if (closeFifoReply(bankOffice) == -1) {
        return -1;
    }

    return 0;
    
}
//====================================================================================================================================
bool accountExists(BankOffice_t * bankOffice, int id)
{
    if (!bankOffice->bankAccounts [id])
        return false;
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
bool validateLogin(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.header.account_id;
    char hash[HASH_LEN + 1];
    char hashInput[MAX_PASSWORD_LEN + SALT_LEN + 1];
    strcpy(hashInput, bankOffice->request->value.header.password);
    strcat(hashInput,  bankOffice->bankAccounts[id]->salt);
    generateHash(hashInput, hash, "sha256sum");

    if (accountExists(bankOffice, id) && !strcmp(hash, bankOffice->bankAccounts[id]->hash)){
        return true;
    }
    return false;
}
//====================================================================================================================================
// //a alterar
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
    unsigned int newBalance = bankOffice->bankAccounts[id]->balance + amount;
    if(newBalance > MAX_BALANCE) {
        return -1;
    }
    bankOffice->bankAccounts[id]->balance = newBalance;
    return 0;
}
//====================================================================================================================================
int subtractBalance(BankOffice_t *bankOffice)
{
    int id = bankOffice->request->value.transfer.account_id;
    int amount = bankOffice->request->value.transfer.amount;
    unsigned int newBalance = bankOffice->bankAccounts[id]->balance - amount;
    if (newBalance < MIN_BALANCE) {
        return -1;
    }
    bankOffice->bankAccounts[id]->balance = newBalance;
    bankOffice->reply->value.transfer.balance = newBalance;
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
void validateCreateAccount(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;
 
    if(!checkAdminOperation(bankOffice))
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;

 
    if (accountExists(bankOffice, bankOffice->request->value.create.account_id)) {
        bankOffice->reply->value.header.ret_code = RC_ID_IN_USE;
    }
}
//====================================================================================================================================
void validateOPBalance(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;

    if(checkAdminOperation(bankOffice))
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
}
//====================================================================================================================================
void validateOPTransfer(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;

    if(checkAdminOperation(bankOffice))
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
 
    if (!accountExists(bankOffice, bankOffice->request->value.create.account_id)) {
        bankOffice->reply->value.header.ret_code = RC_ID_NOT_FOUND;
    }
 
    if (bankOffice->request->value.transfer.account_id == bankOffice->request->value.create.account_id) {
        bankOffice->reply->value.header.ret_code = RC_SAME_ID;
    }
}
//====================================================================================================================================
void validateShutDown(BankOffice_t * bankOffice) {
    bankOffice->reply->value.header.ret_code = RC_OK;

    if(!checkAdminOperation(bankOffice))
        bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
}
//====================================================================================================================================
//função a ser alterada quando tivermos o buffer de contas
void fillReply(BankOffice_t *bankOffice)
{
    bankOffice->reply->type = bankOffice->request->type;
    bankOffice->reply->length = 12; // falta a outra length

    switch (bankOffice->reply->type)
    {
    case OP_CREATE_ACCOUNT:
        bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id; 
        break;
    case OP_BALANCE:
        bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;
        bankOffice->reply->value.balance.balance = bankOffice->bankAccounts[bankOffice->reply->value.header.account_id]->balance;
        break;
    case OP_TRANSFER:
        bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;
        transference(bankOffice);
        break;
    case OP_SHUTDOWN:
        //reply->value.shutdown.active_offices= nr threads ativos
        bankOffice->reply->value.header.account_id = bankOffice->request->value.header.account_id;
        chmod(SERVER_FIFO_PATH,0444);
        bankOffice->reply->value.shutdown.active_offices = 1;
        break;
    default:
        break;
    }
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
    if(!validateLogin(bankOffice)) {
        bankOffice->reply->value.header.ret_code = RC_LOGIN_FAIL;
        return;
    }
 
    switch(bankOffice->request->type) {
        case OP_CREATE_ACCOUNT:
            validateCreateAccount(bankOffice);
            break;
        case OP_BALANCE:
            validateOPBalance(bankOffice);
            break;
        case OP_TRANSFER:
            validateOPTransfer(bankOffice);
            break;
        case OP_SHUTDOWN:
            validateShutDown(bankOffice);
            break;
        default:
            break;
    }
}
//====================================================================================================================================
void *runBankOffice(void *arg)
{
    BankOffice_t *bankOffice = (BankOffice_t *)arg;
    while (1)
    {
        // readRequest(requestsQueue, &(bankOffice->request));
        // logRequest(STDOUT_FILENO, bankOffice->orderNr, bankOffice->request);
        // if (validateLogin(bankOffice))
        //     validateRequest(bankOffice);
        // if (bankOffice->reply->value.header.ret_code == RC_OK)
        //     switch (bankOffice->request->type)
        //     {
        //     case OP_CREATE_ACCOUNT:
        //         //crea
        //         break;
        //     default:
        //         break;
        //     }
    }
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
    }
}
//====================================================================================================================================
void freeBankOffice(BankOffice_t * th) {
    free(th->reply);
    free(th->request);
    free(th);
}
//====================================================================================================================================
void closeBankOffices(Server_t *server)
{
    for(int i = 0; i < server->bankOfficesNo; i++) {
        pthread_join(server->eletronicCounter[i]->tid,NULL);
        logBankOfficeClose(server->sLogFd, i+1, server->eletronicCounter[i]->tid);
        freeBankOffice(server->eletronicCounter[i]);
    }
    free(server->eletronicCounter);
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
void destroyBankAccount(bank_account_t * bank_account) {
    free(bank_account);
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
    int fd = open(logFileName, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
    if (fd == -1)
    {
        perror("Server Log File");
        return -1;
    }
    return 0;
}
//====================================================================================================================================
Server_t * initServer(char *logFileName, char *fifoName, int bankOfficesNo,char *adminPassword)
{
    int fifoFd, logFd;
 
    Server_t *server = (Server_t *)malloc(sizeof(Server_t));
 
    server->bankAccounts = (bank_account_t **)malloc(sizeof(bank_account_t *) * MAX_BANK_ACCOUNTS);

    for (int i = 0; i < MAX_BANK_ACCOUNTS; i++)
        server->bankAccounts[i] = NULL;
 
    createBankAccount(server,ADMIN_ACCOUNT_ID,ADMIN_ACCOUNT_BALLANCE,adminPassword);
 
    if (createFifo(fifoName) == -1)
        return NULL;
 
    if ((fifoFd=openFifo(fifoName)) < 0) {
        return NULL;
    }
 
    if ((logFd=openLogText(logFileName)) == -1) {
        return NULL;
    }
    /*bank offices correspondentes as threads*/
    server->eletronicCounter = (BankOffice_t **)malloc(sizeof(BankOffice_t *) * bankOfficesNo);
 
    server->sLogFd = logFd;
    server->fifoFd = fifoFd;
    server->bankOfficesNo = bankOfficesNo;
 
    return server;
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

    for(int i = 0; i < server->bankOfficesNo; i++) {
        free(server->eletronicCounter[i]);
    }

    for(int i = 0; i< MAX_BANK_ACCOUNTS; i++) {
         free(server->bankAccounts);
    }

    free(server);

    closeServerFifo();

    return 0;
}
//====================================================================================================================================
void readRequestServer(Server_t *server) {
    int n;
    while (1)
    {
        tlv_request_t request;
        if ((n = read(server->fifoFd, &request, sizeof(tlv_request_t))) != -1) {
            if (n == 0) {
                close(server->fifoFd);
                openFifo(SERVER_FIFO_PATH);
            }
            else {
                writeRequest(requestsQueue,&request);
            }
        }
    }
}


//====================================================================================================================================
int main(int argc, char **argv)
{
    parse_args(argc,argv);
 
    srand(time(NULL));
 
    Server_t *server = initServer(SERVER_LOGFILE, SERVER_FIFO_PATH, bankOffices, password);

 
    if (server == NULL)
    {
        perror("Server Initialization");
        return 1;
    }
 
    createBankOffices(server);
    requestsQueue = createQueue(REQUESTS_QUEUE_LEN);

    initializeMutex(bankOffices);

    // /*test--a  mudar para serem threads-funcoes ja feitas*/
    BankOffice_t *bk = (BankOffice_t *) malloc(sizeof(BankOffice_t));
    bk->reply = (tlv_reply_t*) malloc(sizeof(tlv_reply_t));
    bk->request=(tlv_request_t * )malloc(sizeof(tlv_request_t));
    bk->bankAccounts = server->bankAccounts;
  
    int n = 0;
 
    do
    {
        n = read(server->fifoFd, bk->request, sizeof(tlv_request_t));
        if (n > 0)
        {
            char hashInput[strlen(bk->request->value.header.password) + HASH_LEN + 1];
            char hashOutput[HASH_LEN + 1];
            strcpy(hashInput, bk->request->value.header.password);
            strcat(hashInput, bk->bankAccounts[0]->salt);
            generateHash(hashInput, hashOutput, "sha256sum"); 
            logRequest(STDOUT_FILENO, bk->orderNr, bk->request);
            validateRequest(bk);
            fillReply(bk);
            sendReply(bk,USER_FIFO_PATH_PREFIX);            
        }
        sleep(1);
 
    } while (1);
 
    free(bk->reply);
    free(bk->request);
    free(bk);
    
    closeBankOffices(server);
    destroyMutex(bankOffices);
    freeQueue(requestsQueue);
    closeServer(server);
}