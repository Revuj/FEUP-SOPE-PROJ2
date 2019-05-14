

#include "options.h"
#include "../sope.h"
#include "../types.h"

#include "../constants.h"
#include <pthread.h>
#include <semaphore.h>


static pthread_mutex_t bankOffices_mutex[MAX_BANK_ACCOUNTS];
static sem_t notFull, notEmpty;


int intializeMutex(int nr) {
    for(int i=0; i< nr; i++ ) {
        if (pthread_mutex_init(&bankOffices_mutex[i],NULL) != 0 ) {
            return 1;
        }
    }
    return 0;
}

int destroyMutex(int nr) {
    for(int i=0; i< nr; i++ ) {
        if (pthread_mutex_destroy(&bankOffices_mutex[i]) != 0 ) {
            return 1;
        }
    }
    return 0;
}

int bankOfficeLock(int id) {
   if( pthread_mutex_lock(&bankOffices_mutex[id]) != 0) {
        return 1;
    }

    return 0;
} 

int bankOfficeUnlock(int id) {
    if  (pthread_mutex_unlock(&bankOffices_mutex[id]) !=0) {
        return 1;
    }
    return 0;
}

int intializeSems() {
    if (sem_init(&notFull, 0,MAX_BANK_OFFICES) == -1) {
        return 1;
    }
    if (sem_init(&notEmpty, 0, 0) == -1) {
        return 1;
    }
    return 0;
 }

int waitNotFull() {
    if(sem_wait(&notFull) == -1) {
        return 1;
    }
    return 0;
}

int waitNotEmpty() {
    if (sem_wait(&notEmpty) == -1) {
        return 1;
    }
    return 0;
}

int postNotFull() {
    if (sem_post(&notFull) == -1) {
        return 1;
    }
    return 0;
}

int postNotEmpty() {
    if (sem_post(&notEmpty) == -1) {
        return 1;
    }
    return 0;
}

int destroySems() {
    if(sem_destroy(&notEmpty) == -1) {
        return 1;
    }

    if(sem_destroy(&notFull) == -1) {
        return 1;
    }
    return 0;
}

