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

static pthread_mutex_t queueLock;

static void freeQueue(int status,void *arg) {
    queue_t *queue = (queue_t *)arg;
    free(queue->requests);
    queue->requests = NULL;
    queue->size = queue->read_p = queue->write_p = 0;
      
    pthread_mutex_destroy(&queueLock);

    free(queue);
}

void readRequest(queue_t *queue, tlv_request_t** request) {
    pthread_mutex_lock(&queueLock);

    *request = queue->requests[queue->read_p];
    queue->read_p = (queue->read_p + 1) % queue->size;

    queue->itemsNo--;
    pthread_mutex_unlock(&queueLock);
}

void writeRequest(queue_t *queue,tlv_request_t* request) {
    pthread_mutex_lock(&queueLock);

    queue->requests[queue->write_p] = request;
    queue->write_p = (queue->write_p + 1) % queue->size;
    queue->itemsNo++;

    pthread_mutex_unlock(&queueLock);
}

queue_t * createQueue(int size) {
    queue_t * queue = (queue_t *)malloc(sizeof(queue_t));
    queue->requests = (tlv_request_t **) malloc(size * sizeof(tlv_request_t *));
    queue->size = size;
    queue->itemsNo = 0;

    pthread_mutex_init(&queueLock, NULL);

    on_exit(freeQueue,queue);

    return queue;
}
