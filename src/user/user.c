#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include "options.h"
#include "../sope.h"
#include "../types.h"
#include "../constants.h"
#include "../log.c"

#define LENGTH_NAME 20
#define FIFO_LENGTH 20

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
    client->nameFifoAnswer = (char *) malloc(sizeof(FIFO_LENGTH));

    return client;
}

void openRequestFifo(client_t * client) {
    client->fifoRequest = open(SERVER_FIFO_PATH, O_WRONLY);
    
}

int createReplyFifo(client_t * client) {
    
    if (sprintf(client->nameFifoAnswer,USER_FIFO_PATH_PREFIX"%d",  client->request->value.header.pid) < 0) {
        return 1;
    }
    if (mkfifo(client->nameFifoAnswer ,S_IRUSR | S_IWUSR) < 0) {
        if (errno == EEXIST) {
            unlink(client->nameFifoAnswer);
            mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR);
        } 
        else {
            return 2;
        }
    }
    
    return 0;
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


int createAccountRequest(client_t * client, int args, char ** argValues) {

    if (args != 5 && args != 6) {
        return 1;
    }
    /*fill enum*/
    client->request->type = OP_CREATE_ACCOUNT;

    /*finish to fill header*/
    int ID = atoi(argValues[1]);

    if(ID!=0) { /*nao e o admin*/
        return 2;
    }
 
    client->request->value.header.account_id = atoi(argValues[1]);
    strcpy(client->request->value.header.password, argValues[2]);
    client->request->value.header.op_delay_ms = atoi(argValues[3]);
    /*fill union com info da conta a criar*/
    char *token;
   
    token = strtok(argValues[5], " ");
    client->request->value.create.account_id =atoi(token);
    token = strtok(NULL," ");
    client->request->value.create.balance = atoi(token);
    token = strtok(NULL," ");
    strcpy(client->request->value.create.password,token);
   
    return 0;
}

int createBalanceRequest(client_t * client, int args, char ** argValues) {
      if (args != 5 && args != 6) {
        return 1;
    }
    /*fill enum*/
    client->request->type = OP_BALANCE;

    client->request->value.header.account_id = atoi(argValues[1]);
    strcpy(client->request->value.header.password, argValues[2]);
    client->request->value.header.op_delay_ms = atoi(argValues[3]);
    return 0;
}



int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{

    client_t * client = createClient();

    openRequestFifo(client);

    if(client->fifoRequest < 0) {
        return 1;
    }
     
    op_type_t typeofRequest = atoi(argv[4]);

    switch(typeofRequest) {
        case OP_CREATE_ACCOUNT:
        if (createAccountRequest(client, argc, argv)!=0) {
            return 1;
        }
        break;

        case OP_BALANCE :
        createBalanceRequest(client,argc,argv);
        break;

    }

    // //fill header
    // request->value.header.account_id = atoi(argv[1]);
    // strcpy(request->value.header.password, argv[2]);
    // request->value.header.op_delay_ms = atoi(argv[3]);

    // uint32_t id;

    // switch (request->type)
    // {
    // case OP_CREATE_ACCOUNT:
    //     id = atoi(strtok(argv[5], " "));
    //     request->value.create.account_id = id;
    //     uint32_t balance = atoi(strtok(argv[5], " "));
    //     request->value.create.balance = balance;
    //     strcpy(request->value.create.password, argv[5]);
    //     //request.length = 13;
    //     break;
    // case OP_BALANCE:
    //     break;
    // case OP_TRANSFER:
    //     id = atoi(strtok(argv[5], " "));
    //     request->value.transfer.account_id = id;
    //     request->value.transfer.amount = atoi(argv[5]);
    //     break;
    // case OP_SHUTDOWN:
    //     break;
    // }

    sendRequest(client);
    logRequest(STDOUT_FILENO, client->request->value.create.account_id,client->request);
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