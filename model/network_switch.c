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
#define BANDWIDTH 10000000000 // 10Gbps, switch-to-switch bandwidth
#define PROPAGATION_DELAY 50000 // 50000ns, switch-to-switch propagation delay
#define SWITCH_TO_TERMINAL_PROPAGATION_DELAY 5000 // 5000ns
#define SWITCH_TO_TERMINAL_BANDWIDTH 100000000000 // 100Gbps
#define NUM_TO_SWITCH_PORTS 2 // Number of to-switch ports


//-------------Switch stuff-------------

void switch_init (switch_state *s, tw_lp *lp)
{
    int self = lp->gid;
    int num_ports = NUM_TO_SWITCH_PORTS;  // TODO: load it from a file

    // init state data
    s->num_packets_recvd = 0;
    s->num_queues = NUM_QOS_LEVEL * num_ports;
    s->num_meters = NUM_QOS_LEVEL * num_ports;
    s->num_schedulers = num_ports;
    s->num_shapers = num_ports;
    s->num_ports = num_ports;
    s->bandwidths = (double *)malloc(sizeof(double) * s->num_ports);
    s->propagation_delays = (double *)malloc(sizeof(double) * s->num_ports);
    for(int i = 0; i < s->num_ports; i++) {
        s->bandwidths[i] = BANDWIDTH;
        s->propagation_delays[i] = PROPAGATION_DELAY;
    }

    /* Init routing table */
    // HARD CODED FOR 3 SWITCHES!
    // TODO: load it dynamically!
    assert(total_switches == 3);
    s->routing_table_size = total_switches; // number of records in the routing table
    s->routing = (route *)malloc(sizeof(route) * s->routing_table_size);
    int port_id = 0;
    for(int i = 0; i < s->routing_table_size; i++) {
        if(i == self) {
            s->routing[i].nextHop = -1;
            s->routing[i].port_id = -1;
        } else {
            s->routing[i].nextHop = i;
            s->routing[i].port_id = port_id;
            port_id++;
        }
    }


    /* Init meters */
    s->meter_list = (srTCM *)malloc(sizeof(srTCM) * s->num_meters);
    params_srTCM params = {.CIR=100*1024*1024, .CBS=500*1024*1024, .EBS=1024*1024*1024, .is_color_aware=0};
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
         token_bucket_init(&(s->shaper_list[i]), 1024*1024*1024, 100*1024*1024, s->bandwidths[i]);
    }

    /* Init schedulers */
    // also specify the queues and shaper connected to each scheduler
    // Should be done after the initialization of queues and shapers
    s->scheduler_list = (sp_scheduler *)malloc(sizeof(sp_scheduler) * s->num_schedulers);
    int offset = 0;
    for(int i = 0; i < s->num_schedulers; i++) {
        sp_init(&(s->scheduler_list[i]), &(s->qos_queue_list[offset]), NUM_QOS_LEVEL, &(s->shaper_list[i]));
        offset += NUM_QOS_LEVEL;
    }

    /* Init flags of ports to 0s */
    s->port_flags = calloc(s->num_ports, sizeof(int));

}

void switch_prerun (switch_state *s, tw_lp *lp)
{
     int self = lp->gid;
     // printf("%d: I am a switch\n",self);
}

