#include "network.h"
#include "util/ip.h"

static int mean_wait_time; // in nanosecond, this is the mean inter-arrival time of two packets (100Gbps; 1400B per packet)
#define PACKET_SIZE 1400

char* generate_random_ip(tw_rng_stream *rng, const char* ip_net, int netmask);

// Kickoff initialisation
void kickoff(switch_state * s, tw_lp * lp) {
    tw_lpid self = lp->gid;
    s->dest_ip = NULL;
    printf("INFO: Generating traffic for lp %lu switch %d\n", self, s->conf->id);

    if (strcmp(s->conf->type, "access") == 0) {
        // First randomly pick a final destination based on the routing data
        // Randomly pick an output port
        if(s->conf->num_topos == 0) {
            return;
        }
        int topo_id = tw_rand_integer(lp->rng, 0, s->conf->num_topos - 1); 
        const char *srcPortName_ptr = s->conf->topos[topo_id].srcPort;
        
        // Randomly pick a route that has the same srcPort as the selected srcPort, then use that final destination
        char *destNetIP = NULL;
        int mask = 0;
        for(int i=0; i<500; i++) {
            int route_id = tw_rand_integer(lp->rng, 0, s->conf->num_routes - 1);
            if(strcmp(srcPortName_ptr, s->conf->routes[route_id].srcPort) == 0) {
                if(s->conf->routes[route_id].nextHopID != s->conf->id && get_port_for_next_hop(s->conf, s->conf->routes[route_id].netIP)) { // make sure the next hop is not the current switch
                    destNetIP = s->conf->routes[route_id].netIP;
                    mask = s->conf->routes[route_id].mask;
                    break;
                }
            }
        }
        if(destNetIP == NULL) {
            printf("WARNING: No route found for switch to generate traffic: lp %lu - switch %d\n", self, s->conf->id);
            return;
        }

        // Select a random destination ip
        char *dest_ip = NULL;
        for(int i = 0; i < 5; i++) {
            dest_ip = generate_random_ip(lp->rng, destNetIP, mask);
            if(get_port_for_next_hop(s->conf, dest_ip)) {
                break;
            }
        }
        if(dest_ip == NULL) {
            printf("WARNING: No destination IP found for switch to generate traffic: lp %lu - switch %d\n", self, s->conf->id);
            return;
        }

        s->dest_ip = dest_ip;
        printf("switchID %d, lpid %lu, destNet %s, mask %d, random dest %s\n", s->conf->id, lp->gid, destNetIP, mask, s->dest_ip);
        
        const port *port = get_port_for_next_hop(s->conf, s->dest_ip); 
        mean_wait_time = s->traffic_gen_load * PACKET_SIZE * 8 / (port->bw / 1000.0 / 1000.0 / 1000.0); // bps
        printf("port->bw %lu, mean_wait_time %d\n",port->bw, mean_wait_time);

        tw_stime ts = tw_rand_exponential(lp->rng, mean_wait_time) + 1;
        tw_event *e = tw_event_new(self, ts, lp);
        tw_message *kickoff_msg = tw_event_data(e);
        kickoff_msg->type = KICKOFF;
        kickoff_msg->packet.pid = 0;
        tw_event_send(e);

        
    }
}


// Schedule an ARRIVE event and a new KICKOFF event
void handle_kickoff_event(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    tw_lpid self = lp->gid;
    tw_stime ts_now = tw_now(lp);
    tw_stime ts = 1;
    int priority = tw_rand_integer(lp->rng, 0, 2);

    // Generate an ARRIVE event to myself
    tw_event *e = tw_event_new(self, ts, lp);
    tw_message *out_msg =tw_event_data (e);
    out_msg->packet.pid = in_msg->packet.pid;
    out_msg->packet.send_time = ts_now + ts;
    out_msg->packet.src = self;
    out_msg->packet.dest = -1;
    strncpy(out_msg->packet.destIP, s->dest_ip, sizeof(out_msg->packet.destIP)-1);
    out_msg->packet.destIP[sizeof(out_msg->packet.destIP)-1] = '\0';
    //printf("%s switch id %d\n", out_msg->packet.destIP, s->conf->id);
    out_msg->packet.prev_hop = -1;
    out_msg->packet.next_hop = -1;
    out_msg->packet.type = priority;
    out_msg->packet.size_in_bytes = PACKET_SIZE;
    out_msg->packet.TTL = 64;
    out_msg->type = ARRIVE;
    out_msg->port_id = -1;  // this variable is of no use here, so set it to -1.
    tw_event_send(e);

    // Generate a new KICKOFF event to myself
    ts = mean_wait_time;
    tw_event *e_kick = tw_event_new(self, ts, lp);
    tw_message *kickoff_msg = tw_event_data(e_kick);
    kickoff_msg->type = KICKOFF;
    kickoff_msg->packet.pid = in_msg->packet.pid + 1;
    tw_event_send(e_kick);
#ifdef DEBUG
    if(self==PROBE_SWITCH_ID) {
        printf("[%llu]interval: %f\n", self, ts);
    }
#endif

}

void handle_kickoff_event_rc(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp) {
    tw_rand_reverse_unif(lp->rng);
}


// Function to generate a random IP address in the given network
char* generate_random_ip(tw_rng_stream *rng, const char* ip_net, int netmask) {
    unsigned int ip = ip_to_int(ip_net);
    unsigned int mask = ~((1 << (32 - netmask)) - 1);
    unsigned int net = ip & mask;
    unsigned int host_range = ~mask;
    
    // Generate a random host address
    unsigned int rand_host;
    if(host_range == 0) {
        rand_host = 0;
    } else {
        rand_host = tw_rand_integer(rng, 0, host_range);
    }
    
    // Combine network address and random host address
    unsigned int rand_ip = net + rand_host;
    
    // Convert the generated IP address to string
    char* rand_ip_str = malloc(16);
    int_to_ip(rand_ip, rand_ip_str);
    
    return rand_ip_str;
}

