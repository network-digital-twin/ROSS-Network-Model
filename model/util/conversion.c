//
// Created by Nan on 2024/2/2.
//
#include "../network.h"

/**
 * calculate the injection delay in ns. (1Gbps = 1000Mbps)
 * @param bytes size of the packet to be transmitted
 * @param Gbps bandwidth in Gbps
 * @return delay in nano seconds
 */
tw_stime calc_injection_delay(int bytes, double Gbps) {
    // bytes to Gb, then calculate delay in s
    tw_stime time = ((double)bytes*8)/(1000.0*1000.0*1000.0) / Gbps;
    // s to ns
    time = time * 1000.0 * 1000.0 * 1000.0;
    return time;
}
