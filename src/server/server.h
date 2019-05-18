#define READ 0
#define WRITE 1
#define PIPE_ERROR_RETURN -1
#define FORK_ERROR_RETURN -1

#include "../types.h"
#include <pthread.h>

/** @name BankOffice_t */
/**@{ struct that holds the informations about a bank office
 */
typedef struct {
    pthread_t tid; /**< @brief thread id of the bank office thread*/
    tlv_request_t * request; /**< @brief request that the bank office is processing*/
    tlv_reply_t  *reply; /**< @brief reply that the bank office is creating*/
    int fdReply; /**< @brief file descriptor of the fifo where the reply will be placed*/
    int orderNr; /**< @brief identifier of the bank office*/
    bank_account_t **bankAccounts;  /**< @brief array of bank accounts*/
} BankOffice_t;  /** @} end of the BankOffice_t struct */


/** @name Server_t */
/**@{ struct that holds the informations about a server
 */
typedef struct
{
    bank_account_t ** bankAccounts;  /**< @brief array of bank accounts*/
    BankOffice_t ** eletronicCounter;  /**< @brief array of bank offices*/
    int bankOfficesNo; /**< @brief number of threads(bank offices)*/
    int sLogFd; /**< @brief file descriptor of the file where logs will be written*/
    int fifoFd; /**< @brief file descriptor of the fifo to communicate between server and users*/
    bool up; /**< @brief boolean that identifies if the server is up or down*/
} Server_t;  /** @} end of the Server_t struct */

/**
* @Brief Wraps a server so it can be accessed later
*
* @param server - server to wrap or NULL value to have the server previously saved returned
* @return pointer to the server already saved
*/
Server_t * serverWrapper(Server_t * server);

/**
* @Brief Closes the file with the server's log
*
* @param server - server whose log file is associated with
* @return 0 if success or -1 otherwise
*/
int closeLogText(Server_t *server);

/**
* @Brief Closes the server's fifo
*/
void closeServerFifo();

/**
* @Brief Deletes frees all the bank accounts
*/
void freeBankAccounts(bank_account_t **bankAccounts);

/**
* @Brief Closes the server and frees or closes some resources
*
* @return 0 in case of success or 1 otherwise
*/
int closeServer(Server_t *server);

/**
* @Brief Opens fifo's reply
*
* @param bankoffice - bankOffice with the associated fifo that the reply will be sent
* @param prefixName - prefix name of the fifo
* @return 0 in case of success or -1 otherwise
*/
int openFifoReply(BankOffice_t * bankOffice, char * prefixName);

/**
* @Brief Closes fifo's reply
*
* @param bankoffice - bankOffice with the associated fifo that the reply will be sent
* @return 0 in case of success or -1 otherwise
*/
int closeFifoReply(BankOffice_t * bankOffice);

/**
* @Brief Checks if a bank account exists
*
* @param bankoffice - bankOffice to check if it exists
* @param id - id associated with the bank account
* @return true if it exists or false otherwise
*/
bool accountExists(BankOffice_t * bankOffice, int id);

/**
* @Brief Generates a random sal
*
* @param string - string to be filled with the sal
*/
void generateSalt(char *string);

/**
* @Brief Copies the value of a string to the other
*
* @param destiny - string to be filled
* @param source - string's content to be coppied
*/
void copyString(char * destiny, char * source) ;

/**
* @Brief Coprocess to generate the hash
*
* @param input - string that will be hashed
* @param output - result of the input being hashed
* @param algorithm - hash to be applied on the input
*/
void generateHash(const char * input, char * output , char *algorithm);

/**
 * @brief Create a Bank Account object
 * 
 * @param server - server in wich the account will be create
 * @param id  - id of the account to be created
 * @param balance - starting balance of the account
 * @param password  - password of the account
 */
void createBankAccount(Server_t *server, int id, int balance, char *password);

/**
 * @brief Validates Login
 * 
 * @param bankOffice - Bank office that is validating the login
 * @return true if login is valid or false otherwhise
 */
bool validateLogin(BankOffice_t *bankOffice);

/**
 * @brief Cheks balance of an account
 * 
 * @param bankOffice - Bank office that will check the accounts balance
 * @return balance of the account
 */
int checkBalance(BankOffice_t *bankOffice);

/**
 * @brief Adds balance to an account
 * 
 * @param bankOffice - Bank office that will add the balance
 * @return 0 if the operation was a success or -1 otherwhise
 */
int addBalance(BankOffice_t *bankOffice);

