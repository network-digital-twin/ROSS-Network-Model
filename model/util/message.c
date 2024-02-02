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
           "packet_type %d, "
           "port_id %d"
           "\n",
           msg->sender,
           msg->type,
           msg->final_dest_LID,
           msg->next_dest_GID,
           msg->packet_size_in_bytes,
           msg->packet_type,
           msg->port_id
           );

}
