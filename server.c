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

#include "sope.h"
#include "types.h"
#include "constants.h"
#include "log.c"

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

typedef struct  {
    int bankOfficesNo;
    int  sLogFd;
    bank_account_t bankAccounts[MAX_BANK_ACCOUNTS];
    pthread_t bankOffices[MAX_BANK_OFFICES];

} Server_t;


int openLogText(Server_t * server) {
    server->sLogFd = open(SERVER_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
    if (server->sLogFd == -1) {
        perror("Server Log File");
        return -1;
    }
    return 0;
}

void createBankOffices(Server_t * server ) {
    for  (int i = 1; i <= server->bankOfficesNo; i++) {
        pthread_create(&(server->bankOffices[i-1]), NULL, NULL, NULL);
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

int createFifo() {
    if (mkfifo(SERVER_FIFO_PATH, S_IRUSR | S_IWUSR))
    {
        if (errno == EEXIST)
        {

            unlink(SERVER_FIFO_PATH);
            mkfifo(SERVER_FIFO_PATH, S_IRUSR | S_IWUSR);
            return 0;
        }
        else
        {
            perror("FIFO");
            return -1;
        }
    }
}

int openServerFifo() {
    int fd = open(SERVER_FIFO_PATH, O_RDONLY);

    if (fd < 0)
    {
        exit(1);
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

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: ./server <number of bank offices> <admin password>");
        return 1;
    }

    Server_t * server = (Server_t *)malloc(sizeof(Server_t));
    
    if (openLogText(server) == -1) {
        exit(3);
    }

    int opcode = 1;

    if (createFifo() == -1) {
        return 1;
    }


    int fd= openServerFifo();
    
    createBankOffices(server);

    int bank_offices_no = atoi(argv[1]);

    bank_account_t admin_account;
    admin_account.account_id = ADMIN_ACCOUNT_ID;
    admin_account.balance = ADMIN_ACCOUNT_BALLANCE;

    logAccountCreation(STDOUT_FILENO, ADMIN_ACCOUNT_ID, &admin_account);

   

    tlv_request_t request;
    tlv_reply_t reply;

    int n = 0;

    do
    {
        n = read(fd, &request, sizeof(tlv_request_t));
        if (n > 0) {
            fillReply(&reply, request);
            logReply(STDOUT_FILENO, ADMIN_ACCOUNT_ID, &reply);
        }
        sleep(1);     

    } while (true);


    closeBankOffices(server);


    if (closeLogText(server) == -1) {
        exit(765);
    }

    closeServerFifo();

}