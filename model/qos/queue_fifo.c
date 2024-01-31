//
// Created by Nan on 2024/1/11.
//

#include "mail.h"
#include <stdlib.h>
#include "assert.h"


void queue_init(queue_t *queue, int capacity_in_bytes) {
    assert(capacity_in_bytes >= 0);
    queue->max_size_in_bytes = capacity_in_bytes;
    queue->size_in_bytes = 0;
    queue->num_packets = 0;
    queue->head = NULL;
    queue->tail = NULL;
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

/**
 * Enqueue: Put a message at the tail of queue. The value of 'msg' will be copied into the queue automatically.
 * User must ensure there is enough space in the queue! We use assert() to check this.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 * @return
 */
void queue_append_to_tail(queue_t *queue, const letter *msg) {
    assert(queue->size_in_bytes + msg->packet_size < queue->max_size_in_bytes);

    node_t *new_node = malloc(sizeof(node_t));
    new_node->data = *msg;  // Copy the value that the pointer msg points to
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
    queue->size_in_bytes += msg->packet_size;
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


/**
 * Enqueue: Put a message at the head of queue. The value of 'msg' will be copied into the queue automatically.
 * User must ensure there is enough space in the queue! We use assert() to check this.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 */
void queue_append_to_head(queue_t *queue, const letter *msg) {
    assert(queue->size_in_bytes + msg->packet_size < queue->max_size_in_bytes);

    node_t *new_node = malloc(sizeof(node_t));
    new_node->data = *msg;  // Copy the value that the pointer msg points to
    new_node->next = queue->head;
    new_node->prev = NULL;
    queue->head->prev = new_node;
    queue->head = new_node;

    // update queue statistics
    queue->num_packets++;
    queue->size_in_bytes += msg->packet_size;
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


/**
 * Enqueue: Put a message at the tail of queue. The value of 'msg' will be copied into the queue automatically.
 * User must ensure there is enough space in the queue! We use assert() to check this.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 */
void queue_put(queue_t *queue, const letter *msg) {
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

/**
 * Reverse the queue_take() operation. The value of 'msg' will be copied into the queue automatically.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 */
void queue_take_reverse(queue_t *queue, const letter *msg) {
    queue_append_to_head(queue, msg);
}