//
// Created by Nan on 2024/1/11.
//
//The C driver file for a ROSS model
//This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

//Includes
#include "network.h"

//--------------Terminal stuff-------------

void terminal_parse_workload(terminal_state *s, tw_lp *lp)
{
    s->pks = parseWorkload(trace_path);

    printf("----------------------------------------------------\n");
    printf("----------------------------------------------------\n");
    printf("I am terminal: Testing parsing of WL by printing the first 100 packets...\n");
    for (int i = 0; i < 10; i++)
    {
        printf("packet: %d, %d %d, %d, %d, %f, %d \n", s->pks.pks[i].id, s->pks.pks[i].message_id, s->pks.pks[i].src, s->pks.pks[i].dest, s->pks.pks[i].size, s->pks.pks[i].timestamp, s->pks.pks[i].priority_level);
    }
    printf("----------------------------------------------------\n");
    printf("----------------------------------------------------\n");
}

void terminal_init (terminal_state *s, tw_lp *lp)
{
    terminal_parse_workload(s, lp);

    tw_lpid self = lp->gid;

    // init state data
    s->num_packets_recvd = -1;


}

void terminal_prerun(terminal_state *s, tw_lp *lp)
{
    int src;
    tw_stime time = 0;
    tw_stime ts;

    for (int i = 0; i < s->pks.num; i++)
    {
        src = s->pks.pks[i].src;
        ts = s->pks.pks[i].timestamp - time;
        printf("To be scheduled at %f\n", s->pks.pks[i].timestamp);

        tw_event *e = tw_event_new(src,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->type = ARRIVE;
        out_msg->packet.sender = src;
        out_msg->packet.final_dest_LID = s->pks.pks[i].dest; /////////
        out_msg->packet.next_dest_GID = src; ///////
        out_msg->packet.type = s->pks.pks[i].priority_level;
        out_msg->packet.size_in_bytes = s->pks.pks[i].size;
        out_msg->port_id = -1;  // this variable is of no use here, so set it to -1.
        tw_event_send(e);

        time = s->pks.pks[i].timestamp;

    }
    printf("Schedule done\n");

    // printf("%d: I am a terminal assigned to PO %llu\n",self,assigned_switch);
}


void terminal_final(terminal_state *s, tw_lp *lp)
{
    //int self = lp->gid;
    //printf("Terminal %d: S:%d R:%d messages\n", self, s->num_packets_sent, s->num_packets_recvd);
}

