#include "options.h"
#include "../sope.h"
#include "../types.h"

#include <signal.h>

#define LENGTH_NAME 20
#define FIFO_LENGTH 20

/** @name client_t */
/**@{ struct that holds the informations about a client
 */
typedef struct
{
    tlv_reply_t *reply;         /*pointer to a client's request*/
    tlv_request_t *request;     /*pointer to  reply made to the client*/
    int fifoRequest;            /*file descriptor where the request will be sent*/     
    int fifoReply;              /*file descriptor where a reply will be received*/
    char *nameFifoAnswer;       /*name of the fifo specific to the client*/
    int uLogFd;                 /*file descriptor where the users logs are saved */
} client_t;


/**
 * @Brief - Deletes/Frees a client and frees and closes the resources associated with
 * @param status - the exit status
 * @param arg
 */
void destroyClient(int status,void *arg);

/**
 * @Brief - fills tlv_reply_t struct in case of timeout 
 * @param client - Client whose reply will be filled
 */
void fillTimeOutReply(client_t *client);

/**
 * @Brief - Handles the timer event
 * @param sv - Data passed with notification 
 */
void thread_handler(union sigval sv);

/**
 * @Brief - Opens a file
 * @param logFileName - name of the file to be opened
 * @return - the descriptor of a file or -1 in case of unsuccess
 */
int openLogText(char *logFileName);

/**
 * @Brief  - Creates a client
 * @return - client created
 */
client_t *createClient();

/**
 * @Brief - fills tlv_reply_t struct in case of a serverDown
 * @param client - Client whose reply will be filled
 */
void fillServerDownReply(client_t * client);

/**
 * @Brief - opens the fifo that the client's request will be sent
 * @param client - Client whose request will be sent
 */
void openRequestFifo(client_t *client);

/**
 * @Brief - creates the fifo that will receive the server's reply
 * @param client - Client whose reply will be received
 * @param fifoPrefix - prefix of the fifo's name
 */
void createReplyFifo(client_t *client);

/**
 * @Brief - opens the fifo that will receive the server's reply
 * @param client - Client whose reply will be received
 * @return - 0 if sucess or 1 otherwise
 */
void openReplyFifo(client_t *client);

/**
 * @Brief - writes the request to the server's fifo
 * @param client - Client whose request will be send
 */
void sendRequest(client_t *client);

/**
 * @Brief - reads the reply sent to the client's fifo
 * @param client - Client whose reply will be read
 */
void readReply(client_t *client);

/**
 * @Brief - checks the number of sapces
 * @param arguments - string to check the number of spaces
 */
int checkArgumentsSpacesNo(char *arguments);

/**
 * @Brief - parse the arguments for the create account operation
 * @param client - Client whose request will be filled
 * @param args - Arguments string
 */
void parseCreateAccountArgs(client_t *client,char *args);

/**
 * @Brief - creates a create account request
 * @param client - Client whose request will be filled
 * @param options - struct with command line arguments
 */
void createAccountRequest(client_t *client, option_t *options);

/**
 * @Brief - creates a balance request
 * @param client - Client whose request will be filled
 * @param options - struct with command line arguments
 */
void createBalanceRequest(client_t *client, option_t *options);

/**
 * @Brief - parse the arguments for the transfer operation
 * @param client - Client whose request will be filled
 * @param args - Arguments string
 */
void parseTransferArgs(client_t *client,char *args);

/**
 * @Brief - creates a transfer request
 * @param client - Client whose request will be filled
 * @param options - struct with command line arguments
 */
void createTransferRequest(client_t *client, option_t *options);

/**
 * @Brief - creates a shutdown request
 * @param client - Client whose request will be filled
 * @param options - struct with command line arguments
 */
void createShutDownRequest(client_t *client, option_t *options);