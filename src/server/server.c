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


#include "../sope.h"
#include "../types.h"
#include "../constants.h"
#include "options.h"

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

typedef struct
{
    int bankOfficesNo;
    int sLogFd;
    int fifoFd;
    bank_account_t bankAccounts[MAX_BANK_ACCOUNTS];
    pthread_t bankOffices[MAX_BANK_OFFICES];
    bank_account_t *adminAccount;
} Server_t;

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

void *runBankOffice(void *arg)
{
    while (1)
    {
    }
}

void createBankOffices(Server_t *server)
{
    for (int i = 1; i <= server->bankOfficesNo; i++)
    {
        pthread_create(&(server->bankOffices[i - 1]), NULL, runBankOffice, NULL);
        logBankOfficeOpen(server->sLogFd, i, server->bankOffices[i - 1]);
    }
}

void closeBankOffices(Server_t *server)
{
    for (int i = 1; i <= server->bankOfficesNo; i++)
    {
        pthread_join(server->bankOffices[i - 1], NULL);
        logBankOfficeClose(server->sLogFd, i, server->bankOffices[i - 1]);
    }
}

int closeLogText(Server_t *server)
{
    if (close(server->sLogFd) < 0)
    {
        perror("Server Log File");
        return -1;
    }
    return 0;
}

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

void closeServerFifo()
{

    if (unlink(SERVER_FIFO_PATH) < 0)
    {
        perror("FIFO");
        exit(2);
    }
}

//função a ser alterada quando tivermos o buffer de contas
void fillReply(tlv_reply_t *reply, tlv_request_t request)
{
    reply->type = request.type;
    reply->length = 12; // falta a outra length

    switch (reply->type)
    {
    case OP_CREATE_ACCOUNT:
        reply->value.header.account_id = request.value.create.account_id;
        reply->value.balance.balance = request.value.create.balance;
        break;
    case OP_BALANCE:
        reply->value.header.account_id = request.value.create.account_id;
        reply->value.balance.balance = request.value.create.balance;
        break;
    case OP_TRANSFER:
        reply->value.header.account_id = request.value.create.account_id;
        reply->value.transfer.balance = request.value.transfer.amount;
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

//a alterar
bank_account_t *createBankAccount(Server_t *server, int id, int balance, char *password)
{
    bank_account_t *account = malloc(sizeof(bank_account_t));
    account->account_id = id;
    account->balance = balance;
    generateSalt(account->salt);
    char hashInput[HASH_LEN + MAX_PASSWORD_LEN + 1];
    strcpy(hashInput, password);
    strcat(hashInput, account->salt);
    generateHash(hashInput, account->hash, "sha256sum");
    server->bankAccounts[id] = *account;
    logAccountCreation(STDOUT_FILENO, id, account);

    return account;
}

bool accountExists(Server_t *server, int id)
{
    if (server->bankAccounts + id == NULL)
        return false;
    return true;
}

bool validateLogin(Server_t *server, int id, char *password)
{
    char hash[HASH_LEN + 1];
    char hashInput[MAX_PASSWORD_LEN + SALT_LEN + 1];
    generateHash(hashInput, hash, "sha256sum");

    if (accountExists(server, id) || strcmp(hash, server->bankAccounts[id].hash))
        return true;
    return false;
}

//a alterar
int checkBalance(Server_t *server, int id)
{
    return server->bankAccounts[id].balance;
}

void addBalance(Server_t *server, int id, int balance)
{
    server->bankAccounts[id].balance += balance;
}

int subtractBalance(Server_t *server, int id, int balance)
{
    int newBalance = server->bankAccounts[id].balance - balance;
    if (newBalance < 0)
        return -1;
    server->bankAccounts[id].balance -= balance;
    return 0;
}

int transference(Server_t *server, int senderId, int receiverId, int balance)
{

    if (subtractBalance(server, senderId, balance) < 0)
        return -1;

    addBalance(server, receiverId, balance);
    return 0;
}

Server_t *initServer(char *logFileName, char *fifoName, int bankOfficesNo, char *adminPassword)
{
    Server_t *server = (Server_t *)malloc(sizeof(Server_t));

    int logFd = openLogText(logFileName);

    if (logFd == -1)
        return NULL;

    if (createFifo(fifoName) == -1)
        return NULL;

    int fifoFd = openFifo(fifoName);

    if (fifoFd < 0)
        return NULL;

    bank_account_t *admin_account = createBankAccount(server, ADMIN_ACCOUNT_ID, ADMIN_ACCOUNT_BALLANCE, adminPassword);

    server->sLogFd = logFd;
    server->fifoFd = fifoFd;
    server->bankOfficesNo = bankOfficesNo;
    server->adminAccount = admin_account;

    createBankOffices(server);

    free(admin_account);

    return server;
}

void closeServer(Server_t *server)
{
    closeBankOffices(server);

    if (closeLogText(server) == -1)
    {
        exit(765);
    }

    closeServerFifo();
}

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

    tlv_request_t request;
    tlv_reply_t reply;

    int n = 0;

    do
    {
        n = read(server->fifoFd, &request, sizeof(tlv_request_t));
        if (n > 0)
        {
            fillReply(&reply, request);
            logReply(STDOUT_FILENO, ADMIN_ACCOUNT_ID, &reply);
        }
        sleep(1);

    } while (1);

    closeServer(server);
}