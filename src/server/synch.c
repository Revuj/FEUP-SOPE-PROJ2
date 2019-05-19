#include "options.h"
#include "../sope.h"
#include "../types.h"

#include "../constants.h"
#include <pthread.h>
#include <semaphore.h>
#include "synch.h"


static pthread_mutex_t bankAccounts_mutex[MAX_BANK_ACCOUNTS];
static sem_t notFull, notEmpty;
static pthread_mutex_t activeOffices_mutex;

int initializeMutex(int nr) {
    for(int i=0; i< nr; i++ ) {
        if (pthread_mutex_init(&bankAccounts_mutex[i],NULL) != 0 ) {
            return 1;
        }
    }
    if (pthread_mutex_init(&activeOffices_mutex,NULL) != 0 ) {
        return 1;
    }
    return 0;
}

int destroyMutex(int nr) {
    for(int i=0; i< nr; i++ ) {
        if (pthread_mutex_destroy(&bankAccounts_mutex[i]) != 0 ) {
            return 1;
        }
    }
    if (pthread_mutex_destroy(&activeOffices_mutex) != 0 ) {
        return 1;
    }
    return 0;
}

int bankAccountLock(int id) {
   if( pthread_mutex_lock(&bankAccounts_mutex[id]) != 0) {
        return 1;
    }

    return 0;
} 

int bankAccountUnlock(int id) {
    if(pthread_mutex_unlock(&bankAccounts_mutex[id]) !=0) {
        return 1;
    }
    return 0;
}

int activeOfficesLock() {
   if( pthread_mutex_lock(&activeOffices_mutex) != 0) {
        return 1;
    }

    return 0;
} 

int activeOfficesUnlock() {
    if(pthread_mutex_unlock(&activeOffices_mutex) !=0) {
        return 1;
    }
    return 0;
}

int initializeSemNotFull(int semsNo) {
    if (sem_init(&notFull, 0,semsNo) == -1) {
        return 1;
    }
    return 0;
}

int initializeSemNotEmpty() {
    if (sem_init(&notEmpty,0,0) == -1) {
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

int getvalueNotFull() {
    int value;
    sem_getvalue(&notFull,&value);
    return value;
}

int getvalueNotEmpty() {
    int value;
    sem_getvalue(&notEmpty,&value);
    return value;
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

