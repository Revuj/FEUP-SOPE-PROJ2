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
     
    free(queue->requests);
    queue->requests = NULL;
    queue->size = queue->read_p = queue->write_p = 0;
}

void readRequest(queue_t *queue, tlv_request_t** request_ptr) {
    sem_wait(&notEmpty);
    pthread_mutex_lock(&queueLock);

    *request_ptr = queue->requests[queue->read_p];
    queue->read_p = (queue->read_p + 1) % queue->size;

    pthread_mutex_unlock(&queueLock);
    sem_post(&notFull);
}

void writeRequest(queue_t *queue,tlv_request_t* request) {
    sem_wait(&notFull);
    pthread_mutex_lock(&queueLock);

    queue->requests[queue->write_p] = request;
    queue->write_p = (queue->write_p + 1) % queue->size;

    pthread_mutex_unlock(&queueLock);
    sem_post(&notEmpty);
}

queue_t * createQueue(int size) {
    queue_t * queue = (queue_t *)malloc(sizeof(queue_t));
    queue->requests = (tlv_request_t **) malloc(size * sizeof(tlv_request_t *));
    queue->size = size;

    sem_init(&notFull, 0, queue->size);
    sem_init(&notEmpty, 0, 0);
    pthread_mutex_init(&queueLock, NULL);
    return queue;
}
