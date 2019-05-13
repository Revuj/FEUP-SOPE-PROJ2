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

} client_t;

static char hexa[15] = {"123456789abcdef"};
void generateRandomSal(char *sal)
{

    for (int i = 1; i <= SALT_LEN; i++)
    {
        strcat(sal, &hexa[rand() % 15]);
    }
}

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
    destroyClient(clientWrapper(NULL));
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
    client->request->value.header.pid = getpid();
    client->nameFifoAnswer = (char *)malloc(sizeof(FIFO_LENGTH));

    return client;
}

int openRequestFifo(client_t *client)
{
    client->fifoRequest = open(SERVER_FIFO_PATH, O_WRONLY);
    if(client->fifoRequest < 0) {
        perror("Request fifo");
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

    cancelAlarm();
    return 0;
}

int createAccountRequest(client_t *client)
{

    /*fill enum*/
    client->request->type = OP_CREATE_ACCOUNT;

    /*finish to fill header*/

    client->request->value.header.account_id = accountID;
    strcpy(client->request->value.header.password, password);
    client->request->value.header.op_delay_ms = delay;
    /*fill union com info da conta a criar*/
    char *token;

    token = strtok(arguments, " ");
    client->request->value.create.account_id = atoi(token);
    token = strtok(NULL, " ");
    client->request->value.create.balance = atoi(token);
    token = strtok(NULL, " ");
    strcpy(client->request->value.create.password, token);

    return 0;
}

int createBalanceRequest(client_t *client)
{

    /*fill enum*/
    client->request->type = OP_BALANCE;

    client->request->value.header.account_id = accountID;
    strcpy(client->request->value.header.password, password);
    client->request->value.header.op_delay_ms = delay;

    return 0;
}

int createTransferRequest(client_t *client)
{

    client->request->type = OP_TRANSFER;

    client->request->value.header.account_id = accountID;
    strcpy(client->request->value.header.password, password);
    client->request->value.header.op_delay_ms = delay;

    /*fill union*/
    char *token;

    token = strtok(arguments, " ");
    client->request->value.transfer.account_id = atoi(token);
    token = strtok(NULL, " ");
    client->request->value.transfer.amount = atoi(token);

    return 0;
}

int createShutDownRequest(client_t *client)
{

    client->request->type = OP_SHUTDOWN;

    client->request->value.header.account_id = accountID;
    strcpy(client->request->value.header.password, password);
    client->request->value.header.op_delay_ms = delay;

    return 0;
}

int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{
    parse_args(argc,argv);

    client_t *client = createClient();

    if(openRequestFifo(client) != 0)
        exit(EXIT_FAILURE);

    if (installAlarm() != 0)
        exit(EXIT_FAILURE);

    switch (operation)
    {
    case OP_CREATE_ACCOUNT:
        createAccountRequest(client);
        break;

    case OP_BALANCE:
        createBalanceRequest(client);
        break;

    case OP_TRANSFER:
        createTransferRequest(client);
        break;

    case OP_SHUTDOWN:
        createShutDownRequest(client);
        break;
    
    default:
        break;
    }

    createReplyFifo(client, USER_FIFO_PATH_PREFIX);

    sendRequest(client);
    
    clientWrapper(client);
    
    alarm(FIFO_TIMEOUT_SECS);

    //openReplyFifo(client);

    //readReply(client);

    //printf("%d %d", client->reply->value.header.ret_code, client->reply->value.balance.balance);

    logRequest(STDOUT_FILENO, client->request->value.create.account_id, client->request);

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