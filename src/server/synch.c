

#include "options.h"
#include "../sope.h"
#include "../types.h"

#include "../constants.h"
#include <pthread.h>
#include <semaphore.h>
#include "synch.h"


static pthread_mutex_t bankAccounts_mutex[MAX_BANK_ACCOUNTS];
static sem_t notFull, notEmpty;


int initializeMutex(int nr) {
    for(int i=0; i< nr; i++ ) {
        if (pthread_mutex_init(&bankAccounts_mutex[i],NULL) != 0 ) {
            return 1;
        }
    }
    return 0;
}

int destroyMutex(int nr) {
    for(int i=0; i< nr; i++ ) {
        if (pthread_mutex_destroy(&bankAccounts_mutex[i]) != 0 ) {
            return 1;
        }
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
    if  (pthread_mutex_unlock(&bankAccounts_mutex[id]) !=0) {
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

int initializeSemNotEmpty(int semsNo) {
    if (sem_init(&notEmpty, 0, semsNo) == -1) {
        return 1;
    }
    return 0;
}

int waitNotFull() {
    if(sem_wait(&notFull) == -1) {
        return 1;
    }
    printf("wait not full\n");
    return 0;
}

int waitNotEmpty() {
    printf("wait not empty\n");

    if (sem_wait(&notEmpty) == -1) {
        return 1;
    }

    printf("a seguir\n");
    return 0;
}

int postNotFull() {

    if (sem_post(&notFull) == -1) {
        return 1;
    }
    printf("post not full\n");
    return 0;
}

int postNotEmpty() {
    if (sem_post(&notEmpty) == -1) {
        return 1;
    }
    printf("post not empty\n");
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

