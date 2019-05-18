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
void destroyClient(int status,void *arg)
{
    client_t *client = (client_t *)arg;

    timer_delete(timerid);

    if(client->uLogFd)
        close(client->uLogFd);

    if(client->fifoReply) {
        close(client->fifoReply);
        unlink(client->nameFifoAnswer);
    }
    
    if(client->fifoRequest)
        close(client->fifoRequest);

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
int openLogText(char *logFileName)
{
    int fd;
    if((fd = open(logFileName, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU)) < 0) {
        perror("User logfile");
        return -1;
    }
    
    return fd;
}
//====================================================================================================================================
client_t *createClient()
{
    client_t *client = (client_t *)malloc(sizeof(client_t));
    client->request = (tlv_request_t *)malloc(sizeof(tlv_request_t));
    client->reply = (tlv_reply_t *)malloc(sizeof(tlv_reply_t));
    client->nameFifoAnswer = (char *)malloc(sizeof(FIFO_LENGTH));

    client->request->value.header.pid=getpid();
    client->uLogFd = openLogText(USER_LOGFILE);

    on_exit(destroyClient,client);
        
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
        exit(EXIT_SUCCESS);
    }
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
}
//====================================================================================================================================
void openReplyFifo(client_t *client)
{
    if ((client->fifoReply = open(client->nameFifoAnswer, O_RDONLY)) < 0)
    {
        perror("Reply fifo");
        exit(EXIT_FAILURE);
    }
}
//====================================================================================================================================
void sendRequest(client_t *client)
{
    logRequest(client->uLogFd, client->request->value.header.pid, client->request);

    openRequestFifo(client);

    if (write(client->fifoRequest, client->request, sizeof(op_type_t)+sizeof(uint32_t)+client->request->length) < 0)
    {
        perror("Send request");
        exit(EXIT_FAILURE);
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
        exit(EXIT_SUCCESS);
    }

    token = strtok(NULL, " ");
    client->request->value.create.balance = atoi(token);
    if(client->request->value.create.balance < MIN_BALANCE || client->request->value.create.balance > MAX_BALANCE) {
        fprintf(stderr,"Invalid balance argument - must be >= 1 and <= 1000000000\n");
        exit(EXIT_SUCCESS);
    }

    token = strtok(NULL, " ");
    strcpy(client->request->value.create.password, token);
    size_t size = strlen(client->request->value.create.password);
    if(size < MIN_PASSWORD_LEN || size > MAX_PASSWORD_LEN) {
        fprintf(stderr,"Invalid password size - must be >= 8 and <= 20\n");
        exit(EXIT_SUCCESS);
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
        exit(EXIT_SUCCESS);
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
        exit(EXIT_SUCCESS);
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
        exit(EXIT_SUCCESS);
    }

    token = strtok(NULL, " ");
    client->request->value.transfer.amount = atoi(token);
    if(client->request->value.transfer.amount < MIN_BALANCE || client->request->value.transfer.amount > MAX_BALANCE) {
        fprintf(stderr,"Invalid amount argument - must be >= 1 and <= 1000000000\n");
        exit(EXIT_SUCCESS);
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
        exit(EXIT_SUCCESS);
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
        exit(EXIT_SUCCESS);
    }

    client->request->length = sizeof(req_header_t);
}
//====================================================================================================================================
int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{
    client_t *client = createClient();

    option_t *options = init_options();
    
    parse_args(argc,argv,options);

    struct sigevent sev;
    struct itimerspec trigger;

    memset(&sev, 0, sizeof(struct sigevent));
    memset(&trigger, 0, sizeof(struct itimerspec));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &thread_handler;
    sev.sigev_value.sival_ptr = client;

    timer_create(CLOCK_REALTIME, &sev, &timerid);

    trigger.it_value.tv_sec = FIFO_TIMEOUT_SECS;
    
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
        exit(EXIT_SUCCESS);
        break;
    }

    createReplyFifo(client);

    sendRequest(client);
    
    timer_settime(timerid, 0, &trigger, NULL);

    readReply(client);

    exit(EXIT_SUCCESS);
}

// //====================================================================================================================================
// client_t *clientWrapper(client_t *client)
// {
//     static client_t *compClient;
//     if (client == NULL)
//     {
//     }
//     else
//     {
//         compClient = client;
//     }
//     return compClient;
// }
// //====================================================================================================================================
// int openLogText(char *logFileName)
// {
//     int fd = open(logFileName, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
//     if (fd == -1) 
//         return -1;
    
//     return fd;
// }
// //====================================================================================================================================
// void fillTimeOutReply(client_t *client) {
//     client->reply->value.header.account_id = client->request->value.header.account_id;
//     client->reply->value.header.ret_code = RC_SRV_TIMEOUT;
//     client->reply->type = client->request->type;
//     client->reply->length = sizeof(rep_header_t);
// }
// //====================================================================================================================================
// int destroyClient(client_t *client)
// {

//     close(client->fifoRequest);
//     free(client->request);

//     close(client->fifoReply);

