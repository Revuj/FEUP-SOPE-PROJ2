#ifndef QUEUE_H___
#define QUEUE_H___

void initQueue(int size);

void readRequest(const tlv_request_t** request_ptr);

void writeRequest(const tlv_request_t* request);

#endif // QUEUE_H___