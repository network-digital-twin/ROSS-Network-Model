//
// Created by Nan on 2024/1/24.
//

#include "network.h"
#include <stdlib.h>
#include <assert.h>


// initialisation function.
void srTCM_init(srTCM *meter, const params_srTCM *params) {
    meter->last_update_time = 0;
    meter->T_c = params->CBS;
    meter->T_e = params->EBS;
    meter->params = *params;  // copy the value that the pointer points to
    assert(meter->params.CIR > 0);
}

int srTCM_update(srTCM *meter, const tw_message *msg, tw_stime current_time) {
    params_srTCM *params = &meter->params;
    const int packet_size_in_bits = msg->packet.size_in_bytes * 8;
    uint32_t num_new_tokens;
    int color = -1;

    // Calculate the number of newly generated tokens
    num_new_tokens = floor(params->CIR / 1000.0 * (current_time - meter->last_update_time));
    if(num_new_tokens > 0) {
        // If the time difference is too small, there might be no token due to the floor() function
        // Then we do not regard this as an "update"
        meter->last_update_time = current_time;
    }

    // Add tokens to the bucket(s)
    // First add to C bucket
    meter->T_c += num_new_tokens;
    if(meter->T_c > params->CBS) {
        long long _delta = meter->T_c - params->CBS;
        meter->T_c = params->CBS;
        // Then add excess tokens to E bucket
        meter->T_e += _delta;
        if(meter->T_e > params->EBS) {
            meter->T_e = params->EBS;
        }
    }

    // Mark/color the packet (T_e can be 0)
    // 1. color-blind mode
    if(meter->params.is_color_aware == 0) {
        if(packet_size_in_bits <= meter->T_c) {
            color = COLOR_GREEN;
            meter->T_c -= packet_size_in_bits;
        }
        else if(packet_size_in_bits <= meter->T_e) {
            color = COLOR_YELLOW;
            meter->T_e -= packet_size_in_bits;
        }
        else {
            color = COLOR_RED;
        }
    }
    else {
        printf("[EXIT] Color-aware mode is not supported yet! \n");
        exit(-1);
    }

    return color;

}

/**
 * Reverse the meter to a given previous state. User should handle the memory of the state.
 * @param meter
 * @param meter_state
 */
void srTCM_update_reverse(srTCM *meter, const srTCM_state *meter_state) {
    meter->T_c = meter_state->T_c;
    meter->T_e = meter_state->T_e;
    meter->last_update_time = meter_state->last_update_time;
}

/**
 * Take a snapshot of the meter's state, and store it in 'state'. Used for reverse computation.
 * @param meter
 * @param state
 */
void srTCM_snapshot(const srTCM *meter, srTCM_state* state) {
    state->T_c = meter->T_c;
    state->T_e = meter->T_e;
    state->last_update_time = meter->last_update_time;
}
