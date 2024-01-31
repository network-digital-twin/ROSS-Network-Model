//
// Created by Nan on 2024/1/24.
//

#include "network.h"
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

void token_bucket_init(token_bucket *bucket, int capacity, double rate, double port_bandwidth) {
    bucket->capacity = capacity;
    bucket->tokens = capacity;
    bucket->rate = rate;
    bucket->port_bandwidth = port_bandwidth;
    bucket->last_update_time = 0;
    bucket->next_available_time = 0;
}

/**
 * Update the number of tokens, let the packet consume the tokens, and update the next available time
 * @param bucket
 * @param msg
 * @param current_time
 * @return
 */
void token_bucket_consume(token_bucket *bucket, const tw_message *msg, tw_stime current_time) {
    int num_new_tokens;
    int packet_size = msg->packet_size;

    // Calculate the number of newly generated tokens
    num_new_tokens = (int) (bucket->rate * (current_time - bucket->last_update_time));
    bucket->last_update_time = current_time;

    // Add tokens to the bucket
    bucket->tokens += num_new_tokens;
    if(bucket->tokens > bucket->capacity) {
        bucket->tokens = bucket->capacity;
    }

    // Let the packet consume the tokens
    bucket->tokens -= packet_size;

    // Calculate the next available time
    double injection_delay = packet_size / bucket->port_bandwidth;
    if(bucket->tokens > 0) {
        bucket->next_available_time = current_time + injection_delay;
    }
    else {
        double ts = (1 - bucket->tokens) / bucket->rate;  // The time it takes to accumulate to 1 token
        bucket->next_available_time = current_time + MAX(injection_delay, ts);
    }

}
