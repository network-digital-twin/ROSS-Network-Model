#include "network.h"
#define MEAN_WAIT_TIME 112 // in nanosecond, this is the mean inter-arrival time of two packets (100Gbps; 1400B per packet)

void kickoff(switch_state * s, tw_lp * lp) {
    tw_lpid self = lp->gid;
    if (strcmp(s->conf->type, "Access") == 0) {
        tw_stime ts = tw_rand_exponential(lp->rng, MEAN_WAIT_TIME) + 1;
        tw_event *e = tw_event_new(self, ts, lp);
        tw_message *kickoff_msg = tw_event_data(e);
        kickoff_msg->type = KICKOFF;
        tw_event_send(e);
    }
}


// Schedule an ARRIVE event and a new KICKOFF event
void handle_kickoff_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    tw_lpid self = lp->gid;
    tw_lpid dest = total_switches - self; //TODO: now here is hard coded
    tw_stime ts_now = tw_now(lp);
    tw_stime ts = 1;
    int priority = tw_rand_integer(lp->rng, 0, 2);

    // Generate an ARRIVE event to myself
    tw_event *e = tw_event_new(self, ts, lp);
    tw_message *out_msg =tw_event_data (e);
    out_msg->packet.pid = 0;
    out_msg->packet.send_time = ts_now + ts;
    out_msg->packet.src = self;
    out_msg->packet.dest = dest;
    out_msg->packet.prev_hop = -1;
    out_msg->packet.next_hop = -1;
    out_msg->packet.type = priority;
    out_msg->packet.size_in_bytes = 1400;
    out_msg->type = ARRIVE;
    out_msg->port_id = -1;  // this variable is of no use here, so set it to -1.
    tw_event_send(e);

    // Generate a new KICKOFF event to myself
    ts = MEAN_WAIT_TIME;
    tw_event *e_kick = tw_event_new(self, ts, lp);
    tw_message *kickoff_msg = tw_event_data(e_kick);
    kickoff_msg->type = KICKOFF;
    tw_event_send(e_kick);
#ifdef DEBUG
    if(self==PROBE_ID) {
        printf("[%llu]interval: %f\n", self, ts);
    }
#endif

}

void handle_kickoff_event_rc(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    tw_rand_reverse_unif(lp->rng);
}
