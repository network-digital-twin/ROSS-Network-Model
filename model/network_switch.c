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
#define QOS_QUEUE_CAPACITY 65536 // 64KB (in bytes)
#define BANDWIDTH 10 // 10Gbps, switch-to-switch bandwidth
#define PROPAGATION_DELAY 50000 // 50000ns, switch-to-switch propagation delay
#define SWITCH_TO_TERMINAL_PROPAGATION_DELAY 5000 // 5000ns
#define SWITCH_TO_TERMINAL_BANDWIDTH 100 // 100Gbps
#define NUM_TO_SWITCH_PORTS 2 // Number of to-switch ports


//-------------Switch stuff-------------

void switch_init (switch_state *s, tw_lp *lp)
{
    tw_lpid self = lp->gid;
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
     tw_lpid self = lp->gid;
     // printf("%d: I am a switch\n",self);
}

void handle_arrive_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{


    tw_lpid self = lp->gid;
    tw_stime ts;
    tw_lpid next_dest; // to be decided by the routing table
    int out_port; // to be decided by the routing table
    tw_lpid final_dest_LID = in_msg->packet.final_dest_LID;

    if(self==0) {
        printf("ARRIVE[%llu][%f]", lp->gid, tw_now(lp));
        print_message(in_msg);
    }

    // update statistics
    s->num_packets_recvd++; ///////////////////// STATE CHANGE


    /* ------- ROUTING ------- */
    // Determine: Are you the switch that is to deliver the message or do you need to route it to another one
    tw_lpid assigned_switch_gid = get_assigned_switch_GID(final_dest_LID);
    if(self == assigned_switch_gid) //You are the switch to deliver the packet, then send to terminal directly
    {
        if(self==0) {
            printf("I'm the last hop. \n");
        }
        bf->c0 = 1;  // use the bit field to record the "if" branch
        next_dest = get_terminal_GID(final_dest_LID);

        // Calculate the delay of the event
        double injection_delay = calc_injection_delay(in_msg->packet.packet_size_in_bytes, SWITCH_TO_TERMINAL_BANDWIDTH);
        double propagation_delay = SWITCH_TO_TERMINAL_PROPAGATION_DELAY;
        ts = injection_delay + propagation_delay;
        assert(ts >0);

        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        memcpy(out_msg, in_msg, sizeof(tw_message));
        out_msg->packet.sender = self;
        out_msg->packet.next_dest_GID = next_dest;
        out_msg->port_id = -1; // set this unused variable to -1.
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
    int meter_index = out_port * NUM_QOS_LEVEL + in_msg->packet.packet_type;  // TODO: use a function to wrap this calculation
    in_msg->qos_state_snapshot.meter_index = meter_index;  // save the meter index for reverse computation

    /*------- METER -------*/
    // Take a snapshot for reverse computation
    srTCM_state *meter_state = &in_msg->qos_state_snapshot.meter_state;
    srTCM_snapshot(&s->meter_list[meter_index], meter_state);
    // Update
    int color = srTCM_update(&s->meter_list[meter_index], in_msg, tw_now(lp));
    ///////////////////// STATE CHANGE


    /*------- DROPPER and QUEUE -------*/
    // Use a dropper to drop the packet if it is red or the queue is full.
    // Otherwise, put the packet into the corresponding queue
    int queue_index = meter_index;
    queue_t *queue = &s->qos_queue_list[queue_index];
    if(color == COLOR_RED || queue->size_in_bytes + in_msg->packet.packet_size_in_bytes > queue->max_size_in_bytes) {
        bf->c1 = 1;  // use the bit field to record the "if" branch
        if(self==0) {
            printf("Packet dropped at switch %llu\n", self);
        }
        return;
    } else {
        // enqueue the node and modify the routing info of the enqueued node
        ///////////////////// STATE CHANGE
        node_t *enqueued_node = queue_put(queue, in_msg);
        enqueued_node->data.next_dest_GID = next_dest;
    }


    /* ------- DECIDE TO SEND a packet NOW OR IN THE FUTURE -------*/
    // If the sending handler is already issuing recursive sending events, then do not schedule a new SEND event here
    if(s->port_flags[out_port]) {
        bf->c2 = 1; // use the bit field to record the "if" branch
        return;
    }

    // Send out now?
    token_bucket *shaper = &s->shaper_list[out_port];
    sp_scheduler *scheduler = &s->scheduler_list[out_port];
    in_msg->port_id = out_port;  // save the out_port for reverse computation
    if(shaper->next_available_time <= tw_now(lp)) { // Send out now!

        bf->c3 = 1; // use the bit field to record the "if" branch

        // Use scheduler to take a packet from the queue
        node_t *node = sp_update(scheduler);  ///////// STATE CHANGE
        packet *pkt = &node->data;
        sp_delta(scheduler, pkt, &in_msg->qos_state_snapshot.scheduler_state);  // save the state of the scheduler for reverse computation

        // Update token and next_available_time of shaper
        token_bucket_snapshot(shaper, &in_msg->qos_state_snapshot.shaper_state);  // save the state of the shaper for reverse computation
        token_bucket_consume(shaper, pkt, tw_now(lp)); ///////// STATE CHANGE

        // Calculate the delay
        double injection_delay = calc_injection_delay(pkt->packet_size_in_bytes, s->bandwidths[out_port]);
        double propagation_delay = s->propagation_delays[out_port];
        ts = injection_delay + propagation_delay;
        assert(ts >0);

        // Send the packet to the destination switch
        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->packet = *pkt;  // TODO: check if this is correct
        out_msg->packet.sender = self;
        out_msg->port_id = -1; // no use here, so set it to -1.
        out_msg->type = ARRIVE;
        tw_event_send(e);

        free(node);

        // Update statistics
        s->num_packets_sent++;   /////// STATE CHANGE
        if(self==0) {
            printf("Send out now.\n");
        }

    } else { // Send out in the future! Schedule a future SEND event to myself
        s->port_flags[out_port] = 1;  // Set the flag for this port, indicating the port can self-trigger SEND events
        /////// STATE CHANGE

        ts = shaper->next_available_time - tw_now(lp);
        assert(ts >0);

        tw_event *e = tw_event_new(self,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->type = SEND;
        out_msg->port_id = out_port;
        tw_event_send(e);
        if(self==0) {
            printf("-----Schedule to SEND at: now+%f. queue size: %d, %d, %d.\n", ts,
                   scheduler->queue_list[0].num_packets, scheduler->queue_list[1].num_packets,
                   scheduler->queue_list[2].num_packets);
        }
    }

}

void handle_arrive_event_rc(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    s->num_packets_recvd--;

    // Final hop
    if (bf->c0) {
        return;
    }

    /* ------- METER ------- */
    int meter_index = in_msg->qos_state_snapshot.meter_index;
    srTCM_update_reverse(&s->meter_list[meter_index], &in_msg->qos_state_snapshot.meter_state);

    // Packet dropped
    if (bf->c1) {
        return;
    }

    /* ------- QUEUE ------- */
    int queue_index = meter_index;
    queue_t *queue = &s->qos_queue_list[queue_index];
    queue_put_reverse(queue);


    if (bf->c2) {
        return;
    }


    // Has previously decided to send out now or in the future?
    if (bf->c3) { // Sent out now
        token_bucket *shaper = &s->shaper_list[in_msg->port_id];
        sp_scheduler *scheduler = &s->scheduler_list[in_msg->port_id];
        //sp_update_reverse(scheduler, , &in_msg->qos_state_snapshot.scheduler_state);
        token_bucket_consume_reverse(shaper, &in_msg->qos_state_snapshot.shaper_state);
        s->num_packets_sent--;
        return;

    } else { // Sent out in the future
        s->port_flags[in_msg->port_id] = 0;
        return;
    }

}


void handle_send_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    /* Should only use in_msg->port_id. Do not use other fields of in_msg */
    tw_lpid self = lp->gid;
    tw_lpid next_dest;
    int out_port = in_msg->port_id;
    sp_scheduler *scheduler = &s->scheduler_list[out_port];
    token_bucket *shaper = &s->shaper_list[out_port];

    if(self==0) {
        printf("SEND[%llu][%f]\n", self, tw_now(lp));
        printf("-----Handle Send. queue size: %d, %d, %d.\n", scheduler->queue_list[0].num_packets,
               scheduler->queue_list[1].num_packets, scheduler->queue_list[2].num_packets);
    }

    // Use scheduler to dequeue a packet. Need to free the memory later within this function.
    node_t *node = sp_update(scheduler);  /////// STATE CHANGE
    packet *pkt = &node->data;
    next_dest = pkt->next_dest_GID;

    // Update shaper: token and next_available_time
    token_bucket_consume(shaper, pkt, tw_now(lp));  /////// STATE CHANGE


    // Calculate the delay of the event
    double injection_delay = calc_injection_delay(pkt->packet_size_in_bytes, s->bandwidths[out_port]);
    double propagation_delay = s->propagation_delays[out_port];
    tw_stime ts = injection_delay + propagation_delay;
    assert(ts >0);

    // Then send the packet to the destination
    tw_event *e = tw_event_new(next_dest,ts,lp);
    tw_message *out_msg = tw_event_data(e);
    out_msg->packet = *pkt;
    out_msg->packet.sender = self;
    out_msg->port_id = -1; // no use here, so set it to -1.
    out_msg->type = ARRIVE;
    tw_event_send(e);

    if(self==0) {
        printf("-----Send now. ");
        print_message(out_msg);
    }

    free(node);


    // If the scheduler has more to send, then schedule a new SEND event
    if(sp_has_next(scheduler)) {
        ts = shaper->next_available_time - tw_now(lp);
        assert(ts >0);

        tw_event *e = tw_event_new(self,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->type = SEND;
        out_msg->port_id = out_port;
        tw_event_send(e);

        if(self==0) {
            printf("-----Schedule to SEND at: now+%f. queue size: %d, %d, %d.\n", ts,
                   scheduler->queue_list[0].num_packets, scheduler->queue_list[1].num_packets,
                   scheduler->queue_list[2].num_packets);
        }
    } else {
        s->port_flags[out_port] = 0;  // Clear the flag for this port, indicating the port will not self-trigger SEND events
    }


    // Update statistics
    s->num_packets_sent++;   /////// STATE CHANGE

}

void handle_send_event_rc(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    s->num_packets_sent--;

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
            handle_arrive_event_rc(s, bf, in_msg, lp);
            break;
        }
        case SEND :
        {
            handle_send_event_rc(s, bf, in_msg, lp);
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
     tw_lpid self = lp->gid;
//    for(int i = 0; i < NUM_QOS_LEVEL; i++)
//    {
//        queue_destroy(&s->qos_queue_list[i]);
//    }
//    free(s->qos_queue_list);
     printf("Switch %llu: S:%d R:%d messages\n", self, s->num_packets_sent, s->num_packets_recvd);
}
