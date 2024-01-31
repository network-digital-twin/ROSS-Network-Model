//The C driver file for a ROSS model
//This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

//Includes
#include "network.h"
#include <string.h>
#include <assert.h>

#define NUM_QOS_LEVEL 3
#define QOS_QUEUE_CAPACITY 65536 // 64KB
#define BANDWIDTH 10000000000 // 10Gbps
#define PROPAGATION_DELAY 50000 // 50000ns


int num_out_ports = 1;


//-------------Switch stuff-------------

void switch_init (switch_state *s, tw_lp *lp)
{
    int self = lp->gid;

    // init state data
    s->num_packets_recvd = 0;
    s->num_queues = NUM_QOS_LEVEL * num_out_ports;
    s->num_meters = NUM_QOS_LEVEL * num_out_ports;
    s->num_schedulers = num_out_ports;
    s->num_shapers = num_out_ports;
    s->num_out_ports = num_out_ports;
    s->bandwidths = (double *)malloc(sizeof(double) * s->num_out_ports);
    s->propagation_delays = (double *)malloc(sizeof(double) * s->num_out_ports);
    for(int i = 0; i < s->num_out_ports; i++) {
        s->bandwidths[i] = BANDWIDTH;
        s->propagation_delays[i] = PROPAGATION_DELAY;
    }


    /* Init meters */
    s->meter_list = (srTCM *)malloc(sizeof(srTCM) * s->num_meters);
    params_srTCM params = {.CIR=100, .CBS=100, .EBS=100, .is_color_aware=0};
    for(int i = 0; i < s->num_meters; i++) {
        srTCM_init(&(s->meter_list[i]), &params);
    }

    /* Init qos queues */
    s->qos_queue_list = (queue_t *)malloc(sizeof(queue_t) * s->num_queues);
    for(int i = 0; i < s->num_queues; i++) {
        queue_init(&(s->qos_queue_list[i]), QOS_QUEUE_CAPACITY);
    }

    /* Init shapers */
    s->shaper_list = (token_bucket *)malloc(sizeof(token_bucket) * s->num_shapers);
    for(int i = 0; i < s->num_shapers; i++) {
         token_bucket_init(&(s->shaper_list[i]), 100, 100, s->bandwidths[i]);
    }

    /* Init schedulers: also specify the queues and shaper connected to each scheduler */
    // Should be done after the initialization of queues and shapers
    s->scheduler_list = (sp_scheduler *)malloc(sizeof(sp_scheduler) * s->num_schedulers);
    int offset = 0;
    for(int i = 0; i < s->num_schedulers; i++) {
        sp_init(&(s->scheduler_list[i]), &(s->qos_queue_list[offset]), NUM_QOS_LEVEL, &(s->shaper_list[i]));
        offset += NUM_QOS_LEVEL;
    }

    /* Init flags of ports to 0s */
    s->out_port_flags = calloc(s->num_out_ports, sizeof(int));

}

void switch_prerun (switch_state *s, tw_lp *lp)
{
     int self = lp->gid;
     // printf("%d: I am a switch\n",self);
}

