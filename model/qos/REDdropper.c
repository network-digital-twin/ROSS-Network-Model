//
// Created by Nan on 2024/2/23.
//
// See INET: https://github.com/inet-framework/inet/blob/bde57389d5c9b41d18318f5bdfb88a894e6feacb/src/inet/queueing/filter/RedDropper.cc
#include <assert.h>
#include "network.h"

void REDdropper_init(REDdropper *dropper, double minth, double maxth, double maxp, double wq, queue_t* queue) {
    if(maxp != 0) {
        printf("ERROR: maxp must be 0. Other values not supported yet\n");
        exit(-1);
    }
    dropper->minth = minth;
    dropper->maxth = maxth;
    dropper->maxp = maxp;
    dropper->wq = wq;
    dropper->queue = queue;
    dropper->avg = 0;
    dropper->q_time = 0;
}

// Return 1 if to be dropped, 0 if not to be dropped.
int REDdropper_update(REDdropper *dropper, tw_stime current_time) {
    // Get current queue length
    int qlen = dropper->queue->num_packets;
    double maxth = dropper->maxth;
    double wq = dropper->wq;

    if (qlen > 0) {
        // TD: This following calculation is only useful when the queue is not empty!
        // update average queue length
        dropper->avg = (1 - wq) * dropper->avg + wq * qlen;
    } else {
        // TD: Added behaviour for empty queue.
        const double m = (current_time - dropper->q_time)/1000000000 * 150; // 150 is the default value for pkrate in INET: average packet rate for calculations when queue is empty
        dropper->avg = pow(1 - wq, m) * dropper->avg;
        dropper->q_time = current_time;
    }


    if (dropper->avg >= maxth) {
        return 1;
    }

    return 0;
}

void REDdropper_time_update(REDdropper *dropper, tw_stime current_time) {
    dropper->q_time = current_time;
}


void REDdropper_update_reverse(REDdropper *dropper, REDdropper_state *state) {
    dropper->avg = state->avg;
    dropper->q_time = state->q_time;
}

void REDdropper_snapshot(REDdropper *dropper, REDdropper_state *state) {
    state->avg = dropper->avg;
    state->q_time = dropper->q_time;
}
