#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "constants.h"
#include "types.h"
#include <time.h>
#include <errno.h>


#include "sope.h"
#include "types.h"
#include "constants.h"
#include "log.c"

#define LENGTH_NAME 20


typedef struct {
    tlv_reply_t * reply;
    tlv_request_t * request;
    int fifoRequest;
    int fifoReply;
    char * nameFifoAnswer;

} client_t;



static char hexa[15] = {"123456789abcdef"};
void generateRandomSal(char *sal)
{

    for (int i = 1; i <= SALT_LEN; i++)
    {
        strcat(sal, &hexa[rand() % 15]);
    }
}

client_t * createClient() {
    client_t * client = (client_t *)malloc(sizeof(client_t));
    client->request = (tlv_request_t *)malloc(sizeof(tlv_request_t));
    client->reply=(tlv_reply_t *)malloc(sizeof(tlv_reply_t));
    client->request->value.header.pid = getpid();


    return client;
}

void openRequestFifo(client_t * client) {
    client->fifoRequest = open(SERVER_FIFO_PATH, O_WRONLY);
    
}

int createReplyFifo(client_t * client) {
    char *name = NULL;
    if (asprintf(&name,USER_FIFO_PATH_PREFIX"%d",  client->request->value.header.pid) < 0) {
        return 1;
    }
    if (mkfifo(name ,S_IRUSR | S_IWUSR) < 0) {
        if (errno == EEXIST) {
            unlink(name);
            mkfifo(name, S_IRUSR | S_IWUSR);
        } 
        else {
            return 2;
        }
    }
    client->nameFifoAnswer = name;
    return 0;
}

void createRequest(client_t client) {
    
}

int openReplyFifo(client_t *client) {
    int fd = open(client->nameFifoAnswer, O_RDONLY);
    if(fd < 0) {
        return 1;
    }

    client->fifoReply = fd;
    return 0;

}

int sendRequest(client_t *client) {
    if (write(client->fifoRequest,client->request,sizeof( tlv_request_t)) < 0 ) {
        return 1;
    }

    return 0;
}

int readReply(client_t * client) {
    int nrRead;

    while((nrRead = read(client->fifoReply,client->reply,sizeof(tlv_request_t) )) == -1) {
        if (nrRead!=0) {
            break;
        }
    }
    return 0;
}

int destroyClient(client_t *client) {
    close(client->fifoRequest);
    free(client->request);

    close(client->fifoReply);

    if(unlink(client->nameFifoAnswer)== -1) {
        return 1;
    }

    free(client->reply);
    free(client->nameFifoAnswer);
    return 0;
}



int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{

    client_t * client = createClient();

    openRequestFifo(client);

    if(client->fifoRequest < 0) {
        return 1;
    }
     


    if (strcmp(argv[1], "0") == 0)
    {
    }

    tlv_request_t * request=  client->request;
    request->type = atoi(argv[4]);

    //fill header
    request->value.header.account_id = atoi(argv[1]);
    strcpy(request->value.header.password, argv[2]);
    request->value.header.op_delay_ms = atoi(argv[3]);

    uint32_t id;

    switch (request->type)
    {
    case OP_CREATE_ACCOUNT:
        id = atoi(strtok(argv[5], " "));
        request->value.create.account_id = id;
        uint32_t balance = atoi(strtok(argv[5], " "));
        request->value.create.balance = balance;
        strcpy(request->value.create.password, argv[5]);
        //request.length = 13;
        break;
    case OP_BALANCE:
        break;
    case OP_TRANSFER:
        id = atoi(strtok(argv[5], " "));
        request->value.transfer.account_id = id;
        request->value.transfer.amount = atoi(argv[5]);
        break;
    case OP_SHUTDOWN:
        break;
    }

    sendRequest(client);
    logRequest(STDOUT_FILENO, id,request);
    destroyClient(client);
    //close(fd);
    // srand(time(NULL));
    // pid_t pid = getpid();
    // char fifoName[20] = USER_FIFO_PATH_PREFIX;
    // bank_account_t account;

    // if (strcmp(argv[1],"0") == 0)
    // {

    //     account.account_id = atoi(argv[1]);
    //     account.balance = atoi(argv[2]);
    // }

    // if (strcat(fifoName, atoi(pid)) == NULL)
    // {
    //     return 1;
    // }
    // if (mkfifo(fifoName, 0444) < 0)
    // {
    //     return 1;
    // }
}