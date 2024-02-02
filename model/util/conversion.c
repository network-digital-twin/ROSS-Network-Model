//
// Created by Nan on 2024/2/2.
//
#include "../network.h"

/**
 * calculate the injection delay in ns.
 * @param bytes size of the packet to be transmitted
 * @param GB_p_s bandwidth in GiB/s
 * @return delay in nano seconds
 */
tw_stime calc_injection_delay(int bytes, double GB_p_s) {
    // bytes to GiB
    tw_stime time = ((double)bytes)/(1024.0*1024.0*1024.0);
    // delay in s
    time = time / GB_p_s;
    // s to ns
    time = time * 1000.0 * 1000.0 * 1000.0;
    return time;
}
