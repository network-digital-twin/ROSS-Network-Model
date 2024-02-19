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
#define QOS_QUEUE_CAPACITY 65536 // 64KiB (in bytes)
#define BANDWIDTH 10 // 10Gbps, switch-to-switch bandwidth (1Gbps = 1000Mbps)
#define PROPAGATION_DELAY 50000 // 50000ns, switch-to-switch propagation delay
#define SWITCH_TO_TERMINAL_PROPAGATION_DELAY 5000 // 5000ns
#define SWITCH_TO_TERMINAL_BANDWIDTH 100 // 100Gbps (1Gbps = 1000Mbps)
#define NUM_TO_SWITCH_PORTS 2 // Number of to-switch ports


//-------------Switch stuff-------------


void switch_init_config(switch_state *s, tw_lp *lp)
{
    char *path = (char *) malloc(strlen(route_dir_path) + 50);
    sprintf(path, "%s/%llu.yaml", route_dir_path, lp->gid);

    FILE * file;
    file = fopen(path, "r");
    if (file != NULL)
    {
        printf("%s\n", path);

        s->conf = parseConfigFile(path, 1);
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
        fclose(file);
    }
    free(path);

}

void switch_init (switch_state *s, tw_lp *lp)
{
    switch_init_config(s, lp);

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
    s->ports_available_time = (double *)malloc(sizeof(double) * s->num_ports);
    for(int i = 0; i < s->num_ports; i++) {
        s->bandwidths[i] = BANDWIDTH;
        s->propagation_delays[i] = PROPAGATION_DELAY;
        s->ports_available_time[i] = 0;
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
    params_srTCM params = {.CIR=100*1000*1000, .CBS=500*1000*1000, .EBS=1000*1000*1000, .is_color_aware=0};
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
         token_bucket_init(&(s->shaper_list[i]), 1000*1000*1000, 100*1000*1000, s->bandwidths[i]);
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
    tw_stime ts_now = tw_now(lp);
    tw_lpid next_dest; // to be decided by the routing table
    int out_port; // to be decided by the routing table
    tw_lpid final_dest_LID = in_msg->packet.final_dest_LID;

#ifdef DEBUG
    if(self==0) {
        printf("ARRIVE[%llu][%f]", lp->gid, tw_now(lp));
        print_message(in_msg);
    }
#endif
    // update statistics
    s->num_packets_recvd++; ///////////////////// STATE CHANGE


    /* ------- ROUTING ------- */
    // Determine: Are you the switch that is to deliver the message or do you need to route it to another one
    tw_lpid assigned_switch_gid = get_assigned_switch_GID(final_dest_LID);
    if(self == assigned_switch_gid) //You are the switch to deliver the packet, then send to terminal directly
    {
#ifdef DEBUG

        if(self==0) {
            printf("I'm the last hop. \n");
        }
#endif
        bf->c0 = 1;  // use the bit field to record the "if" branch
        next_dest = get_terminal_GID(final_dest_LID);

        // Calculate the delay of the event
        double injection_delay = calc_injection_delay(in_msg->packet.size_in_bytes, SWITCH_TO_TERMINAL_BANDWIDTH);
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
    int meter_index = out_port * NUM_QOS_LEVEL + in_msg->packet.type;  // TODO: use a function to wrap this calculation
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
    if(color == COLOR_RED || queue->size_in_bytes + in_msg->packet.size_in_bytes > queue->max_size_in_bytes) {
        bf->c1 = 1;  // use the bit field to record the "if" branch
#ifdef DEBUG

        if(self==0) {
            printf("Packet dropped at switch %llu\n", self);
        }
#endif
        return;
    } else {
        // enqueue the node and modify the routing info of the enqueued node
        ///////////////////// STATE CHANGE
        node_t *enqueued_node = queue_put(queue, in_msg);
        enqueued_node->data.next_dest_GID = next_dest;
        printf("[%llu][%f] queue_address %x, queue_size %d\n", self, tw_now(lp), queue, queue->num_packets);

    }

    /* ------- DECIDE TO SEND a packet NOW OR IN THE FUTURE -------*/
    sp_scheduler *scheduler = &s->scheduler_list[out_port];
    token_bucket *shaper = &s->shaper_list[out_port];
    in_msg->port_id = out_port;  // save the out_port for reverse computation

    token_bucket_snapshot(shaper, &in_msg->qos_state_snapshot.shaper_state);
    token_bucket_consume(shaper, NULL, ts_now); ///////// STATE CHANGE
    int next_pkt_size = sp_has_next(scheduler); // the size of the next packet

    // Check whether the shaper has enough token
    if (next_pkt_size <= shaper->tokens) { // SEND OUT NOW
        bf->c2 = 1; // use the bit field to record the "if" branch

        // Use scheduler to take a packet from the queue
        node_t *node = sp_update(scheduler);  ///////// STATE CHANGE
        packet *pkt = &node->data;
        assert(pkt->size_in_bytes == next_pkt_size);
        // save the state (dequeued packet and queue index) of the scheduler for reverse computation
        sp_delta(scheduler, pkt, &in_msg->qos_state_snapshot.scheduler_state);

        // Update token of shaper
        // No need to snapshot token_bucket now, because we have already done that.
        token_bucket_consume(shaper, pkt, ts_now); ///////// STATE CHANGE

        // Calculate the delay
        double injection_delay = calc_injection_delay(pkt->size_in_bytes, s->bandwidths[out_port]);
        double propagation_delay = s->propagation_delays[out_port];
        double port_av_time = s->ports_available_time[out_port];
        ts = MAX(ts_now, port_av_time) - ts_now + injection_delay + propagation_delay;
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

        // For reverse computation
        in_msg->qos_state_snapshot.port_available_time_rc = port_av_time;
        // Update port available time
        s->ports_available_time[out_port] = MAX(ts_now, port_av_time) + injection_delay;  /////// STATE CHANGE
        // Update statistics
        s->num_packets_sent++;   /////// STATE CHANGE
#ifdef DEBUG
        if(self==0) {
            printf("Send out now.\n");
        }
#endif
    } else { // SEND OUT LATER
        // If the sending handler is [NOT] already issuing recursive sending events, then schedule a new SEND event here
        if(!s->port_flags[out_port]) {
            // Send out in the future! Schedule a future SEND event to myself
            bf->c3 = 1; // use the bit field to record the "if" branch
            s->port_flags[out_port] = 1;  // Set the flag for this port, indicating the port can self-trigger SEND events
            /////// STATE CHANGE

            ts = token_bucket_next_available_time(shaper, next_pkt_size) - ts_now;
            assert(ts >0);

            tw_event *e = tw_event_new(self,ts,lp);
            tw_message *out_msg = tw_event_data(e);
            out_msg->type = SEND;
            out_msg->port_id = out_port;
            tw_event_send(e);
#ifdef DEBUG

            if(self==0) {
            printf("-----Schedule to SEND at: now+%f. queue size: %d, %d, %d.\n", ts,
                   scheduler->queue_list[0].num_packets, scheduler->queue_list[1].num_packets,
                   scheduler->queue_list[2].num_packets);
        }
#endif
        }
    }

}

void handle_arrive_event_rc(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    s->num_packets_recvd--;

    // Final hop
    if (bf->c0) {
        s->num_packets_sent--;
        return;
    }

    /* ------- METER ------- */
    int meter_index = in_msg->qos_state_snapshot.meter_index;
    srTCM_update_reverse(&s->meter_list[meter_index], &in_msg->qos_state_snapshot.meter_state);

    // Packet dropped
    if (bf->c1) {
        return;
    }

    // If the program runs to this place, it means the packet is enqueued.


    /* SCHEDULER */
    /* This must be put prior than the reverse computation of QUEUE.
     * Because in the forward computation, the queue sometimes is modified twice: we first enqueue, then dequeue.
     * Here in the reverse computation, we need to reverse the dequeue first, then reverse the enqueue.
     * */
    if (bf->c2) { // Sent out now
        sp_scheduler *scheduler = &s->scheduler_list[in_msg->port_id];
        sp_update_reverse(scheduler, &in_msg->qos_state_snapshot.scheduler_state);
        // No need to reverse shaper's tokens now, but reverse in later lines.
        s->ports_available_time[in_msg->port_id] = in_msg->qos_state_snapshot.port_available_time_rc;
        s->num_packets_sent--;

    }

    if (bf->c3) { // Sent out in the future
        s->port_flags[in_msg->port_id] = 0;
    }

    /* ------- SHAPER ------- */
    token_bucket *shaper = &s->shaper_list[in_msg->port_id];
    token_bucket_consume_reverse(shaper, &in_msg->qos_state_snapshot.shaper_state);


    /* ------- QUEUE ------- */
    int queue_index = meter_index;
    queue_t *queue = &s->qos_queue_list[queue_index];
    if(queue->size_in_bytes == 0) {
        printf("reverse[%llu][%f] queue_address %x, queue_size%d\n", lp->gid, tw_now(lp), queue,queue->num_packets);
    }
    queue_put_reverse(queue);

}


void handle_send_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp)
{
    /* Should only use in_msg->port_id. Do not use other fields of in_msg */
    tw_lpid self = lp->gid;
    tw_lpid next_dest;
    tw_stime ts;
    tw_stime ts_now = tw_now(lp);
    int out_port = in_msg->port_id;
    sp_scheduler *scheduler = &s->scheduler_list[out_port];
    token_bucket *shaper = &s->shaper_list[out_port];
#ifdef DEBUG

    if(self==0) {
        printf("SEND[%llu][%f]\n", self, tw_now(lp));
        printf("-----Handle Send. queue size: %d, %d, %d.\n", scheduler->queue_list[0].num_packets,
               scheduler->queue_list[1].num_packets, scheduler->queue_list[2].num_packets);
    }
#endif

    /* ------- DECIDE TO SEND a packet NOW OR IN THE FUTURE -------*/
    in_msg->port_id = out_port;  // save the out_port for reverse computation

    token_bucket_snapshot(shaper, &in_msg->qos_state_snapshot.shaper_state);
    token_bucket_consume(shaper, NULL, ts_now); ///////// STATE CHANGE
    int next_pkt_size = sp_has_next(scheduler); // the size of the next packet

    // Check whether the shaper has enough token
    if (next_pkt_size <= shaper->tokens) { // SEND OUT NOW
        bf->c0 = 1; // use the bit field to record the "if" branch

        // Use scheduler to dequeue a packet. Need to free the memory later within this function.
        node_t *node = sp_update(scheduler);  /////// STATE CHANGE
        packet *pkt = &node->data;
        assert(pkt->size_in_bytes == next_pkt_size);
        next_dest = pkt->next_dest_GID;
        // save the state (dequeued packet and queue index) of the scheduler for reverse computation
        sp_delta(scheduler, pkt, &in_msg->qos_state_snapshot.scheduler_state);

        // Update token of shaper
        // No need to snapshot token_bucket now, because we have already done that.
        token_bucket_consume(shaper, pkt, ts_now);  /////// STATE CHANGE

        // Calculate the delay
        double injection_delay = calc_injection_delay(pkt->size_in_bytes, s->bandwidths[out_port]);
        double propagation_delay = s->propagation_delays[out_port];
        double port_av_time = s->ports_available_time[out_port];
        ts = MAX(ts_now, port_av_time) - ts_now + injection_delay + propagation_delay;
        assert(ts >0);

        // Then send the packet to the destination
        tw_event *e = tw_event_new(next_dest,ts,lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->packet = *pkt;  // TODO: check if this is correct
        out_msg->packet.sender = self;
        out_msg->port_id = -1; // no use here, so set it to -1.
        out_msg->type = ARRIVE;
        tw_event_send(e);

        free(node);

        // For reverse computation
        in_msg->qos_state_snapshot.port_available_time_rc = port_av_time;
        // Update port available time
        s->ports_available_time[out_port] = MAX(ts_now, port_av_time) + injection_delay;  /////// STATE CHANGE
        // Update statistics
        s->num_packets_sent++;   /////// STATE CHANGE
#ifdef DEBUG
        if(self==0) {
        printf("-----Send now. ");
        print_message(out_msg);
    }
#endif

        // If the scheduler has more to send, then schedule a new SEND event
        if(sp_has_next(scheduler)) { // HAS MORE TO SEND
            next_pkt_size = sp_has_next(scheduler);
        } else {
            bf->c1 = 1; // use the bit field to record the "if" branch
            s->port_flags[out_port] = 0;  // Clear the flag for this port, indicating the port will not self-trigger SEND events
            /////// STATE CHANGE
            return; // return now!
        }
    }

    // SEND OUT LATER
    ts = token_bucket_next_available_time(shaper, next_pkt_size) - ts_now;
    assert(ts >0);

    tw_event *e = tw_event_new(self,ts,lp);
    tw_message *out_msg = tw_event_data(e);
    out_msg->type = SEND;
    out_msg->port_id = out_port;
    tw_event_send(e);
#ifdef DEBUG
        if(self==0) {
            printf("-----Schedule to SEND at: now+%f. queue size: %d, %d, %d.\n", ts,
                   scheduler->queue_list[0].num_packets, scheduler->queue_list[1].num_packets,
                   scheduler->queue_list[2].num_packets);
        }
#endif

}

void handle_send_event_rc(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    int out_port = in_msg->port_id;

    token_bucket *shaper = &s->shaper_list[out_port];
    token_bucket_consume_reverse(shaper, &in_msg->qos_state_snapshot.shaper_state);

    if(!bf->c0) {
        sp_scheduler *scheduler = &s->scheduler_list[out_port];
        sp_update_reverse(scheduler, &in_msg->qos_state_snapshot.scheduler_state);
        s->ports_available_time[out_port] = in_msg->qos_state_snapshot.port_available_time_rc;
        s->num_packets_sent--;
    }
    if(bf->c1) {
        s->port_flags[out_port] = 1;
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
    printf("REVERSE[%llu][%f]msg_type: %d\n", lp->gid, tw_now(lp), in_msg->type);
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
