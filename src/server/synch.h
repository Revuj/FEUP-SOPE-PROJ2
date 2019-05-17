#ifndef SYNCH_H___
#define SYNCH_H___

/**
 * @brief Intializes a number of mutexes
 * 
 * @param nr - number of the mutexes to be initialized
 * @return 0 on success or 1 otherwhise
 */
int initializeMutex(int nr);

/**
 * @brief Destroys a number of mutexes
 * 
 * @param nr - number of the mutexes to be destroyed
 * @return 0 on success or 1 otherwhise
 */
int destroyMutex(int nr);

/**
 * @brief Locks a certain mutex
 * 
 * @param id - id of the mutex to be locked
 * @return 0 on success or 1 otherwhise
 */
int bankAccountLock(int id) ;

/**
 * @brief Unlocks a certain mutex
 * 
 * @param id - id of the mutex to be unlocked
 * @return 0 on success or 1 otherwhise
 */
int bankAccountUnlock(int id);

/**
 * @brief Initializes the notFull semaphore
 * 
 * @param semsNo - initial value of the semaphore
 * @return 0 on success or 1 otherwhise
 */
int initializeSemNotFull(int semsNo);

/**
 * @brief Initializes the notEmpty semaphore
 * 
 * @param semsNo - initial value of the semaphore
 * @return 0 on success or 1 otherwhise
 */
int initializeSemNotEmpty(int semsNo);

/**
 * @brief Decrements the notFull semaphore
 * 
 * @return 0 on success or 1 otherwhise
 */
int waitNotFull();

/**
 * @brief Decrements the notEmpty semaphore
 * 
 * @return 0 on success or 1 otherwhise
 */
int waitNotEmpty();

/**
 * @brief Increments the notFull semaphore
 * 
 * @return 0 on success or 1 otherwhise
 */
int postNotFull();

/**
 * @brief Increments the notEmpty semaphore
 * 
 * @return 0 on success or 1 otherwhise
 */
int postNotEmpty();

/**
 * @brief Gets the value of the notFull semaphore
 * 
 * @return value of the semaphore
 */
int getvalueNotFull();

/**
 * @brief Gets the value of the notEmpty semaphore
 * 
 * @return value of the semaphore
 */
int getvalueNotEmpty();

/**
 * @brief Destroys the semaphores that were initialized
 * 
 * @return 0 on success or 1 otherwhise
 */
int destroySems();

#endif 