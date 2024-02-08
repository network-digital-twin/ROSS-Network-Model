
// The C driver file for a ROSS model
// This file includes:
//  - an initialization function for each LP type
//  - a forward event function for each LP type
//  - a reverse event function for each LP type
//  - a finalization function for each LP type

// Includes
#include "network.h"
#include <string.h>

//-------------Switch stuff-------------

void switch_init_config(switch_state *s, tw_lp *lp)
{
    s->conf = parseConfigFile("/Users/georgediamantopoulos/Desktop/PhD/ZTE_proj/ROSS/ROSS-Network-Model/model/data/second_subgraph/1.yaml", 1);

    printf("----------------------------------------------------\n");
    printf("----------------------------------------------------\n");
    // some prints for validation
    printf("I am switch: printing my config and searching for next hop... \n");
    printConfig(s->conf);
    char *nextPort = getNextHopPort(s->conf, 11);
    printf("Next hop for dest node 11 is: %s\n", nextPort);
    printf("Looking for its BW: %f \n", getPortBandwidth(s->conf, nextPort));
    printf("----------------------------------------------------\n");
    printf("----------------------------------------------------\n");
}

void switch_init(switch_state *s, tw_lp *lp)
{
    int self = lp->gid;

    // init state data
    s->num_packets_recvd = 0;
    // for now hardcode path for testing

    switch_init_config(s, lp);
}

void switch_prerun(switch_state *s, tw_lp *lp)
{
    int self = lp->gid;
    // printf("%d: I am a switch\n",self);
}

void handle_arrive_event(switch_state *s, tw_bf *bf, message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_stime ts = 1;

    // modify the state here
    s->num_packets_recvd++;

    tw_event *e = tw_event_new(self, ts, lp);
    message *out_msg = tw_event_data(e);
    memcpy(out_msg, in_msg, sizeof(message));
    out_msg->type = SEND;
    tw_event_send(e);
}

void handle_send_event(switch_state *s, tw_bf *bf, message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_lpid final_dest;
    tw_lpid next_dest;

    // Determine: Are you the switch that is to deliver the message or do you need to route it to another one
    int assigned_switch_gid = get_assigned_switch_GID(in_msg->final_dest);
    if (self == assigned_switch_gid) // You are the switch to deliver the message
    {
        final_dest = in_msg->final_dest;
        next_dest = in_msg->final_dest;

        // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + lookahead;
        tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + 5;
        tw_event *e = tw_event_new(next_dest, ts, lp);
        message *out_msg = tw_event_data(e);
        out_msg->sender = self;
        out_msg->final_dest = final_dest;
        out_msg->next_dest = next_dest;
        tw_event_send(e);
        s->num_packets_sent++;
    }
    else // You need to route it to another switch
    {
        final_dest = in_msg->final_dest;
        next_dest = assigned_switch_gid;

        // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + lookahead;
        tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + 5;
        tw_event *e = tw_event_new(next_dest, ts, lp);
        message *out_msg = tw_event_data(e);
        out_msg->sender = self;
        out_msg->final_dest = final_dest;
        out_msg->next_dest = next_dest;
        tw_event_send(e);
        s->num_packets_sent++;
    }
}

void switch_event_handler(switch_state *s, tw_bf *bf, message *in_msg, tw_lp *lp)
{

    switch (in_msg->type)
    {
    case ARRIVE:
    {
        handle_arrive_event(s, bf, in_msg, lp);
        break;
    }
    case SEND:
    {
        handle_send_event(s, bf, in_msg, lp);
        break;
    }
    default:
    {
        printf("ERROR: Invalid message type\n");
        return;
    }
    }
}

void switch_RC_event_handler(switch_state *s, tw_bf *bf, message *in_msg, tw_lp *lp)
{
    switch (in_msg->type)
    {
    case ARRIVE:
    {
        s->num_packets_recvd--;
        break;
    }
    case SEND:
    {
        tw_rand_reverse_unif(lp->rng);
        s->num_packets_sent--;
        break;
    }
    default:
    {
        printf("ERROR: Invalid message type\n");
        return;
    }
    }
}

void switch_final(switch_state *s, tw_lp *lp)
{
    int self = lp->gid;
    printf("Switch %d: S:%d R:%d messages\n", self, s->num_packets_sent, s->num_packets_recvd);
}