void handle_arrive_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_stime ts;
    tw_lpid next_dest;
    tw_lpid final_dest_LID = in_msg->final_dest_LID;
    int out_port;

    // modify the state here
    s->num_packets_recvd++;


    /* ------- ROUTING ------- */
    // Determine: Are you the switch that is to deliver the message or do you need to route it to another one
    tw_lpid assigned_switch_gid = get_assigned_switch_GID(final_dest_LID);
    if(self == assigned_switch_gid) //You are the switch to deliver the packet, then send to terminal directly
    {
        bf->c0 = 1;  // use the bit field to record the "if" branch
        next_dest = get_terminal_GID(final_dest_LID);

        // Calculate the delay of the event
        double injection_delay = (double)in_msg->packet_size / SWITCH_TO_TERMINAL_BANDWIDTH;
        double propagation_delay = SWITCH_TO_TERMINAL_PROPAGATION_DELAY;
        ts = injection_delay + propagation_delay;

        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, in_msg, sizeof(tw_message));
        out_msg->sender = self;
        out_msg->next_dest_GID = next_dest;
        tw_event_send(e);

        // Update statistics
        s->num_packets_sent++;
        return;
    }

    // Else, you need to route it to another switch
    // TODO: Look up the routing table to get out_port and next_dest_GID
    out_port = s->routing[final_dest_LID].port_id;
    next_dest = s->routing[final_dest_LID].nextHop;

    /* ------- CLASSIFIER ------- */
    // Classify the packet into the correct meter: get the correct meter index
    int meter_index = out_port * NUM_QOS_LEVEL + in_msg->packet_type;  // TODO: use a function to wrap this calculation

    /*------- METER -------*/
    srTCM_state *meter_state = (srTCM_state *)malloc(sizeof(srTCM_state));
    srTCM_snapshot(&s->meter_list[meter_index], meter_state);
    int color = srTCM_update(&s->meter_list[meter_index], in_msg, tw_now(lp));
    ///////////////////// STATE CHANGE

    /*------- DROPPER and QUEUE -------*/
    // Use a dropper to drop the packet if it is red or the queue is full.
    // Otherwise, put the packet into the corresponding queue
    int queue_index = meter_index;
    queue_t *queue = &s->qos_queue_list[queue_index];
    if(color == COLOR_RED || queue->size_in_bytes + in_msg->packet_size > queue->max_size_in_bytes) {
        bf->c1 = 1;  // use the bit field to record the "if" branch
        printf("Packet dropped at switch %llu\n", self);
        return;
    } else {
        queue_put(queue, in_msg);  ///////////////////// STATE CHANGE
    }


    /* ------- DECIDE TO SEND NOW OR IN THE FUTURE -------*/
    // If the sending handler is already issuing recursive sending events, then do not schedule a new SEND event here
    if(s->port_flags[out_port]) {
        bf->c2 = 1; // use the bit field to record the "if" branch
        return;
    }

    // Send out now?
    if(s->shaper_list[out_port].next_available_time <= tw_now(lp)) { // Send out now!

        bf->c3 = 1; // use the bit field to record the "if" branch

        // Use scheduler to take a packet from the queue
        node_t *node = sp_update(&s->scheduler_list[out_port]);
        assert(node);
        // Update token and next_available_time of shaper
        token_bucket_consume(&s->shaper_list[out_port], &node->data, tw_now(lp));

        // Calculate the delay
        double injection_delay = node->data.packet_size / s->bandwidths[out_port];
        double propagation_delay = s->propagation_delays[out_port];
        ts = injection_delay + propagation_delay;

        // Send the packet to the destination switch
        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, &node->data, sizeof(tw_message)); /////////
        out_msg->type = ARRIVE;
        tw_event_send(e);

        free(node);

        // Update statistics
        s->num_packets_sent++;   /////// STATE CHANGE

    } else { // Send out in the future! Schedule a future SEND event to myself
        s->port_flags[out_port] = 1;  // Set the flag for this port, indicating the port can self-trigger SEND events

        ts = tw_now(lp) - s->shaper_list[out_port].next_available_time;
        tw_event *e = tw_event_new(self,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, in_msg, sizeof(tw_message));
        out_msg->type = SEND;
        out_msg->port_id = out_port;
        out_msg->next_dest_GID = next_dest;
        tw_event_send(e);
    }

}


void handle_send_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_lpid next_dest = in_msg->next_dest_GID;
    int out_port = in_msg->port_id;
    sp_scheduler *scheduler = &s->scheduler_list[out_port];
    token_bucket *shaper = &s->shaper_list[out_port];

    // Use scheduler to dequeue a packet. Need to free the memory later.
    node_t *node = sp_update(scheduler);
    // Update shaper: token and next_available_time
    token_bucket_consume(shaper, &node->data, tw_now(lp));


    // Calculate the delay of the event
    double injection_delay = in_msg->packet_size / s->bandwidths[out_port];
    double propagation_delay = s->propagation_delays[out_port];
    tw_stime ts = injection_delay + propagation_delay;

    // Then send the packet to the destination
    tw_event *e = tw_event_new(next_dest,ts,lp);
    tw_message *out_msg = tw_event_data(e);
    memcpy(out_msg, in_msg, sizeof(tw_message));
    out_msg->sender = self;
    tw_event_send(e);

    free(node);


    // If the scheduler has more to send, then schedule a new SEND event
    if(sp_has_next(scheduler)) {
        ts = tw_now(lp) - shaper->next_available_time;
        tw_event *e = tw_event_new(self,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, in_msg, sizeof(tw_message));
        out_msg->type = SEND;
        out_msg->port_id = out_port;
        out_msg->next_dest_GID = next_dest;
        tw_event_send(e);
    } else {
        s->port_flags[out_port] = 0;  // Clear the flag for this port, indicating the port will not self-trigger SEND events
    }


    // Update statistics
    s->num_packets_sent++;   /////// STATE CHANGE

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