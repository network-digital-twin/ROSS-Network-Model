//
// Created by Nan on 2024/1/11.
//

#ifndef TEST_MODEL_QUEUE_FIFO_H
#define TEST_MODEL_QUEUE_FIFO_H

#include "mail.h"  // TODO: check if this results in repeated include

typedef struct node {
    letter data;
    struct node *next;
    struct node *prev;
} node_t;

typedef struct {
    int num_packets;
    int size_in_bytes;  // total size of all packets
    int max_size_in_bytes;  // capacity of the queue
    node_t *head;
    node_t *tail;
} queue_t;


queue_t *queue_init(int size);
void queue_destroy(queue_t *queue);

void queue_put(queue_t *queue, letter msg);
void queue_put_reverse(queue_t *queue);
node_t *queue_take(queue_t *queue);
void queue_take_reverse(queue_t *queue, letter msg);



#endif //TEST_MODEL_QUEUE_FIFO_H