/**
 * @brief Removes balance to an account
 * 
 * @param bankOffice - Bank office that will remove the balance
 * @return 0 if the operation was a success or -1 otherwhise
 */
int subtractBalance(BankOffice_t *bankOffice);

/**
 * @brief Transferes money from an account to other
 * 
 * @param bankOffice - Bank office that will transfer the money
 * @return 0 if the operation was a success or -1 otherwhise
 */
int transference(BankOffice_t *bankOffice);

/**
 * @brief Checks if the operation os being made my the administrator
 * 
 * @param bankOffice - Bank office that will check if it is the administrator
 * @return true if it is the administrator requesting and operation or false otherwhise
 */
bool checkAdminOperation(BankOffice_t *bankOffice);

/**
 * @brief Validates the creation of an account
 * 
 * @param bankOffice - Bank Office that validates the operation
 * @return 0 if the validations was sucessful or less than 0 otherwhise
 */
int validateCreateAccount(BankOffice_t * bankOffice);

/**
 * @brief Validates balance checking
 * 
 * @param bankOffice - Bank Office that validates the operation
 * @return 0 if the validations was sucessful or less than 0 otherwhise
 */
int validateOPBalance(BankOffice_t * bankOffice);

/**
 * @brief Validates a transference between accounts
 * 
 * @param bankOffice - Bank Office that validates the operation
 * @return 0 if the validations was sucessful or less than 0 otherwhise
 */
int validateOPTransfer(BankOffice_t * bankOffice);

/**
 * @brief Validates the shutdown of the system
 * 
 * @param bankOffice - Bank Office that validates the operation
 * @return 0 if the validations was sucessful or less than 0 otherwhise
 */
int validateShutDown(BankOffice_t * bankOffice);


/**
 * @brief Creates an accounts
 * 
 * @param bankOffice - Bank office that execute the operation
 */
void OPCreateAccount(BankOffice_t * bankOffice);


/**
 * @brief Checks balance of an accounts
 * 
 * @param bankOffice - Bank office that execute the operation
 */
void OPBalance(BankOffice_t * bankOffice) ;


/**
 * @brief Transfers money between accounts
 * 
 * @param bankOffice - Bank office that execute the operation
 */
void OPTransfer(BankOffice_t * bankOffice);


/**
 * @brief Shuts down the system
 * 
 * @param bankOffice - Bank office that execute the operation
 */
void OPShutDown(BankOffice_t * bankOffice);


/**
 * @brief Validates a request send by an user
 * 
 * @param bankOffice - Bank offices that executes the validation
 */
void validateRequest(BankOffice_t *bankOffice);


/**
 * @brief function that simulates a bank office
 * 
 * @param arg - Bank office to be simulated
 */
void *runBankOffice(void *arg);

/**
 * @brief Allocates space for a bank office
 * 
 * @param bankOffice - Bank Office struct pointer that we want to allocate space for
 */
void allocateBankOffice(BankOffice_t * bankOffice);

/**
 * @brief Initializes the bank offices of a server
 * 
 * @param server - server whose bank offices are to be initialized
 */
void createBankOffices(Server_t *server);

/**
 * @brief Create a Fifo object
 * 
 * @param fifoName - name of the fifo
 * @return 0 on success or -1 otherwhise
 */
int createFifo(char *fifoName);

/**
 * @brief Opens Fifo
 * 
 * @param fifoName - name of the fifo
 * @return 0 on success or -1 otherwhise
 */
int openFifo(char *fifoName);

/**
 * @brief Opens a file where the logs will be saved
 * 
 * @param logFileName - name of the file
 * @return 0 on success or -1 otherwhise
 */
int openLogText(char *logFileName);

/**
 * @brief Initializes the server
 * 
 * @param logFileName - name of the file where logs will be saves
 * @param fifoName - name of the fifo that will be used to communicate between server and user
 * @param bankOfficesNo - number of bank offices (threads) to be run
 * @param adminPassword - administrator's password
 * @return pointer to the server created
 */
Server_t * initServer(char *logFileName, char *fifoName, int bankOfficesNo,char *adminPassword);

/**
 * @brief Reads requests on the fifo buffer
 * 
 * @param server - server that is reading the requests
 */
void readRequestServer(Server_t *server);

/**
 * @brief Initializes synchronization mechanisms
 * 
 * @param server - server that owns the mechanisms
 * @return 0 on success or 1 otherwhise
 */
int initSynch(Server_t * server);