//     if (unlink(client->nameFifoAnswer) == -1)
//     {
//         return 1;
//     }

//     free(client->reply);
//     free(client->nameFifoAnswer);
//     return 0;
// }
// //====================================================================================================================================
// void alarmHandler(int signo)
// {
//     client_t * client = clientWrapper(NULL);
//     fillTimeOutReply(client);
//     logReply(client->uLogFd, client->request->value.header.pid, client->reply);
//     destroyClient(client);
//     exit(1);
// }
// //====================================================================================================================================
// void cancelAlarm()
// {
//     alarm(0);
// }
// //====================================================================================================================================
// int installAlarm()
// {

//     struct sigaction action;

//     action.sa_flags = 0;
//     sigemptyset(&action.sa_mask);
//     action.sa_handler = alarmHandler;

//     if (sigaction(SIGALRM, &action, NULL) < 0)
//     {
//         perror("Sigaction");
//         return -1;
//     }

//     return 0;
// }
// //====================================================================================================================================
// client_t *createClient()
// {
//     client_t *client = (client_t *)malloc(sizeof(client_t));
//     client->request = (tlv_request_t *)malloc(sizeof(tlv_request_t));
//     client->reply = (tlv_reply_t *)malloc(sizeof(tlv_reply_t));
//     client->request->value.header.pid=getpid();
//     client->nameFifoAnswer = (char *)malloc(sizeof(FIFO_LENGTH));
//     client->uLogFd = openLogText(USER_LOGFILE);

//     if (client->uLogFd == -1)
//         return NULL;
//     return client;
// }
// //====================================================================================================================================
// void fillServerDownReply(client_t * client) {
//     client->reply->value.header.account_id = client->request->value.header.account_id;
//     client->reply->value.header.ret_code =  RC_SRV_DOWN;
//     client->reply->type = client->request->type;
//     client->reply->length = sizeof(rep_header_t);
// }
// //====================================================================================================================================
// int openRequestFifo(client_t *client)
// {
//     client->fifoRequest = open(SERVER_FIFO_PATH, O_WRONLY);
//     if(client->fifoRequest < 0) {
//         logRequest(client->uLogFd, client->request->value.header.pid, client->request);
//         fillServerDownReply(client);
//         logReply(client->uLogFd, client->request->value.header.pid, client->reply);
//         return -1;
//     }
//     return 0;
// }
// //====================================================================================================================================
// int createReplyFifo(client_t *client, char *fifoPrefix)
// {

//     if (sprintf(client->nameFifoAnswer, "%s%d", fifoPrefix, client->request->value.header.pid) < 0)
//     {
//         return 1;
//     }
//     if (mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR) < 0)
//     {
//         if (errno == EEXIST)
//         {
//             unlink(client->nameFifoAnswer);
//             mkfifo(client->nameFifoAnswer, S_IRUSR | S_IWUSR);
//         }
//         else
//         {
//             return 2;
//         }
//     }

//     return 0;
// }
// //====================================================================================================================================
// int openReplyFifo(client_t *client)
// {
//     int fd = open(client->nameFifoAnswer, O_RDONLY);
//     if (fd < 0)
//     {
//         return 1;
//     }

//     client->fifoReply = fd;
//     return 0;
// }
// //====================================================================================================================================
// int sendRequest(client_t *client)
// {
//     printf("Sending Request\n");
//     if (write(client->fifoRequest, client->request, sizeof(op_type_t)+sizeof(uint32_t)+client->request->length) < 0)
//     {
//         perror("Send request");
//         return -1;
//     }

//     logRequest(client->uLogFd, client->request->value.header.pid, client->request);

//     return 0;
// }
// //====================================================================================================================================
// void readReply(client_t *client)
// {
//     int nrRead;

//     while(1){
//         if((nrRead = read(client->fifoReply, &client->reply->type, sizeof(op_type_t))) != -1) {
//             if((nrRead = read(client->fifoReply, &client->reply->length, sizeof(uint32_t))) != -1) {
//                 if((nrRead = read(client->fifoReply, &client->reply->value, client->reply->length)) != -1)
//                 {
//                     if (nrRead != 0)
//                     {
//                         break;
//                     }
//                 }
//             }
//         }
//     }
    
//     logReply(client->uLogFd, client->request->value.header.pid, client->reply);
// }
// //====================================================================================================================================
// int checkArgumentsSpacesNo(char *arguments) {
//     int i,count = 0;
//     for (i = 0;arguments[i] != '\0';i++)
//     {
//         if (arguments[i] == ' ')
//             count++;    
//     }
//     return count;
// }
// //====================================================================================================================================
// int createAccountRequest(client_t *client, option_t *options)
// {
//     client->request->type = options->type;
//     client->request->length = sizeof(req_header_t) + sizeof(req_create_account_t);
//     client->request->value.header.account_id = options->account_id;
//     client->request->value.header.op_delay_ms = options->op_delay_ms;
//     strcpy(client->request->value.header.password,options->password);

//     if(checkArgumentsSpacesNo(options->operation_arguments) != 2) {
//         fprintf(stderr,"Invalid number of arguments for this operation\n");
//         return -1;
//     }

