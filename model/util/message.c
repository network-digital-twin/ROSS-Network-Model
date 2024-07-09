//
// Created by Nan on 2024/2/2.
//

#include "network.h"

void print_message(const tw_message *msg) {
    printf("pid %llu, "
           "send_time %f, "
           "src %lu, "
           "dest %lu, "
           "destIP %s, "
           "prev_hop %lu, "
           "next_hop %lu, "
           "message_type %d, "
           "packet_size %d, "
           "type %d, "
           "port_id %d, "
           "TTL %d "
           "\n",
           msg->packet.pid,
           msg->packet.send_time,
           msg->packet.src,
           msg->packet.dest,
           msg->packet.destIP,
           msg->packet.prev_hop,
           msg->packet.next_hop,
           msg->type,
           msg->packet.size_in_bytes,
           msg->packet.type,
           msg->port_id,
           msg->packet.TTL
           );

}
