#ifndef QUEUE_H___
#define QUEUE_H___

#include "../types.h"

/** @name queue_t */
/**@{ struct that holds the informations about a queue
 */
typedef struct {
    tlv_request_t ** requests; /**< @brief array of requests*/
    int size;  /**< @brief maximum size of the queue*/
    int read_p;  /**< @brief index that specifies where to read*/
    int write_p; /**< @brief index that specifies where to write*/
    int itemsNo;  /**< @brief number of requests on the queue*/
} queue_t;/** @} end of the queue_t struct */

/**
 * @brief Creates a queue
 * 
 * @param size - size of the queue to be created
 * @return pointer to the queue created
 */
queue_t * createQueue(int size);

/**
 * @brief Frees the space allocated for the queue
 * 
 * @param queue - queue to be freed
 * @return 0 on success or 1 otherwhise
 */
int freeQueue(queue_t *queue);

/**
 * @brief Reads a requests on the queue (pops)
 * 
 * @param queue - queue where the request wil be read
 * @param request_ptr - request struct to be filled 
 */
void readRequest(queue_t *queue, tlv_request_t** request);

/**
 * @brief Writes a request on the queue (pushes)
 * 
 * @param queue - queue where the request will be written
 * @param request - request to be written on the queue
 */
void writeRequest(queue_t *queue,tlv_request_t* request);

#endif // QUEUE_H___