//     char *token;

//     token = strtok(options->operation_arguments, " ");
//     client->request->value.create.account_id = atoi(token);
//     if(client->request->value.create.account_id < 1 || client->request->value.create.account_id >= MAX_BANK_ACCOUNTS) {
//         fprintf(stderr,"Invalid account id argument\n");
//         return -1;
//     }

//     token = strtok(NULL, " ");
//     client->request->value.create.balance = atoi(token);
//     if(client->request->value.create.balance < MIN_BALANCE || client->request->value.create.balance > MAX_BALANCE) {
//         fprintf(stderr,"Invalid balance argument\n");
//         return -1;
//     }

//     token = strtok(NULL, " ");
//     strcpy(client->request->value.create.password, token);
//     size_t size = strlen(client->request->value.create.password);
//     if(size < MIN_PASSWORD_LEN || size > MAX_PASSWORD_LEN) {
//         fprintf(stderr,"Invalid password argument\n");
//         return -1;
//     }

//     return 0;
// }
// //====================================================================================================================================
// int createBalanceRequest(client_t *client, option_t *options)
// {
//     client->request->type = options->type;
//     client->request->length = sizeof(req_header_t);
//     client->request->value.header.account_id = options->account_id;
//     client->request->value.header.op_delay_ms = options->op_delay_ms;
//     strcpy(client->request->value.header.password,options->password);

//     if(checkArgumentsSpacesNo(options->operation_arguments) != 0) {
//         fprintf(stderr,"Invalid number of arguments for this operation\n");
//         return -1;
//     }

//     return 0;
// }
// //====================================================================================================================================
// int createTransferRequest(client_t *client, option_t *options)
// {
//     client->request->type = options->type;
//     client->request->length = sizeof(req_header_t) + sizeof(req_transfer_t);
//     client->request->value.header.account_id = options->account_id;
//     client->request->value.header.op_delay_ms = options->op_delay_ms;
//     strcpy(client->request->value.header.password,options->password);

//     if(checkArgumentsSpacesNo(options->operation_arguments) != 1) {
//         fprintf(stderr,"Invalid number of arguments for this operation\n");
//         return -1;
//     }

//     char *token;

//     token = strtok(options->operation_arguments, " ");
//     client->request->value.transfer.account_id = atoi(token);
//     if(client->request->value.transfer.account_id < 1 || client->request->value.transfer.account_id >= MAX_BANK_ACCOUNTS) {
//         fprintf(stderr,"Invalid account id argument\n");
//         return -1;
//     }

//     token = strtok(NULL, " ");
//     client->request->value.transfer.amount = atoi(token);
//     if(client->request->value.transfer.amount < MIN_BALANCE || client->request->value.transfer.amount > MAX_BALANCE) {
//         fprintf(stderr,"Invalid amount argument\n");
//         return -1;
//     }

//     return 0;
// }
// //====================================================================================================================================
// int createShutDownRequest(client_t *client, option_t *options)
// {
//     client->request->type = options->type;
//     client->request->length = sizeof(req_header_t);
//     client->request->value.header.account_id = options->account_id;
//     client->request->value.header.op_delay_ms = options->op_delay_ms;
//     strcpy(client->request->value.header.password,options->password);

//     if(checkArgumentsSpacesNo(options->operation_arguments) != 0) {
//         fprintf(stderr,"Invalid number of arguments for this operation\n");
//         return -1;
//     }

//     return 0;
// }
// //====================================================================================================================================
// int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
// {
//     client_t *client = createClient();

//     if (!client) {
//         perror("Could not open user logfile\n");
//         exit(1);
//     }

//     option_t *options = init_options();
    
//     parse_args(argc,argv,options);

//     if (installAlarm() != 0)
//         exit(EXIT_FAILURE);
    
//     //verificar erros nas funções de input, vAtor
//     switch (options->type)
//     {
//     case OP_CREATE_ACCOUNT:
//         if(createAccountRequest(client, options) < 0)
//             exit(EXIT_FAILURE);
//         break;

//     case OP_BALANCE:
//         if(createBalanceRequest(client, options) < 0)
//             exit(EXIT_FAILURE);
//         break;

//     case OP_TRANSFER:
//         if(createTransferRequest(client, options) < 0)
//             exit(EXIT_FAILURE);
//         break;

//     case OP_SHUTDOWN:
//         if(createShutDownRequest(client, options) < 0)
//             exit(EXIT_FAILURE);
//         break;
    
//     default:  
//         fprintf(stderr,"Invalid operation number\n");  
//         break;
//     }

//     if(openRequestFifo(client) != 0) 
//         exit(EXIT_FAILURE);
    
//     sendRequest(client);
    
//     clientWrapper(client);
    
//     alarm(FIFO_TIMEOUT_SECS);

//     createReplyFifo(client, USER_FIFO_PATH_PREFIX);

//     openReplyFifo(client);

//     readReply(client);

//     free_options(options);

//     destroyClient(client);
// }