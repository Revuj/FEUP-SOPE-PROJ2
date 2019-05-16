#define READ 0
#define WRITE 1
#define PIPE_ERROR_RETURN -1
#define FORK_ERROR_RETURN -1

#include "../types.h"
#include <pthread.h>

typedef struct {
    pthread_t tid;
    tlv_request_t * request;
    tlv_reply_t  *reply;
    int fdReply;
    int orderNr;
    bank_account_t **bankAccounts; /*array de contas*/
} BankOffice_t;


typedef struct
{
    bank_account_t ** bankAccounts; /*array de contas*/
    BankOffice_t ** eletronicCounter; /*array de bankoffices*/
    int bankOfficesNo;
    int sLogFd;
    int fifoFd;
    bool up;
} Server_t;

/*
@Brief - Wraps a server so it can be accessed later
@param server - server to wrap or NULL value to have the server previously saved returned
@return - Pointer to the server already saved
*/
Server_t * serverWrapper(Server_t * server);

/*
@Brief - closes the file with the server's log
@param server - server whose log file is associated with
@return - 0 if success or -1 otherwise
*/
int closeLogText(Server_t *server);

/*
@Brief - closes the server's fifo
@return -
*/
void closeServerFifo();

/*
@Brief - deletes/ frees all the bank accounts
@return -
*/
void freeBankAccounts(bank_account_t **bankAccounts);

/*
@Brief - closes the server and frees or closes some resources
@return - 0 in case of success or 1 otherwise
*/
int closeServer(Server_t *server);

/*
@Brief - opens fifo's reply
@param bankoffice - bankOffice with the associated fifo that the reply will be sent
@param prefixName - prefix name of the fifo
@return - 0 in case of success or -1 otherwise
*/
int openFifoReply(BankOffice_t * bankOffice, char * prefixName);

/*
@Brief - closes fifo's reply
@param bankoffice - bankOffice with the associated fifo that the reply will be sent
@return - 0 in case of success or -1 otherwise
*/
int closeFifoReply(BankOffice_t * bankOffice);

/*
@Brief - checks if a bank account exists
@param bankoffice - bankOffice to check if it exists
@param id - id associated with the bank account
@return - true if it exists or false otherwise
*/
bool accountExists(BankOffice_t * bankOffice, int id);

/*
@Brief - generates a random sal
@param string - string to be filled with the sal
@return - 
*/
void generateSalt(char *string);

/*
@Brief - copies the value of a string to the other
@param destiny - string to be filled
@param source - string's content to be coppied
@return - 
*/
void copyString(char * destiny, char * source) ;

/*
@Brief - coprocess to generate the hash
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void generateHash(const char *name, char *fileHash, char *algorithm);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void createBankAccount(Server_t *server, int id, int balance, char *password);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
bool validateLogin(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int checkBalance(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int addBalance(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int subtractBalance(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int transference(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
bool checkAdminOperation(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int validateCreateAccount(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int validateOPBalance(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int validateOPTransfer(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int validateShutDown(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void OPCreateAccount(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void OPBalance(BankOffice_t * bankOffice) ;

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void OPTransfer(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void OPShutDown(BankOffice_t * bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void removeNewLine(char *line);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void validateRequest(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void resetBankOffice(BankOffice_t *bankOffice);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void *runBankOffice(void *arg);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void allocateBankOffice(BankOffice_t * th);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void createBankOffices(Server_t *server);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int createFifo(char *fifoName);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int openFifo(char *fifoName);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int openLogText(char *logFileName);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
Server_t * initServer(char *logFileName, char *fifoName, int bankOfficesNo,char *adminPassword);

/*
@Brief - server
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
void readRequestServer(Server_t *server);

/*
@Brief - calls synch methods
@param name - 
@param fileHash -
@param algorithm -
@return - 
*/
int initSynch(Server_t * server);










