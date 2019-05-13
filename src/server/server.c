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

typedef struct {
    pthread_t tid;
    tlv_request_t * request;
    tlv_reply_t  *reply;
    int fdReply;
    int orderNr;
    queue_t *requestsQueue;
    bank_account_t **bankAccounts; /*array de contas*/
} BankOffice_t;


typedef struct
{
    bank_account_t **bankAccounts; /*array de contas*/
    BankOffice_t ** eletronicCounter; /*array de bankoffices*/
    int bankOfficesNo;
    int sLogFd;
    int fifoFd;
    queue_t *requestsQueue;
} Server_t;

//====================================================================================================================================
int openFifoReply(BankOffice_t * bankOffice, char * prefixName) {
    sprintf("%s%d",prefixName, bankOffice->request->value.header.pid);
    int fd = open(prefixName,O_WRONLY);

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
//função a ser alterada quando tivermos o buffer de contas
void fillReply(tlv_reply_t *reply, tlv_request_t *request)
{
    reply->type = request->type;
    reply->length = 12; // falta a outra length

    switch (reply->type)
    {
    case OP_CREATE_ACCOUNT:
        reply->value.header.account_id = request->value.create.account_id;
        reply->value.balance.balance = request->value.create.balance;
        break;
    case OP_BALANCE:
        reply->value.header.account_id = request->value.create.account_id;
        reply->value.balance.balance = request->value.create.balance;
        break;
    case OP_TRANSFER:
        reply->value.header.account_id = request->value.create.account_id;
        reply->value.transfer.balance = request->value.transfer.amount;
        break;
    case OP_SHUTDOWN:
        //reply->value.shutdown.active_offices= nr threads ativos
        chmod(SERVER_FIFO_PATH,0444);
        reply->value.shutdown.active_offices = 1;
        break;
    default:
        break;
    }
    reply->value.header.ret_code = 0; //mudar

} 

void removeNewLine(char *line)
{
    char *pos;
    if ((pos = strchr(line, '\n')) != NULL)
        *pos = '\0';
}

void generateSalt(char *string)
{
    char salt[SALT_LEN];
    for (int i = 0; i < SALT_LEN; i++)
    {
        sprintf(salt + i, "%x", rand() % 16);
    }
    strcpy(string, salt);
}

//a alterar
void generateHash(const char *name, char *fileHash, char *algorithm)
{
    char command[30 + HASH_LEN + MAX_PASSWORD_LEN + 1] = "echo -n ";
    strcat(command, name);
    strcat(command, " | sha256sum");
    FILE *fpin = popen(command, "r");
    fgets(fileHash, HASH_LEN + 1, fpin);

    //será necessário fazer um coprocesso (o exec não funciona por causa do simbolo de pipe: |)

    // int fd[2];
    // pid_t pid;

    // if (pipe(fd) == PIPE_ERROR_RETURN)
    // {
    //     perror("Pipe Error");
    //     exit(1);
    // }

    // pid = fork();

    // if (pid > 0)
    // {
    //     close(fd[WRITE]);
    //     wait(NULL);
    //     char fileInfo[300];
    //     read(fd[READ], fileInfo, 300);
    //     char *fileHashCopy = strtok(fileInfo, " ");
    //     //removeNewLine(fileHashCopy);
    //     strcpy(fileHash, fileHashCopy);
    //     close(fd[READ]);
    // }
    // else if (pid == FORK_ERROR_RETURN)
    // {
    //     perror("Fork error");
    //     exit(2);
    // }
    // else
    // {
    //     close(fd[READ]);
    //     dup2(fd[WRITE], STDOUT_FILENO);
    //     execl("echo", "echo", "-n", "foobar", "|", algorithm, NULL);
    //     close(fd[WRITE]);
    // }
}
//====================================================================================================================================
void validateRequest(BankOffice_t *bankOffice) {
    switch(bankOffice->request->type) {
        case OP_CREATE_ACCOUNT:
            if(bankOffice->request->value.header.account_id != 0) {
                bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
            }
            //accountExists
            break;
        case OP_BALANCE:
            if(bankOffice->request->value.header.account_id == 0) {
                bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
            }
            break;
        case OP_TRANSFER:
            if(bankOffice->request->value.header.account_id == 0) {
                bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
            }
            //accountExists
            if(bankOffice->request->value.header.account_id == bankOffice->request->value.transfer.account_id) {
                bankOffice->reply->value.header.ret_code = RC_SAME_ID;
            }
            //saldoinsuficiente
            break;
        case OP_SHUTDOWN:
            if(bankOffice->request->value.header.account_id != 0) {
                bankOffice->reply->value.header.ret_code = RC_OP_NALLOW;
            }
            break;
        default:
            break;
    }
}
//====================================================================================================================================
void *runBankOffice(void *arg)
{
    BankOffice_t *bankOffice = (BankOffice_t *) arg;
    while (1)
    {
        readRequest(bankOffice->requestsQueue,&(bankOffice->request));
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
        allocateBankOffice(server->eletronicCounter[i]);
        pthread_create(&server->eletronicCounter[i]->tid, NULL,runBankOffice,NULL);
        server->eletronicCounter[i]->requestsQueue = server->requestsQueue;
        logBankOfficeOpen(server->sLogFd, i+1, server->eletronicCounter[i]->tid);
    }
}
//====================================================================================================================================
void freeBankOffice(BankOffice_t * th) {
    free(th->reply);
    free(th->request);
    freeQueue(th->requestsQueue);
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
}
//====================================================================================================================================
void createBankAccount(Server_t *server, int id, int balance, char *password)
{
    bank_account_t *account = (bank_account_t *)malloc(sizeof(bank_account_t));
    account->account_id = id;
    account->balance = balance;
    generateSalt(account->salt);
    char hashInput[HASH_LEN + MAX_PASSWORD_LEN + 1];
    strcpy(hashInput, password);
    strcat(hashInput, account->salt);
    generateHash(hashInput, account->hash, "sha256sum");
    server->bankAccounts[id] = account;
}
//====================================================================================================================================
void destroyBankAccount(bank_account_t * bank_account) {
    free(bank_account);
}
//====================================================================================================================================
bool accountExists(Server_t *server, int id)
{
    if (server->bankAccounts + id == NULL)
        return false;
    return true;
}
//====================================================================================================================================
bool validateLogin(Server_t *server, int id, char *password)
{
    char hash[HASH_LEN + 1];
    char hashInput[MAX_PASSWORD_LEN + SALT_LEN + 1];
    generateHash(hashInput, hash, "sha256sum");

    if (accountExists(server, id) || strcmp(hash, server->bankAccounts[id]->hash))
        return true;
    return false;
}
//====================================================================================================================================
// //a alterar
int checkBalance(Server_t *server, int id)
{
    return server->bankAccounts[id]->balance;
}
//====================================================================================================================================
int addBalance(Server_t *server, int id, int balance)
{
    unsigned int newBalance = server->bankAccounts[id]->balance + balance;
    if(newBalance > MAX_BALANCE) {
        return -1;
    }
    server->bankAccounts[id]->balance = newBalance;
    return 0;
}
//====================================================================================================================================
int subtractBalance(Server_t *server, int id, int balance)
{
    unsigned int newBalance = server->bankAccounts[id]->balance - balance;
    if (newBalance < MIN_BALANCE) {
        return -1;
    }
    server->bankAccounts[id]->balance -= balance;
    return 0;
}
//====================================================================================================================================
int transference(Server_t *server, int senderId, int receiverId, int balance)
{

    if (subtractBalance(server, senderId, balance) < 0)
        return -1;

    addBalance(server, receiverId, balance);
    return 0;
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
Server_t *initServer(char *logFileName, char *fifoName, int bankOfficesNo,char *adminPassword)
{
    int fifoFd, logFd;

    Server_t *server = (Server_t *)malloc(sizeof(Server_t));

    createBankAccount(server,ADMIN_ACCOUNT_ID,ADMIN_ACCOUNT_BALLANCE,adminPassword);

    server->bankAccounts = (bank_account_t **)malloc(sizeof(bank_account_t)* MAX_BANK_ACCOUNTS);


    if (createFifo(fifoName) == -1)
        return NULL;

    if ((fifoFd=openFifo(fifoName)) < 0) {
        return NULL;
    }

    if ((logFd=openLogText(logFileName)) == -1) {
        return NULL;
    }
    /*bank offices correspondentes as threads*/
    server->eletronicCounter = (BankOffice_t **)malloc(sizeof(BankOffice_t) * bankOfficesNo);

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

    free(server->bankAccounts);/*tb e necessario fazer um ciclo com numero de contas*/

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
                
                writeRequest(server->requestsQueue,&request);
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

    //createBankOffices(server);
    /*test--a  mudar para serem threads-funcoes ja feitas*/
    BankOffice_t *bk = (BankOffice_t *) malloc(sizeof(BankOffice_t));
    bk->reply = (tlv_reply_t*) malloc(sizeof(tlv_reply_t));
    bk->request=(tlv_request_t * )malloc(sizeof(tlv_request_t));

    int n = 0;

    do
    {
        n = read(server->fifoFd, bk->request, sizeof(tlv_request_t));
        if (n > 0)
        {
            fillReply(bk->reply, bk->request);
            //sendReply(bk,USER_FIFO_PATH_PREFIX);
            logReply(STDOUT_FILENO, ADMIN_ACCOUNT_ID, bk->reply);
            
        }
        sleep(1);

    } while (1);
    free(bk->reply);
    free(bk->request);
    free(bk);

    closeServer(server);
}