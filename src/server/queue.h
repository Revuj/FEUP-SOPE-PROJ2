#ifndef QUEUE_H___
#define QUEUE_H___

#include "../types.h"

typedef struct {
    tlv_request_t** requestsQueue;
    int queueSize, queueRead_p, queueWrite_p;
} queue_t;

void initQueue(queue_t *queue,int size);

void freeQueue(queue_t *queue);

void readRequest(queue_t *queue, tlv_request_t** request_ptr);

void writeRequest(queue_t *queue,tlv_request_t* request);

#endif // QUEUE_H___