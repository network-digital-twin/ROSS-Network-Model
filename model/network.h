#ifndef _network_h
#define _network_h

#include "ross.h"

#define MEAN_TERMINAL_WAIT .005
#define MEAN_SWITCH_PROCESS_WAIT .01
// #define MEAN_SWITCH_PROCESS_WAIT 45.0

#define MSG_PER_TERMINAL 1


//MESSAGE STRUCTS ------------------------------

typedef enum {
    KICKOFF,
    ARRIVE,
    SEND,
} message_type;

typedef struct
{
    tw_lpid sender;
    tw_lpid final_dest; // The GID of a terminal
    tw_lpid next_dest;
    int packet_size;
    int packet_type;  // ToS (type of service)
    message_type type;
    int port_id;   // for SEND event, which output port to use
} tw_message; // It should not contain any pointers, otherwise the operations with queue will be affected.


//QUEUE -----------------------------


typedef struct node_t {
    tw_message data;
    struct node_t *next;
    struct node_t *prev;
} node_t;

typedef struct {
    int num_packets;
    int size_in_bytes;  // total size of all packets
    int max_size_in_bytes;  // capacity of the queue
    node_t *head;
    node_t *tail;
} queue_t;

void queue_init(queue_t *queue, int capacity_in_bytes);
void queue_destroy(queue_t *queue);
void queue_put(queue_t *queue, const tw_message *msg);
void queue_put_reverse(queue_t *queue);
node_t *queue_take(queue_t *queue);
void queue_take_reverse(queue_t *queue, const tw_message *msg);


//SHAPER -----------------------------

typedef struct token_bucket {
    int capacity;
    int tokens;
    double rate;
    double port_bandwidth;  // The bandwidth of the port that the bucket is attached to, in bps.
    tw_stime last_update_time;
    tw_stime next_available_time;
} token_bucket;

void token_bucket_init(token_bucket *bucket, int capacity, double rate, double port_bandwidth);
void token_bucket_consume(token_bucket *bucket, const tw_message *msg, tw_stime current_time);
int token_bucket_update_reverse(token_bucket *bucket, tw_message *msg);


//SCHEDULER -----------------------------

typedef struct {
    int num_queues;  // number of QoS queues (size of queue_list)
    int last_priority;  // last priority that has been served
    queue_t *queue_list;  // pointer to existing list of queues, index of the queue indicates the priority. 0 is the highest priority.
    token_bucket *shaper;  // pointer to the shaper
} sp_scheduler;

void sp_init(sp_scheduler *scheduler, queue_t *queue_list, int num_queues, token_bucket *shaper);
node_t *sp_update(sp_scheduler* scheduler);
void sp_update_reverse(sp_scheduler* scheduler, const tw_message *msg, int priority);
int sp_has_next(const sp_scheduler *scheduler);


//METER -----------------------------

enum {
    COLOR_GREEN = 0,
    COLOR_YELLOW = 1,
    COLOR_RED = 2
};

typedef struct {
    int CIR;  // committed information rate: token generation rate
    int CBS;  // committed burst size: capacity of bucket C
    int EBS;  // excess burst size: capacity of bucket E; can be 0
    int is_color_aware;  // 1 for color-aware mode, 0 for color-blind mode
} params_srTCM;

typedef struct {
    int T_c;  // number of tokens in bucket C, initialised to CBS
    int T_e;  // number of tokens in bucket E, initialised to EBS
    tw_stime last_update_time;  // last time the meter is updated
    params_srTCM params;
} srTCM;

typedef struct {
    int T_c;
    int T_e;
    tw_stime last_update_time;
} srTCM_state; // used for snapshot only

void srTCM_init(srTCM *meter, const params_srTCM *params);
int srTCM_update(srTCM *meter, const tw_message *msg, tw_stime current_time);
void srTCM_update_reverse(srTCM *meter, const srTCM_state *meter_state);
void srTCM_snapshot(const srTCM *meter, srTCM_state* state);


//STATE STRUCTS -----------------------------

typedef struct {
     int num_packets_sent;
     int num_packets_recvd;
} terminal_state;

typedef struct {
     int num_packets_sent;
     int num_packets_recvd;
     int num_queues;
     queue_t *qos_queue_list;
     int num_meters;
     srTCM *meter_list;
     int num_schedulers;
     sp_scheduler *scheduler_list;
     int num_shapers;
     token_bucket *shaper_list;
     int num_out_ports;
     int *out_port_flags;
     double *bandwidths;  // the bandwidth of each out port
     double *propagation_delays;  // the propagation delay of each out port's physical cable
} switch_state;


//MAPPING -----------------------------
enum lpTypeVals
{
     TERMINAL = 0,
     SWITCH = 1
};

extern tw_lpid lpTypeMapper(tw_lpid gid);
extern tw_peid network_map(tw_lpid gid);
extern int get_terminal_GID(int lpid);
extern int get_switch_GID(int lpid);
extern int get_assigned_switch_LID(int lpid);
extern int get_assigned_switch_GID(int lpid);

//DRIVER -----------------------------

extern void terminal_init(terminal_state *s, tw_lp *lp);
extern void terminal_prerun(terminal_state *s, tw_lp *lp);
extern void terminal_event_handler(terminal_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp);
extern void terminal_RC_event_handler(terminal_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp);
extern void terminal_final(terminal_state *s, tw_lp *lp);
extern void terminal_commit(terminal_state *s, tw_bf *bf, tw_message *m, tw_lp *lp);


extern void switch_init(switch_state *s, tw_lp *lp);
extern void switch_prerun(switch_state *s, tw_lp *lp);
extern void switch_event_handler(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp);
extern void switch_RC_event_handler(switch_state *s, tw_bf *bf, tw_message *in_msg, tw_lp *lp);
extern void switch_final(switch_state *s, tw_lp *lp);
extern void switch_commit(switch_state *s, tw_bf *bf, tw_message *m, tw_lp *lp);

//MAIN -----------------------------

extern tw_lptype model_lps[];

tw_stime lookahead;
unsigned int nlp_per_pe;
unsigned int num_LPs_per_pe;

int total_terminals;
int total_switches;


#endif
