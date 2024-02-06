//
// Created by Nan on 2024/2/2.
//

#include "network.h"

void print_message(const tw_message *msg) {
    printf("sender %llu, "
           "message_type %d, "
           "final_dest_LID %llu, "
           "next_dest_GID %llu, "
           "packet_size %d, "
           "type %d, "
           "port_id %d"
           "\n",
           msg->packet.sender,
           msg->type,
           msg->packet.final_dest_LID,
           msg->packet.next_dest_GID,
           msg->packet.size_in_bytes,
           msg->packet.type,
           msg->port_id
           );

}