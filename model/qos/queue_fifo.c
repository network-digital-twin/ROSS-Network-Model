//
// Created by Nan on 2024/1/11.
//

#include "network.h"
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
 * Enqueue: Put a packet at the tail of queue. The value of 'msg->packet' will be copied into the queue automatically.
 * User must ensure there is enough space in the queue! We use assert() to check this.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 * @return pointer to the newly enqueued node
 */
node_t *queue_append_to_tail(queue_t *queue, const tw_message *msg) {
    assert(queue->size_in_bytes + msg->packet.size_in_bytes <= queue->max_size_in_bytes);

    node_t *new_node = malloc(sizeof(node_t));
    new_node->data = msg->packet;  // Copy the value that the pointer msg points to
    new_node->next = NULL;
    if (queue->head == NULL) { // If there was no node in the queue
        queue->head = new_node;
        new_node->prev = NULL;
    } else {
        queue->tail->next = new_node;
        new_node->prev = queue->tail;
    }
    queue->tail = new_node;

    // update queue statistics
    queue->num_packets++;
    queue->size_in_bytes += msg->packet.size_in_bytes;
    return new_node;
}

node_t *queue_return_from_tail(queue_t *queue) {
    if (queue->num_packets == 0) {
        return NULL;
    }
    node_t *tail_node = queue->tail;
    queue->tail = queue->tail->prev;
    if (queue->tail) { // If the queue had more than one node
        queue->tail->next = NULL;
    } else { // If the queue only had one node -- no the queue is empty
        queue->tail = NULL;
        queue->head = NULL;
    }
    tail_node->prev = NULL;
//    tail_node->next = NULL; // redundant

    // update queue statistics
    queue->num_packets--;
    queue->size_in_bytes -= tail_node->data.size_in_bytes;
    return tail_node;
}


/**
 * Enqueue: Put a packet at the head of queue. The value of 'msg->packet' will be copied into the queue automatically.
 * User must ensure there is enough space in the queue! We use assert() to check this.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 */
void queue_append_to_head(queue_t *queue, const packet *pkt) {
    assert(queue->size_in_bytes + pkt->size_in_bytes <= queue->max_size_in_bytes);

    node_t *new_node = malloc(sizeof(node_t));
    new_node->data = *pkt;  // Copy the value that the pointer msg points to
    new_node->next = queue->head;
    new_node->prev = NULL;
    if(queue->head) { // If the queue was not empty
        queue->head->prev = new_node;
        queue->head = new_node;
    } else {
        queue->head = new_node;
        queue->tail = new_node;
    }
    // update queue statistics
    queue->num_packets++;
    queue->size_in_bytes += pkt->size_in_bytes;
}


node_t *queue_return_from_head(queue_t *queue) {
    if (queue->head == NULL) {
        return NULL;
    }
    node_t *head_node = queue->head;
    queue->head = head_node->next;
    if (queue->head) { // If the queue had more than one node
        queue->head->prev = NULL;
    } else { // If the queue only had one node -- no the queue is empty
        queue->tail = NULL;
    }

    head_node->prev = NULL;  // this is redundant
    head_node->next = NULL;

    // update queue statistics
    queue->num_packets--;
    queue->size_in_bytes -= head_node->data.size_in_bytes;
    return head_node;
}


/**
 * Enqueue: Put a packet at the tail of queue. The value of 'msg' will be copied into the queue automatically.
 * User must ensure there is enough space in the queue! We use assert() to check this.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 * @return pointer to the newly enqueued node
 */
node_t * queue_put(queue_t *queue, const tw_message *msg) {
    return queue_append_to_tail(queue, msg);
}

void queue_put_reverse(queue_t *queue) {
    assert(queue->num_packets > 0);
    node_t *node = queue_return_from_tail(queue);
    assert(node !=NULL);
    free(node);
}

/***
 * Dequeue: Get a packet from the head of queue. User must ensure there is at least one message in the queue!
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
 * Reverse the queue_take() operation. The value of 'msg->packet' will be copied into the queue automatically.
 * @param queue
 * @param msg the struct 'msg' internally must not contain any pointers
 */
void queue_take_reverse(queue_t *queue, const packet *pkt) {
    queue_append_to_head(queue, pkt);
}