//
// Created by Nan on 2024/1/24.
//

#include <assert.h>
#include "network.h"

/**
 *
 * @param bucket
 * @param capacity unit: bits (assuming 1 token == 1 bit)
 * @param rate unit: tokens per nanosecond (Giga tokens per second)
 * @param port_bandwidth
 */
void token_bucket_init(token_bucket *bucket, uint32_t capacity, double rate, double port_bandwidth) {
    bucket->capacity = capacity;
    bucket->tokens = capacity;
    bucket->rate = rate;
    bucket->port_bandwidth = port_bandwidth;
    bucket->last_update_time = 0;
}

/**
 * Update the number of tokens, let the packet consume the tokens, and update the next available time
 * @param bucket
 * @param pkt can be NULL
 * @param current_time
 * @return
 */
void token_bucket_consume(token_bucket *bucket, const packet *pkt, tw_stime current_time) {
    uint32_t num_new_tokens;
    int packet_size;
    if(pkt == NULL) {
       packet_size = 0;
    } else {
        packet_size = pkt->size_in_bytes;
    }

    // Calculate the number of newly generated tokens
    tw_stime elapsed;
    tw_stime new_time;
    if(generationInterval > 0) {
        int periods = floor((current_time - bucket->last_update_time) / generationInterval);
        elapsed = periods * generationInterval;
        new_time = elapsed + bucket->last_update_time;
    } else {
        elapsed = current_time - bucket->last_update_time;
        new_time = current_time;
    }
    assert(elapsed >=0);
    assert(bucket->rate < INT_MAX);  // if rate is too large, the following line of calculation may lose precision
    num_new_tokens = floor(bucket->rate * elapsed);
    if(num_new_tokens > 0) {
        assert(elapsed + bucket->last_update_time <= current_time);
        // If the time difference is too small, there might be no token due to the floor() function
        // Then we do not regard this as an "update"
        bucket->last_update_time = new_time;
    }

    // Add tokens to the bucket
    bucket->tokens += num_new_tokens;
    if(bucket->tokens > bucket->capacity) {
        bucket->tokens = bucket->capacity;
    }
    assert(current_time >= bucket->last_update_time);
    // Let the packet consume the tokens
    bucket->tokens -= packet_size * 8;
    assert(bucket->tokens >=0);

}

void token_bucket_consume_reverse(token_bucket *bucket, token_bucket_state *bucket_state) {
    bucket->tokens = bucket_state->tokens;
    bucket->last_update_time = bucket_state->last_update_time;
}

void token_bucket_snapshot(token_bucket *bucket, token_bucket_state *state) {
    state->tokens = bucket->tokens;
    state->last_update_time = bucket->last_update_time;
}

// Calculate the next available time when token is enough
tw_stime token_bucket_next_available_time(token_bucket *bucket, int packet_size) {
    if(token_bucket_ready(bucket, packet_size)) {
        return bucket->last_update_time;
    } else {
        // The following conversion should not be a problem,
        // since (1 - bucket->tokens) depends on the packet_size, which is an int.
        // So abs(1 - bucket->tokens) will not be too large.
        assert(packet_size - bucket->tokens< INT_MAX);
        // Return the time (ns) it takes to accumulate to packet_size token
        tw_stime delta_ts = (double)(packet_size * 8 - bucket->tokens) / bucket->rate;
        delta_ts = ceil(delta_ts / generationInterval) * generationInterval;
        return bucket->last_update_time + delta_ts;
    }
}

// return 1 for ready, 0 for not ready.
int token_bucket_ready(token_bucket *shaper, int packet_size) {
    if (packet_size * 8 <= shaper->tokens) {
        return 1;
    } else {
        return 0;
    }
}