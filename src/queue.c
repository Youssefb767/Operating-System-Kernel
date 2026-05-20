/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Simple circular queue implementation
 */

#include <spede/string.h>

#include "queue.h"

/**
 * Initializes an empty queue
 * Sets the empty queue items to -1
 *
 * @param  queue - pointer to the queue
 * @return -1 on error; 0 on success
 */
int queue_init(queue_t *queue) {
    if (!queue) {
        return -1;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;

    for (int i = 0; i < QUEUE_SIZE; i++) {
        queue->items[i] = -1;
    }

    return 0;
}

/**
 * Adds an item to the end of a queue
 * @param  queue - pointer to the queue
 * @param  item  - the item to add
 * @return -1 on error; 0 on success
 */
int queue_in(queue_t *queue, int item) {
    if (!queue) {
        return -1;
    }

    if (queue_is_full(queue)) {
        return -1;
    }

    queue->items[queue->tail] = item;
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->size++;

    return 0;
}

/**
 * Pulls an item from the specified queue
 * @param  queue - pointer to the queue
 * @param  item  - pointer to the memory to save item to
 * @return -1 on error; 0 on success
 */
int queue_out(queue_t *queue, int *item) {
    if (!queue || !item) {
        return -1;
    }

    if (queue_is_empty(queue)) {
        return -1;
    }

    *item = queue->items[queue->head];
    queue->items[queue->head] = -1;
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->size--;

    return 0;
}

/**
 * Indicates if the queue is empty
 * @param queue - pointer to the queue structure
 * @return true if empty, false if not empty
 */
bool queue_is_empty(queue_t *queue) {
    if (!queue) {
        return true;
    }

    return queue->size == 0;
}

/**
 * Indicates if the queue is full
 * @param queue - pointer to the queue structure
 * @return true if full, false if not full
 */
bool queue_is_full(queue_t *queue) {
    if (!queue) {
        return true;
    }

    return queue->size >= QUEUE_SIZE;
}