void handle_arrive_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_stime ts  = 1;

    // modify the state here
    s->num_packets_recvd++;

    /* ROUTING */
    // TODO: add routing
    int out_port = 0;

    /* CLASSIFIER */
    // Classify the packet into the correct meter: get the correct meter index
    int meter_index = (out_port + 1) * in_msg->packet_type;

    /* METER */
    srTCM_state *meter_state = (srTCM_state *)malloc(sizeof(srTCM_state));
    srTCM_snapshot(&s->meter_list[meter_index], meter_state);
    int color = srTCM_update(&s->meter_list[meter_index], in_msg, tw_now(lp));  // TODO: is the time correct?
    ///////////////////// STATE CHANGE

    /* DROPPER and QUEUE */
    // Use a dropper to drop the packet if it is red or the queue is full.
    // Otherwise, put the packet into the corresponding queue
    int queue_index = meter_index;
    queue_t *queue = &s->qos_queue_list[queue_index];
    if(color == COLOR_RED || queue->size_in_bytes + in_msg->packet_size > queue->max_size_in_bytes) {
        bf->c0 = 1;  // use the bit field to record the "if" branch
        printf("Packet dropped at switch %llu\n", self);
        return;
    } else {
        queue_put(queue, in_msg);  ///////////////////// STATE CHANGE
    }

    // If the sending handler is already issuing recursive sending events, then do not schedule a new SEND event here
    if(s->out_port_flags[out_port]) {
        bf->c1 = 1; // use the bit field to record the "if" branch
        return;
    }

    if(s->shaper_list[out_port].next_available_time <= tw_now(lp)) {
        // Send out now
        bf->c2 = 1; // use the bit field to record the "if" branch

        // Use scheduler to take a packet from the queue
        node_t *node = sp_update(&s->scheduler_list[out_port]);
        // Update token and next_available_time of shaper
        token_bucket_consume(&s->shaper_list[out_port], &node->data, tw_now(lp));

        // Calculate the delay and then send
        double injection_delay = node->data.packet_size / s->bandwidths[out_port];
        double propagation_delay = s->propagation_delays[out_port];
        ts = injection_delay + propagation_delay;

        tw_event *e = tw_event_new(self,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, &node->data, sizeof(tw_message)); /////////
        out_msg->type = ARRIVE;
        tw_event_send(e);

        free(node);

    } else {
        // Schedule a future SEND event to myself
        s->out_port_flags[out_port] = 1;  // Set the flag for this port, indicating the port can self-trigger SEND events

        ts = tw_now(lp) - s->shaper_list[out_port].next_available_time;
        tw_event *e = tw_event_new(self,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, in_msg, sizeof(tw_message));
        out_msg->type = SEND;
        tw_event_send(e);
    }

}


void handle_send_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_lpid final_dest;
    tw_lpid next_dest;

    //Determine: Are you the switch that is to deliver the message or do you need to route it to another one
    int assigned_switch_gid = get_assigned_switch_GID(in_msg->final_dest);
    if(self == assigned_switch_gid) //You are the switch to deliver the message
    {
        final_dest = in_msg -> final_dest;
        next_dest = in_msg -> final_dest;

        // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + lookahead;
        tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + 5;
        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->sender = self;
        out_msg->final_dest = final_dest;
        out_msg->next_dest = next_dest;
        tw_event_send(e);
        s->num_packets_sent++;

    }
    else //You need to route it to another switch
    {
        final_dest = in_msg -> final_dest;
        next_dest = assigned_switch_gid;

        // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + lookahead;
        tw_stime ts = tw_rand_exponential(lp->rng, MEAN_SWITCH_PROCESS_WAIT) + 5;
        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->sender = self;
        out_msg->final_dest = final_dest;
        out_msg->next_dest = next_dest;
        tw_event_send(e);
        s->num_packets_sent++;
    }
}

void switch_event_handler(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{

    *(int *) bf = (int) 0;  // initialise the bit field. https://github.com/ROSS-org/ROSS/wiki/Tips-&-Tricks

    switch (in_msg->type) {
        case ARRIVE :
        {
            handle_arrive_event(s,bf,in_msg,lp);
            break;
        }
        case SEND :
        {
            handle_send_event(s,bf,in_msg,lp);
            break;
        }
        default :
        {
            printf("ERROR: Invalid message type\n");
            return;
        }

    }


}



void switch_RC_event_handler(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    switch (in_msg->type) {
        case ARRIVE :
        {
            s->num_packets_recvd--;
            break;
        }
        case SEND :
        {
            tw_rand_reverse_unif(lp->rng);
            s->num_packets_sent--;
            break;
        }
        default :
        {
            printf("ERROR: Invalid message type\n");
            return;
        }
    }

}

void switch_final(switch_state *s, tw_lp *lp)
{
     int self = lp->gid;
//    for(int i = 0; i < NUM_QOS_LEVEL; i++)
//    {
//        queue_destroy(&s->qos_queue_list[i]);
//    }
//    free(s->qos_queue_list);
     printf("Switch %d: S:%d R:%d messages\n", self, s->num_packets_sent, s->num_packets_recvd);
}
