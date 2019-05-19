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

#include "user.h"
#include "../constants.h"

static timer_t timerid;

//====================================================================================================================================
void closeUlog(int status,void *arg)
{
    client_t *client = (client_t *)arg;

    close(client->uLogFd);
}
//====================================================================================================================================
void closeRequestFifo(int status,void *arg)
{
    client_t *client = (client_t *)arg;

    close(client->fifoRequest);
}
//====================================================================================================================================
void closeReplyFifo(int status,void *arg)
{
    client_t *client = (client_t *)arg;

    close(client->fifoReply);
}
//====================================================================================================================================
void deleteReplyFifo(int status,void *arg)
{
    client_t *client = (client_t *)arg;

    unlink(client->nameFifoAnswer);
}
//====================================================================================================================================
void destroyClient(int status,void *arg)
{
    client_t *client = (client_t *)arg;

    free(client->request);
    free(client->reply);
    free(client->nameFifoAnswer);
    free(client);
}
//====================================================================================================================================
void fillTimeOutReply(client_t *client) {
    client->reply->value.header.account_id = client->request->value.header.account_id;
    client->reply->value.header.ret_code = RC_SRV_TIMEOUT;
    client->reply->type = client->request->type;
    client->reply->length = sizeof(req_header_t);
}
//====================================================================================================================================
void thread_handler(union sigval sv) {
    client_t *client = sv.sival_ptr;
    fillTimeOutReply(client);
    logReply(client->uLogFd, client->request->value.header.pid, client->reply);
    exit(EXIT_SUCCESS);
}
//====================================================================================================================================
void openLogText(client_t *client)
{
    if((client->uLogFd = open(USER_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU)) < 0) {
        perror("User logfile");
        exit(EXIT_FAILURE);
    }
    on_exit(closeUlog,client);
}
//====================================================================================================================================
void createReplyFifo(client_t *client)
{
    sprintf(client->nameFifoAnswer, "%s%0*d", USER_FIFO_PATH_PREFIX,WIDTH_ID, client->request->value.header.pid);

    if (mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR) < 0)
    {
        if (errno == EEXIST)
        {
            if(unlink(client->nameFifoAnswer) < 0) {
                perror(client->nameFifoAnswer);
                exit(EXIT_FAILURE);
            }
            if(mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR) < 0) {
                perror(client->nameFifoAnswer);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            perror(client->nameFifoAnswer);
            exit(EXIT_FAILURE);
        }
    }
    on_exit(deleteReplyFifo,client);
}
//====================================================================================================================================
client_t *createClient()
{
    client_t *client = (client_t *)malloc(sizeof(client_t));
    client->request = (tlv_request_t *)malloc(sizeof(tlv_request_t));
    client->reply = (tlv_reply_t *)malloc(sizeof(tlv_reply_t));
    client->nameFifoAnswer = (char *)malloc(sizeof(FIFO_LENGTH));

    on_exit(destroyClient,client);

    client->request->value.header.pid=getpid();
    openLogText(client);
    createReplyFifo(client);
        
    return client;
}
//====================================================================================================================================
void fillServerDownReply(client_t * client) {
    client->reply->value.header.account_id = client->request->value.header.account_id;
    client->reply->value.header.ret_code =  RC_SRV_DOWN;
    client->reply->type = client->request->type;
    client->reply->length = sizeof(req_header_t);
}
//====================================================================================================================================
void openRequestFifo(client_t *client)
{
    if((client->fifoRequest = open(SERVER_FIFO_PATH, O_WRONLY)) < 0) {
        fillServerDownReply(client);
        logReply(client->uLogFd, client->request->value.header.pid, client->reply);
        exit(FIFO_ERROR);
    }
}
//====================================================================================================================================
void openReplyFifo(client_t *client)
{
    if ((client->fifoReply = open(client->nameFifoAnswer, O_RDONLY)) < 0)
    {
        perror("Reply fifo");
        exit(FIFO_ERROR);
    }
    on_exit(closeReplyFifo,client);
}
//====================================================================================================================================
void sendRequest(client_t *client)
{
    logRequest(client->uLogFd, client->request->value.header.pid, client->request);

    openRequestFifo(client);

    if (write(client->fifoRequest, client->request, sizeof(op_type_t)+sizeof(uint32_t)+client->request->length) < 0)
    {
        perror("Send request");
        exit(WRITE_ERROR);
    }
}
//====================================================================================================================================
void readReply(client_t *client)
{
    openReplyFifo(client);

    int nrRead;

    while(1){
        if((nrRead = read(client->fifoReply, &client->reply->type, sizeof(op_type_t))) != -1) {
            if((nrRead = read(client->fifoReply, &client->reply->length, sizeof(uint32_t))) != -1) {
                if((nrRead = read(client->fifoReply, &client->reply->value, client->reply->length)) != -1)
                {
                    if (nrRead != 0)
                    {
                        break;
                    }
                }
            }
        }
    }
    
    logReply(client->uLogFd, client->request->value.header.pid, client->reply);
}
//====================================================================================================================================
int checkArgumentsSpacesNo(char *arguments) {
    int i,count = 0;
    for (i = 0;arguments[i] != '\0';i++)
    {
        if (arguments[i] == ' ')
            count++;    
    }
    return count;
}
//====================================================================================================================================
void parseCreateAccountArgs(client_t *client,char *args) {
    char *token;

    token = strtok(args, " ");
    client->request->value.create.account_id = atoi(token);
    if(client->request->value.create.account_id < 1 || client->request->value.create.account_id >= MAX_BANK_ACCOUNTS) {
        fprintf(stderr,"Invalid account id argument - must be >= 0 and <= 4096\n");
        exit(ID_ERROR);
    }

    token = strtok(NULL, " ");
    client->request->value.create.balance = atoi(token);
    if(client->request->value.create.balance < MIN_BALANCE || client->request->value.create.balance > MAX_BALANCE) {
        fprintf(stderr,"Invalid balance argument - must be >= 1 and <= 1000000000\n");
        exit(BALANCE_ERROR);
    }

    token = strtok(NULL, " ");
    strcpy(client->request->value.create.password, token);
    size_t size = strlen(client->request->value.create.password);
    if(size < MIN_PASSWORD_LEN || size > MAX_PASSWORD_LEN) {
        fprintf(stderr,"Invalid password size - must be >= 8 and <= 20\n");
        exit(PASSWORD_ERROR);
    }
}
//====================================================================================================================================
void createAccountRequest(client_t *client, option_t *options)
{
    client->request->type = options->type;
    client->request->value.header.account_id = options->account_id;
    client->request->value.header.op_delay_ms = options->op_delay_ms;
    strcpy(client->request->value.header.password,options->password);

    if(checkArgumentsSpacesNo(options->operation_arguments) != 2) {
        fprintf(stderr,"Invalid number of arguments for this operation\n");
        exit(ARG_NUM_ERROR);
    }

    parseCreateAccountArgs(client,options->operation_arguments);

    client->request->length = sizeof(req_header_t) + sizeof(req_create_account_t);
}
//====================================================================================================================================
void createBalanceRequest(client_t *client, option_t *options)
{
    client->request->type = options->type;
    client->request->value.header.account_id = options->account_id;
    client->request->value.header.op_delay_ms = options->op_delay_ms;
    strcpy(client->request->value.header.password,options->password);

    if(checkArgumentsSpacesNo(options->operation_arguments) != 0) {
        fprintf(stderr,"Invalid number of arguments for this operation\n");
        exit(ARG_NUM_ERROR);
    }

    client->request->length = sizeof(req_header_t);
}
//====================================================================================================================================
void parseTransferArgs(client_t *client,char *args) {
    char *token;

    token = strtok(args, " ");
    client->request->value.transfer.account_id = atoi(token);
    if(client->request->value.transfer.account_id < 1 || client->request->value.transfer.account_id >= MAX_BANK_ACCOUNTS) {
        fprintf(stderr,"Invalid account id argument - must be >= 0 and <= 4096\n");
        exit(ID_ERROR);
    }

    token = strtok(NULL, " ");
    client->request->value.transfer.amount = atoi(token);
    if(client->request->value.transfer.amount < MIN_BALANCE || client->request->value.transfer.amount > MAX_BALANCE) {
        fprintf(stderr,"Invalid amount argument - must be >= 1 and <= 1000000000\n");
        exit(BALANCE_ERROR);
    }
}
//====================================================================================================================================
void createTransferRequest(client_t *client, option_t *options)
{
    client->request->type = options->type;
    client->request->value.header.account_id = options->account_id;
    client->request->value.header.op_delay_ms = options->op_delay_ms;
    strcpy(client->request->value.header.password,options->password);

    if(checkArgumentsSpacesNo(options->operation_arguments) != 1) {
        fprintf(stderr,"Invalid number of arguments for this operation\n");
        exit(ARG_NUM_ERROR);
    }

    parseTransferArgs(client,options->operation_arguments);

    client->request->length = sizeof(req_header_t) + sizeof(req_transfer_t);
}
//====================================================================================================================================
void createShutDownRequest(client_t *client, option_t *options)
{
    client->request->type = options->type;
    client->request->value.header.account_id = options->account_id;
    client->request->value.header.op_delay_ms = options->op_delay_ms;
    strcpy(client->request->value.header.password,options->password);

    if(checkArgumentsSpacesNo(options->operation_arguments) != 0) {
        fprintf(stderr,"Invalid number of arguments for this operation\n");
        exit(ARG_NUM_ERROR);
    }

    client->request->length = sizeof(req_header_t);
}
//====================================================================================================================================
int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{
    client_t *client = createClient();

    struct sigevent sev;
    struct itimerspec trigger;

    memset(&sev, 0, sizeof(struct sigevent));
    memset(&trigger, 0, sizeof(struct itimerspec));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &thread_handler;
    sev.sigev_value.sival_ptr = client;

    timer_create(CLOCK_REALTIME, &sev, &timerid);

    trigger.it_value.tv_sec = FIFO_TIMEOUT_SECS;

    option_t *options = init_options();
    
    parse_args(argc,argv,options);
    
    switch (options->type)
    {
    case OP_CREATE_ACCOUNT:
        createAccountRequest(client, options);
        break;

    case OP_BALANCE:
        createBalanceRequest(client, options);
        break;

    case OP_TRANSFER:
        createTransferRequest(client, options);
        break;

    case OP_SHUTDOWN:
        createShutDownRequest(client, options);
        break;
    
    default:  
        fprintf(stderr,"Invalid operation number\n0 - Create account\n1 - Check Balance\n2 - Make a transfer\n3 - Shutdown the server");  
        exit(OP_CODE_ERROR);
        break;
    }

    sendRequest(client);
    
    timer_settime(timerid, 0, &trigger, NULL);

    readReply(client);

    timer_delete(timerid);

    exit(EXIT_SUCCESS);
}