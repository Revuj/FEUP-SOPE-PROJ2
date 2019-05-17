#include "options.h"
#include "../sope.h"
#include "../types.h"

#define LENGTH_NAME 20
#define FIFO_LENGTH 20

/** @name client_t */
/**@{ struct that holds the informations about a client
 */
typedef struct
{
    tlv_reply_t *reply;         /**< @brief pointer to a client's request*/
    tlv_request_t *request;     /**< @brief pointer to  reply made to the client*/
    int fifoRequest;            /**< @brief file descriptor where the request will be sent*/     
    int fifoReply;              /**< @brief file descriptor where a reply will be received*/
    char *nameFifoAnswer;       /**< @brief name of the fifo specific to the client*/
    int uLogFd;                 /**< @brief file descriptor where the users logs are saved */
} client_t; /** @} end of the client_t struct */

/**
* @Brief - Wraps a client so it can be accessed later
*
* @param client - client to wrap or NULL value to have the client previously saved returned
* @return pointer to the client already saved
*/
client_t *clientWrapper(client_t *client);

/**
* @Brief - Opens a file
*
* @param logFileName - name of the file to be opened
* @return the descriptor of a file or -1 in case of unsuccess
*/
int openLogText(char *logFileName);

/**
* @Brief - fills tlv_reply_t struct in case of timeout
* 
* @param client - client whose reply will be filled
*/
void fillTimeOutReply(client_t *client);

/**
* @Brief - Deletes/Frees a client and frees and closes the resources associated with
*
* @param client - the client to be deleted
* @return 0 in case of success or 1 otherwise
*/
int destroyClient(client_t *client);

/**
* @Brief  - Cancels the alarm
*
*/
void cancelAlarm();

/**
* @Brief  - Installs an alarm
*
* @return 0 in case of success or 1 otherwise
*/
int installAlarm();

/**
* @Brief  - Creates a client
*
* @return client created or NULL in case of an error
*/
client_t *createClient();

/**
* @Brief - fills tlv_reply_t struct in case of a serverDown
*
* @param client - client whose reply will be filled
*/
void fillServerDownReply(client_t * client);

/**
* @Brief - opens the fifo that the client's request will be sent
*
* @param client - client whose request will be sent
* @return 0 if sucess or -1 otherwise
*/
int openRequestFifo(client_t *client);

/**
@return -
* @Brief - creates the fifo that will receive the server's reply
*
* @param client - client whose reply will be received
* @param fifoPrefix - prefix of the fifo's name
* @return 0 if sucess or others values otherwise
*/
int createReplyFifo(client_t *client, char *fifoPrefix);

/**
* @Brief - opens the fifo that will receive the server's reply
*
* @param client - client whose reply will be received
* @return 0 if sucess or 1 otherwise
*/
int openReplyFifo(client_t *client);

/**
* @Brief - writes the request to the server's fifo
*
* @param client - client whose request will be send
* @return 0 if sucess or -1 otherwise
*/
int sendRequest(client_t *client);

/**
* @Brief - reads the reply sent to the client's fifo
*
* @param client - client whose reply will be read
*/
void readReply(client_t *client);

/**
 * @brief Checks number of whitespaces
 * 
 * @param arguments - string that will be check
 * @return number of whitespaces on the string
 */
int checkArgumentsSpacesNo(char *arguments);

/**
* @Brief - creates a create account request
*
* @param client - client whose request will be filled
* @param options - arguments of the request
* @return 0 if sucess or -1 otherwise
*/
int createAccountRequest(client_t *client, option_t *options);

/**
* @Brief - creates a balance request
*
* @param client - client whose request will be filled
* @param options - arguments of the request
* @return 0 if sucess or -1 otherwise
*/
int createBalanceRequest(client_t *client, option_t *options);

/**
* @Brief - creates a transfer request
*
* @param client - client whose request will be filled
* @param options - arguments of the request
* @return 0 if sucess or -1 otherwise
*/
int createTransferRequest(client_t *client, option_t *options);

/**
* @Brief - creates a shutdown request
*
* @param client - client whose request will be filled
* @param options - arguments of the request
* @return 0 if sucess or -1 otherwise
*/
int createShutDownRequest(client_t *client, option_t *options);











