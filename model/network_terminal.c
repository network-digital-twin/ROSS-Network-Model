//
// Created by Nan on 2024/1/11.
//
// The C driver file for a ROSS model
// This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

// Includes
#include "network.h"

//--------------Terminal stuff-------------

void terminal_parse_workload(terminal_state *s, tw_lp *lp)
{
    s->pks = parseWorkload("/home/lenovo/Documents/GitHub/ROSS-Network-Model/model/data/trace_second_SG.txt");

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

void terminal_init(terminal_state *s, tw_lp *lp)
{
    tw_lpid self = lp->gid;

    // init state data
    s->num_packets_recvd = -1;
    // for now hardcode the path

    terminal_parse_workload(s, lp);

    int i;
    for (i = 0; i < MSG_PER_TERMINAL; i++)
    {
        // tw_event *e = tw_event_new(self,tw_rand_exponential(lp->rng, MEAN_TERMINAL_WAIT),lp);
        // tw_stime ts = 1;
        // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_TERMINAL_WAIT) + lookahead;
        tw_stime ts = tw_rand_exponential(lp->rng, MEAN_TERMINAL_WAIT) + 1;
        tw_event *e = tw_event_new(self, ts, lp);
        message *msg = tw_event_data(e);
        msg->sender = self;
        msg->final_dest = self;
        msg->type = KICKOFF;
        tw_event_send(e);
    }
}

void terminal_prerun(terminal_state *s, tw_lp *lp)
{
    int self = lp->gid;
    tw_lpid assigned_switch = get_assigned_switch_LID(lp->gid);

    // printf("%d: I am a terminal assigned to PO %llu\n", self, assigned_switch);
}

void terminal_event_handler(terminal_state *s, tw_bf *bf, message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_lpid final_dest;
    tw_lpid next_dest;

    // unsigned int ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
    // dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
    final_dest = tw_rand_integer(lp->rng, 0, total_terminals - 1);

    // Next destination from a terminal is its assigned switch
    tw_lpid assigned_switch = get_assigned_switch_LID(lp->gid);
    next_dest = get_switch_GID(assigned_switch);

    // // initialize the bit field
    // *(int *) bf = (int) 0;

    s->num_packets_recvd++;

    // schedule a new message
    //  tw_event *e = tw_event_new(self,tw_rand_exponential(lp->rng, MEAN_TERMINAL_WAIT),lp);
    //  tw_stime ts = 1;
    //  tw_stime ts = tw_rand_exponential(lp->rng, mean) + lookahead + (tw_stime)(lp->gid % (unsigned int)g_tw_ts_end);
    //  tw_stime ts = tw_rand_exponential(lp->rng, MEAN_TERMINAL_WAIT) + lookahead;
    tw_stime ts = tw_rand_exponential(lp->rng, MEAN_TERMINAL_WAIT) + 1;
    tw_event *e = tw_event_new(next_dest, ts, lp);
    message *out_msg = tw_event_data(e);
    out_msg->sender = self;
    out_msg->final_dest = final_dest;
    out_msg->next_dest = next_dest;
    out_msg->type = ARRIVE;
    tw_event_send(e);
    s->num_packets_sent++;
}

void terminal_RC_event_handler(terminal_state *s, tw_bf *bf, message *in_msg, tw_lp *lp)
{
    tw_rand_reverse_unif(lp->rng);
    tw_rand_reverse_unif(lp->rng);
    s->num_packets_recvd--;
    s->num_packets_sent--;
}

void terminal_final(terminal_state *s, tw_lp *lp)
{
    int self = lp->gid;
    printf("Terminal %d: S:%d R:%d messages\n", self, s->num_packets_sent, s->num_packets_recvd);
}

// void terminal_commit(state * s, tw_bf * bf, message * m, tw_lp * lp)
// {
// }
