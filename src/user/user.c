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

#define LENGTH_NAME 20
#define FIFO_LENGTH 20

typedef struct
{
    tlv_reply_t *reply;
    tlv_request_t *request;
    int fifoRequest;
    int fifoReply;
    char *nameFifoAnswer;
    int uLogFd;
} client_t;

client_t *clientWrapper(client_t *client)
{
    static client_t *compClient;
    if (client == NULL)
    {
    }
    else
    {
        compClient = client;
    }
    return compClient;
}

int openLogText(char *logFileName)
{
    int fd = open(logFileName, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
    if (fd == -1) 
        return -1;
    
    return fd;
}

void fillTimeOutReply(client_t *client) {
    client->reply->value.header.account_id = client->request->value.header.account_id;
    client->reply->value.header.ret_code = RC_SRV_TIMEOUT;
    client->reply->type = client->request->type;
    client->reply->length = sizeof(rep_header_t);
}

int destroyClient(client_t *client)
{

    close(client->fifoRequest);
    free(client->request);

    close(client->fifoReply);

    if (unlink(client->nameFifoAnswer) == -1)
    {
        return 1;
    }

    free(client->reply);
    free(client->nameFifoAnswer);
    return 0;
}

void alarmHandler(int signo)
{
    client_t * client = clientWrapper(NULL);
    fillTimeOutReply(client);
    logReply(client->uLogFd, client->request->value.header.pid, client->reply);
    destroyClient(client);
    exit(1);
}

void cancelAlarm()
{
    alarm(0);
}

int installAlarm()
{

    struct sigaction action;

    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    action.sa_handler = alarmHandler;

    if (sigaction(SIGALRM, &action, NULL) < 0)
    {
        perror("Sigaction");
        return -1;
    }

    return 0;
}

client_t *createClient()
{
    client_t *client = (client_t *)malloc(sizeof(client_t));
    client->request = (tlv_request_t *)malloc(sizeof(tlv_request_t));
    client->reply = (tlv_reply_t *)malloc(sizeof(tlv_reply_t));
    client->request->value.header.pid=getpid();
    client->nameFifoAnswer = (char *)malloc(sizeof(FIFO_LENGTH));
    client->uLogFd = openLogText(USER_LOGFILE);

    if (client->uLogFd == -1)
        return NULL;
    return client;
}

void fillServerDownReply(client_t * client) {
    client->reply->value.header.account_id = client->request->value.header.account_id;
    client->reply->value.header.ret_code =  RC_SRV_DOWN;
    client->reply->type = client->request->type;
    client->reply->length = sizeof(rep_header_t);
}

int openRequestFifo(client_t *client)
{
    client->fifoRequest = open(SERVER_FIFO_PATH, O_WRONLY);
    if(client->fifoRequest < 0) {
        logRequest(client->uLogFd, client->request->value.header.pid, client->request);
        fillServerDownReply(client);
        logReply(client->uLogFd, client->request->value.header.pid, client->reply);
        return -1;
    }
    return 0;
}

int createReplyFifo(client_t *client, char *fifoPrefix)
{

    if (sprintf(client->nameFifoAnswer, "%s%d", fifoPrefix, client->request->value.header.pid) < 0)
    {
        return 1;
    }
    if (mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR) < 0)
    {
        if (errno == EEXIST)
        {
            unlink(client->nameFifoAnswer);
            mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR);
        }
        else
        {
            return 2;
        }
    }

    return 0;
}

int openReplyFifo(client_t *client)
{
    int fd = open(client->nameFifoAnswer, O_RDONLY);
    if (fd < 0)
    {
        return 1;
    }

    client->fifoReply = fd;
    return 0;
}

int sendRequest(client_t *client)
{
    if (write(client->fifoRequest, client->request, sizeof(tlv_request_t)) < 0)
    {
        perror("Send request");
        return -1;
    }

    logRequest(client->uLogFd, client->request->value.header.pid, client->request);

    return 0;
}

int readReply(client_t *client)
{
    int nrRead;

    while ((nrRead = read(client->fifoReply, client->reply, sizeof(tlv_request_t))) != -1)
    {
        if (nrRead != 0)
        {
            break;
        }
    }
    
    logReply(client->uLogFd, client->request->value.header.pid, client->reply);
    cancelAlarm();
    return 0;
}


int createAccountRequest(client_t *client, char * arguments)
{
    char *token;

    token = strtok(arguments, " ");
    client->request->value.create.account_id = atoi(token);
    token = strtok(NULL, " ");
    client->request->value.create.balance = atoi(token);
    token = strtok(NULL, " ");
    strcpy(client->request->value.create.password, token);
    client->request->length = sizeof(req_header_t) + sizeof(req_create_account_t);

    return 0;
}

int createBalanceRequest(client_t *client, char * arguments)
{

    if (strlen(arguments) != 0) {
        return 1;

    }
    client->request->length = sizeof(req_header_t);

    return 0;
}

int createTransferRequest(client_t *client, char * arguments)
{
    char *token;

    token = strtok(arguments, " ");
    client->request->value.transfer.account_id = atoi(token);
    token = strtok(NULL, " ");
    client->request->value.transfer.amount = atoi(token);

    client->request->length = sizeof(req_header_t) + sizeof(req_transfer_t);

    return 0;
}

int createShutDownRequest(client_t *client, char * arguments)
{
     if (strlen(arguments) != 0) {
        return 1; 

    }

    client->request->length = sizeof(req_header_t)  ;

    return 0;
}

int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{
    client_t *client = createClient();

    if (!client) {
        perror("Could not open user logfile\n");
        exit(1);
    }
    
    parse_args(argc,argv,client->request);

    if (installAlarm() != 0)
        exit(EXIT_FAILURE);
    
    //verificar erros nas funções de input, vAtor
    switch (client->request->type)
    {
    case OP_CREATE_ACCOUNT:
        createAccountRequest(client, argv[5]);
        break;

    case OP_BALANCE:
        createBalanceRequest(client, argv[5]);
        break;

    case OP_TRANSFER:
        createTransferRequest(client, argv[5]);
        break;

    case OP_SHUTDOWN:
        createShutDownRequest(client, argv[5]);
        break;
    
    default:    
        break;
    }

    if(openRequestFifo(client) != 0) 
        exit(EXIT_FAILURE);
    
    sendRequest(client);
    
    clientWrapper(client);
    
    alarm(FIFO_TIMEOUT_SECS);

    createReplyFifo(client, USER_FIFO_PATH_PREFIX);


    openReplyFifo(client);

    readReply(client);

    //printf("%d %d", client->reply->value.header.ret_code, client->reply->value.balance.balance);

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