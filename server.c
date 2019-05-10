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
#include<sys/mman.h>

#include "sope.h"
#include "types.h"
#include "constants.h"
#include "log.c"

typedef struct {
    pthread_mutex_t bufferLock;
    pthread_cond_t full;
    pthread_cond_t empty;
} Shared_memory;

typedef struct  {
    int bankOfficesNo;
    int sLogFd;
    int fifoFd;
    bank_account_t bankAccounts[MAX_BANK_ACCOUNTS];
    pthread_t bankOffices[MAX_BANK_OFFICES];
    Shared_memory * shm;
} Server_t;

Shared_memory * initSharedMemory(char * shmName, int shmSize) {
    int shmfd = shm_open(shmName, O_RDWR, 0660);
    if (shmfd < 0) {
        perror("Shared Memory");
        return NULL;
    }

    Shared_memory * shm = mmap(0, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    if (shm == MAP_FAILED) {
        perror("mmap()");
        return NULL;
    }

    return (Shared_memory * ) shm;
}

int openLogText(char * logFileName) {
    int fd = open(logFileName, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
    if (fd == -1) {
        perror("Server Log File");
        return -1;
    }
    return 0;
}

void * runBankOffice(void * arg) {
    while(1) {

    }
}

void createBankOffices(Server_t * server ) {
    for  (int i = 1; i <= server->bankOfficesNo; i++) {
        pthread_create(&(server->bankOffices[i-1]), NULL, runBankOffice, NULL);
        logBankOfficeOpen(server->sLogFd, i, server->bankOffices[i-1]);
    }
}

void closeBankOffices(Server_t * server) {
    for  (int i = 1; i <= server->bankOfficesNo; i++) {
        pthread_join(server->bankOffices[i-1], NULL);
        logBankOfficeClose(server->sLogFd, i, server->bankOffices[i-1]);
    }
}

int closeLogText(Server_t *server) {
    if (close(server->sLogFd) < 0) {
        perror("Server Log File");
        return -1;
    }
    return 0;
}

int createFifo(char * fifoName) {
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
}

int openFifo(char * fifoName) {
    int fd = open(fifoName, O_RDONLY);

    if (fd < 0)
    {
        perror("Opening fifo");
        return -1;
    }

    return fd;
}

void closeServerFifo() {

    if (unlink(SERVER_FIFO_PATH) < 0)
    {
        perror("FIFO");
        exit(2);
    }
}

//função a ser alterada quando tivermos o buffer de contas
void fillReply(tlv_reply_t * reply, tlv_request_t request) {
    reply->type = request.type;
    reply->value.header.account_id = request.value.header.account_id;
    reply->value.header.ret_code = 69;
    reply->value.balance.balance = 500; // a mudar
    reply->value.transfer.balance = 100;
    reply->value.shutdown.active_offices = 34;
}

Server_t * initServer(char * logFileName, char * fifoName, int bankOfficesNo) {
    Server_t * server = (Server_t *)malloc(sizeof(Server_t));

    int logFd = openLogText(logFileName);

    if (logFd == -1) 
        return NULL;

    if (createFifo(fifoName) == -1) 
        return NULL;


    int fifoFd = openFifo(fifoName);

    if (fifoFd == -1)
        return NULL;
    
    createBankOffices(server);

    server->sLogFd = logFd;
    server->fifoFd = fifoFd;
    server->bankOfficesNo = bankOfficesNo;

    bank_account_t admin_account;
    admin_account.account_id = ADMIN_ACCOUNT_ID;
    admin_account.balance = ADMIN_ACCOUNT_BALLANCE;

    logAccountCreation(STDOUT_FILENO, ADMIN_ACCOUNT_ID, &admin_account);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: ./server <number of bank offices> <admin password>");
        return 1;
    }

    Server_t * server = initServer(SERVER_LOGFILE, SERVER_FIFO_PATH, atoi(argv[1]));

   

    tlv_request_t request;
    tlv_reply_t reply;

    int n = 0;

    do
    {
        n = read(server->fifoFd, &request, sizeof(tlv_request_t));
        if (n > 0) {
            fillReply(&reply, request);
            logReply(STDOUT_FILENO, ADMIN_ACCOUNT_ID, &reply);
        }
        sleep(1);     

    } while (1);


    closeBankOffices(server);


    if (closeLogText(server) == -1) {
        exit(765);
    }

    closeServerFifo();

}