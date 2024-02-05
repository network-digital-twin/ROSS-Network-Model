//
// Created by Nan on 2024/1/24.
//

#include <assert.h>
#include "network.h"
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

void token_bucket_init(token_bucket *bucket, long long capacity, long long rate, double port_bandwidth) {
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
void token_bucket_consume(token_bucket *bucket, const packet *pkt, tw_stime current_time) {
    long long num_new_tokens;
    int packet_size = pkt->packet_size_in_bytes;

    // Calculate the number of newly generated tokens
    assert(bucket->rate < INT_MAX);  // if rate is too large, the following line of calculation may lose precision
    num_new_tokens = (long long) (bucket->rate * (current_time - bucket->last_update_time) / (1000.0 * 1000.0 * 1000.0));
    bucket->last_update_time = current_time;

    // Add tokens to the bucket
    bucket->tokens += num_new_tokens;
    if(bucket->tokens > bucket->capacity) {
        bucket->tokens = bucket->capacity;
    }

    // Let the packet consume the tokens
    bucket->tokens -= packet_size * 8;

    // Calculate the next available time
    double injection_delay = calc_injection_delay(packet_size, bucket->port_bandwidth);
    if(bucket->tokens > 0) {
        bucket->next_available_time = current_time + injection_delay;
    }
    else {
        // The following conversion should not be a problem,
        // since (1 - bucket->tokens) depends on the packet_size, which is an int.
        // So abs(1 - bucket->tokens) will not be too large.
        assert(bucket->tokens - 1 < INT_MAX);
        double ts = (1 - bucket->tokens) / bucket->rate * 1000.0 * 1000.0 * 1000.0;  // The time (ns) it takes to accumulate to 1 token
        bucket->next_available_time = current_time + MAX(injection_delay, ts);
    }

}

void token_bucket_consume_reverse(token_bucket *bucket, token_bucket_state *bucket_state) {
    bucket->tokens = bucket_state->tokens;
    bucket->last_update_time = bucket_state->last_update_time;
    bucket->next_available_time = bucket_state->next_available_time;
}

void token_bucket_snapshot(token_bucket *bucket, token_bucket_state *state) {
    state->tokens = bucket->tokens;
    state->last_update_time = bucket->last_update_time;
    state->next_available_time = bucket->next_available_time;
}