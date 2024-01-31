//
// Created by Nan on 2024/1/25.
//

#include "network.h"

void sp_init(sp_scheduler *scheduler, queue_t *queue_list, int num_queues, token_bucket *shaper) {
    scheduler->queue_list = queue_list;
    scheduler->num_queues = num_queues;
    scheduler->last_priority = -1;
    scheduler->shaper = shaper;
}

node_t *sp_update(sp_scheduler* scheduler) {
    for(int i = 0; i < scheduler->num_queues; i++) {
        queue_t *q = &scheduler->queue_list[i];
        if(q->num_packets > 0) {
            node_t *node = queue_take(q);
            scheduler->last_priority = i;
            return node;
        }
    }
    scheduler->last_priority = -1;
    return NULL;
}

/**
 * Reverse function of sp_update(). The value of 'msg' will be copied into the corresponding queue automatically.
 * @param scheduler
 * @param msg the struct 'msg' internally must not contain any pointers
 * @param priority
 */
void sp_update_reverse(sp_scheduler* scheduler, const tw_message *msg, int priority) {
    queue_take_reverse(&scheduler->queue_list[priority], msg);
}

/**
 * Check if the scheduler has more packets to send
 * @param scheduler
 * @return 1 for true, 0 for false.
 */
int sp_has_next(const sp_scheduler *scheduler) {
    for(int i = 0; i < scheduler->num_queues; i++) {
        queue_t *q = &scheduler->queue_list[i];
        if(q->num_packets > 0) {
            return 1;
        }
    }
    return 0;
}
