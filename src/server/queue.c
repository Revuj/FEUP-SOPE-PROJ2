#include "queue.h"
#include "options.h"
#include "../types.h"
#include "../sope.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

static sem_t notFull, notEmpty;
static pthread_mutex_t queueLock;


void freeQueue(queue_t *queue) {
    sem_destroy(&notFull);
    sem_destroy(&notEmpty);
    
    pthread_mutex_destroy(&queueLock);
     
    free(queue->requestsQueue);
    queue->requestsQueue = NULL;
    queue->queueSize = queue->queueRead_p = queue->queueWrite_p = 0;
}

void readRequest(queue_t *queue, tlv_request_t** request_ptr) {
    sem_wait(&notEmpty);
    pthread_mutex_lock(&queueLock);

    *request_ptr = queue->requestsQueue[queue->queueRead_p];
    queue->queueRead_p = (queue->queueRead_p + 1) % queue->queueSize;

    pthread_mutex_unlock(&queueLock);
    sem_post(&notFull);
}

void writeRequest(queue_t *queue,tlv_request_t* request) {
    sem_wait(&notFull);
    pthread_mutex_lock(&queueLock);

    queue->requestsQueue[queue->queueWrite_p] = request;
    queue->queueWrite_p = (queue->queueWrite_p + 1) % queue->queueSize;

    pthread_mutex_unlock(&queueLock);
    sem_post(&notEmpty);
}

void initQueue(queue_t *queue,int size) {
    queue->requestsQueue = malloc(size * sizeof(tlv_request_t*));
    queue->queueSize = size;

    sem_init(&notFull, 0, queue->queueSize);
    sem_init(&notEmpty, 0, 0);
    pthread_mutex_init(&queueLock, NULL);
}
