#ifndef QUEUE_H___
#define QUEUE_H___

#include "../types.h"

typedef struct {
    tlv_request_t ** requests;
    int size, read_p, write_p;
    int itemsNo;
} queue_t;

queue_t * createQueue(int size);

int freeQueue(queue_t *queue);

void readRequest(queue_t *queue, tlv_request_t** request_ptr);

void writeRequest(queue_t *queue,tlv_request_t* request);

#endif // QUEUE_H___