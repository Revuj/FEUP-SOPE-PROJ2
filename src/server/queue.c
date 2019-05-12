#include "queue.h"
#include "options.h"
#include "../types.h"
#include "../sope.h"
#include "../log.c"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

static const tlv_request_t** requestsQueue = NULL;
static int queueSize = 0, queueRead_p = 0, queueWrite_p = 0;

static sem_t notFull, notEmpty;
static pthread_mutex_t bufferLock;


static void freeQueue() {
    sem_destroy(&notFull);
    sem_destroy(&notEmpty);
    
    pthread_mutex_destroy(&bufferLock);
     
    free(requestsQueue);
    requestsQueue = NULL;
    queueSize = queueRead_p = queueWrite_p = 0;
}

static void readRequest(const tlv_request_t** request_ptr) {
    sem_wait(&notEmpty);
    pthread_mutex_lock(&bufferLock);

    *request_ptr = requestsQueue[queueRead_p];
    queueRead_p = (queueRead_p + 1) % queueSize;

    pthread_mutex_unlock(&bufferLock);
    sem_post(&notFull);
}

static void writeRequest(const tlv_request_t* request) {
    sem_wait(&notFull);
    pthread_mutex_lock(&bufferLock);

    requestsQueue[queueWrite_p] = request;
    queueWrite_p = (queueWrite_p + 1) % queueSize;

    pthread_mutex_unlock(&bufferLock);
    sem_post(&notEmpty);
}

static void initQueue(int size) {
    requestsQueue = malloc(size * sizeof(tlv_request_t*));
    queueSize = size;

    sem_init(&notFull, 0, queueSize);
    sem_init(&notEmpty, 0, 0);
    pthread_mutex_init(&bufferLock, NULL);

    atexit(freeQueue);
}
