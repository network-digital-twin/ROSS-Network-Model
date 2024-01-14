//
// Created by Nan on 2024/1/11.
//

#include "queue_fifo.h"
#include <stdlib.h>
#include "assert.h"


queue_t *queue_init(int capacity_in_bytes) {
    assert(capacity_in_bytes >= 0);
    queue_t *queue = malloc(sizeof(queue_t));
    queue->max_size_in_bytes = capacity_in_bytes;
    queue->size_in_bytes = 0;
    queue->num_packets = 0;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void queue_destroy(queue_t *queue) {
    node_t *current = queue->head;
    while (current != NULL) {
        node_t *next = current->next;
        free(current);
        current = next;
    }
    free(queue);
}

/***
 * Enqueue: Put a message at the tail of queue. User must ensure there is enough space in the queue!
 * We use assert() to check this.
 * @param queue
 * @param msg user should create this message in advance
 * @return
 */
void queue_append_to_tail(queue_t *queue, const letter msg) {
    assert(queue->size_in_bytes + msg.packet_size > queue->max_size_in_bytes);

    node_t *new_node = malloc(sizeof(node_t));
    new_node->data = msg;
    new_node->next = NULL;
    if (queue->head == NULL) {
        queue->head = new_node;
        new_node->prev = NULL;
    } else {
        queue->tail->next = new_node;
        new_node->prev = queue->tail;
    }
    queue->tail = new_node;

    // update queue statistics
    queue->num_packets++;
    queue->size_in_bytes += msg.packet_size;
}

node_t *queue_return_from_tail(queue_t *queue) {
    if (queue->num_packets == 0) {
        return NULL;
    }
    node_t *tail_node = queue->tail;
    node_t *temp = queue->tail->prev;
    temp->next = NULL;
    free(queue->tail);
    queue->tail = temp;
    tail_node->prev = NULL;
    tail_node->next = NULL;

    // update queue statistics
    queue->num_packets--;
    queue->size_in_bytes -= tail_node->data.packet_size;
    return tail_node;
}


/***
 * Enqueue: Put a message at the head of queue. User must ensure there is enough space in the queue!
 * We use assert() to check this.
 * @param queue
 * @param msg user should create this message in advance
 */
void queue_append_to_head(queue_t *queue, const letter msg) {
    assert(queue->size_in_bytes + msg.packet_size > queue->max_size_in_bytes);

    node_t *new_node = malloc(sizeof(node_t));
    new_node->data = msg;
    new_node->next = queue->head;
    new_node->prev = NULL;
    queue->head->prev = new_node;
    queue->head = new_node;

    // update queue statistics
    queue->num_packets++;
    queue->size_in_bytes += msg.packet_size;
}


node_t *queue_return_from_head(queue_t *queue) {
    if (queue->head == NULL) {
        return NULL;
    }
    node_t *head_node = queue->head;
    queue->head = head_node->next;
    head_node->prev = NULL;  // this is redundant
    head_node->next = NULL;

    // update queue statistics
    queue->num_packets--;
    queue->size_in_bytes -= head_node->data.packet_size;
    return head_node;
}


/***
 * Enqueue: Put a message at the tail of queue. User must ensure there is enough space in the queue!
 * We use assert() to check this.
 * @param queue
 * @param msg user should create this message in advance
 */
void queue_put(queue_t *queue, letter msg) {
    queue_append_to_tail(queue, msg);
}

void queue_put_reverse(queue_t *queue) {
    assert(queue->num_packets > 0);
    node_t *node = queue_return_from_tail(queue);
    free(node);
}

/***
 * Dequeue: Get a message from the head of queue. User must ensure there is at least one message in the queue!
 * We use assert() to check this.
 * @param queue
 * @return
 */
node_t *queue_take(queue_t *queue) {
    assert(queue->num_packets > 0);
    node_t *node = queue_return_from_head(queue);
    return node;
}

void queue_take_reverse(queue_t *queue, letter msg) {
    queue_append_to_head(queue, msg);
}