#ifndef _network_h
#define _network_h

#include "ross.h"
#include "util/parser.h"

#define MEAN_TERMINAL_WAIT .005
#define MEAN_SWITCH_PROCESS_WAIT .01
// #define MEAN_SWITCH_PROCESS_WAIT 45.0

#define MSG_PER_TERMINAL 500

// ===================================
// QOS REVERSE COMPUTATION STRUCTS ---
// ===================================
// Used for storing snapshot of the QoS modules.

// QOS METER STATE -------------------
typedef struct {
    long long T_c;
    long long T_e;
    tw_stime last_update_time;
} srTCM_state;

typedef struct {
    int last_priority;
} sp_scheduler_state;

// QOS SHAPER STATE -------------------
typedef struct {
    long long tokens;
    tw_stime last_update_time;
    tw_stime next_available_time;
} token_bucket_state;

// QOS FULL STATE ---------------------
typedef struct {
    int meter_index;
    int shaper_index;
    srTCM_state meter_state;
    sp_scheduler_state scheduler_state;
    token_bucket_state shaper_state;
} qos_state_rc; // snapshot of full state of QoS modules that are useful for reverse computation

// ===================================
// MESSAGE STRUCTS --------------------
// ===================================

typedef enum {
    KICKOFF,
    ARRIVE,
    SEND,
} message_type;

// It should not contain any pointers, otherwise the operations with qos queues will be affected.
// In distributed mode, having pointers may also cause problem.
typedef struct
{
    message_type type;
    int port_id;   // for SEND event and reverse computations: which output port to use
    tw_lpid sender; // GID
    tw_lpid final_dest_LID; // The LID of the dest terminal
    tw_lpid next_dest_GID; // GID
    int packet_size_in_bytes;  //
    int packet_type;  // ToS (type of service)
    qos_state_rc qos_state_snapshot; // Snapshot of QoS modules. Used for reverse computation
} tw_message;


// ===================================
// QOS STRUCTS -----------------------
// ===================================

// QOS QUEUE STRUCTS -----------------------------
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

// QOS SHAPER STRUCTS -----------------------------
typedef struct token_bucket {
    long long capacity;
    long long tokens;
    long long rate;
    double port_bandwidth;  // Gbps. The bandwidth of the port that the bucket is attached to.
    tw_stime last_update_time;
    tw_stime next_available_time;
} token_bucket;

// QOS SCHEDULER STRUCTS -----------------------------
typedef struct {
    int num_queues;  // number of QoS queues (size of queue_list)
    int last_priority;  // last priority that has been served
    queue_t *queue_list;  // pointer to existing list of queues, index of the queue indicates the priority. 0 is the highest priority.
    token_bucket *shaper;  // pointer to the shaper
} sp_scheduler;

// QOS METER STRUCTS -----------------------------

enum {
    COLOR_GREEN = 0,
    COLOR_YELLOW = 1,
    COLOR_RED = 2
};

typedef struct {
    long long CIR;  // (in bps) committed information rate: token generation rate;
    long long CBS;  // (in bits) committed burst size: capacity of bucket C;
    long long EBS;  // (in bits) excess burst size: capacity of bucket E; can be 0;
    int is_color_aware;  // 1 for color-aware mode, 0 for color-blind mode
} params_srTCM;

typedef struct {
    long long T_c;  // number of tokens in bucket C, initialised to CBS
    long long T_e;  // number of tokens in bucket E, initialised to EBS
    tw_stime last_update_time;  // last time the meter is updated
    params_srTCM params;
} srTCM;



// ===================================
// FUNCTIONS -------------------------
// ===================================

// QOS QUEUE FUNCTIONS -----------------------------

void queue_init(queue_t *queue, int capacity_in_bytes);
void queue_destroy(queue_t *queue);
node_t * queue_put(queue_t *queue, const tw_message *msg);
void queue_put_reverse(queue_t *queue);
node_t *queue_take(queue_t *queue);
void queue_take_reverse(queue_t *queue, const tw_message *msg);


// QOS SHAPER FUNCTIONS -----------------------------

void token_bucket_init(token_bucket *bucket, long long capacity, long long rate, double port_bandwidth);
void token_bucket_consume(token_bucket *bucket, const tw_message *msg, tw_stime current_time);
void token_bucket_consume_reverse(token_bucket *bucket, token_bucket_state *bucket_state);
void token_bucket_snapshot(token_bucket *bucket, token_bucket_state *state);

// QOS SCHEDULER FUNCTIONS -----------------------------

void sp_init(sp_scheduler *scheduler, queue_t *queue_list, int num_queues, token_bucket *shaper);
node_t *sp_update(sp_scheduler* scheduler);
void sp_update_reverse(sp_scheduler* scheduler, const tw_message *msg, sp_scheduler_state *state);
int sp_has_next(const sp_scheduler *scheduler);
void sp_delta(sp_scheduler *scheduler, tw_message *msg, sp_scheduler_state *state);

// QOS METER FUNCTIONS -----------------------------

void srTCM_init(srTCM *meter, const params_srTCM *params);
int srTCM_update(srTCM *meter, const tw_message *msg, tw_stime current_time);
void srTCM_update_reverse(srTCM *meter, const srTCM_state *meter_state);
void srTCM_snapshot(const srTCM *meter, srTCM_state* state);


// UTILS FUNCTIONS -------------------------------
tw_stime calc_injection_delay(int bytes, double GB_p_s);
extern void print_message(const tw_message *msg);


// ===================================
// LP
// ===================================

//LP STATE STRUCTS -----------------------------

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
     // Ports
     int num_ports;  // number of ports that connect to switches (not to terminals)
     int *port_flags;
     double *bandwidths;  // the bandwidth of each to-switch port
     tw_stime *propagation_delays;  // the propagation delay of each out port's physical cable
     // Routing table
     route *routing;  // Routing table. The index is the final destination switch's GID/LID
     int routing_table_size; // number of records in the routing table
} switch_state;


//MAPPING -----------------------------
enum lpTypeVals
{
     TERMINAL = 0,
     SWITCH = 1
};
extern tw_lpid *switch_LID_to_terminal_GID;
extern tw_lpid *terminal_GID_to_switch_LID;
extern void init_mapping(int count, const tw_lpid *switch_LIDs);
extern tw_lpid lpTypeMapper(tw_lpid gid);
extern tw_peid network_map(tw_lpid gid);
extern tw_lpid get_terminal_GID(tw_lpid t_lid);
extern tw_lpid get_terminal_LID(tw_lpid t_gid);
extern tw_lpid get_switch_GID(tw_lpid s_lid);
extern tw_lpid get_assigned_switch_GID(tw_lpid t_lid);
extern tw_lpid get_attached_terminal_LID(tw_lpid s_lid);
extern tw_lpid get_attached_terminal_GID(tw_lpid s_lid);

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
