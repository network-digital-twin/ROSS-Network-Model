//
// Created by Nan on 2024/2/2.
//

#include "network.h"

void print_message(const tw_message *msg) {
    printf("send_time %f, "
           "src %llu, "
           "dest %llu, "
           "prev_hop %llu, "
           "next_hop %llu, "
           "message_type %d, "
           "packet_size %d, "
           "type %d, "
           "port_id %d"
           "\n",
           msg->packet.send_time,
           msg->packet.src,
           msg->packet.dest,
           msg->packet.prev_hop,
           msg->packet.next_hop,
           msg->type,
           msg->packet.size_in_bytes,
           msg->packet.type,
           msg->port_id
           );

}
