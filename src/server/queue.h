#ifndef QUEUE_H___
#define QUEUE_H___

#include "../types.h"

void initQueue(int size);

void readRequest(tlv_request_t** request_ptr);

void writeRequest(tlv_request_t* request);

#endif // QUEUE_H___