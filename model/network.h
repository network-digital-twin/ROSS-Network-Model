#ifndef _network_h
#define _network_h

#include "ross.h"
#include "util/parsers.h"

#define NUM_QOS_LEVEL 3

#define MAX(i, j) (((i) > (j)) ? (i) : (j))
#define MAX_RECORDS 10000 // 10000 records to store statistics in a switch
#define MEAN_TERMINAL_WAIT .005
#define MEAN_SWITCH_PROCESS_WAIT .01
// #define MEAN_SWITCH_PROCESS_WAIT 45.0

#define MSG_PER_TERMINAL 500

// ===================================
// PACKET STRUCTS --------------------
// ===================================

typedef struct {
    unsigned long long pid;  // packet ID
    tw_stime send_time; // Initial time when the packet is generated
    tw_lpid src; // Source switch
    tw_lpid dest; // Final destination switch
    tw_lpid prev_hop;
    tw_lpid next_hop;
    int size_in_bytes;
    int type;  // ToS (type of service)
} packet;

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

// QOS SCHEDULER STATE -------------------
typedef struct {
    int last_priority;
    packet packet;
} sp_scheduler_state;

// QOS SHAPER STATE -------------------
typedef struct {
    long long tokens;
    tw_stime last_update_time;
} token_bucket_state;

// QOS DROPPER STATE -------------------
typedef struct {
    double avg;
    tw_stime q_time;
} REDdropper_state;

// QOS FULL STATE ---------------------
typedef struct {
    int meter_index;
    int shaper_index;
    srTCM_state meter_state;
    sp_scheduler_state scheduler_state;
    token_bucket_state shaper_state;
    REDdropper_state dropper_state;
    double dropper_q_time;  // an extra field to record another q_time // TODO: this is not elegant!
    tw_stime port_available_time_rc; // For reverse computation: the next available time of a port
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
    packet packet;
    qos_state_rc qos_state_snapshot; // Snapshot of QoS modules. Used for reverse computation
} tw_message;


// ===================================
// QOS STRUCTS -----------------------
// ===================================

// QOS QUEUE STRUCTS -----------------------------
typedef struct node_t {
    packet data;
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
    long long capacity; // in bits
    long long tokens; // Current number of tokens. 1 token == 1 bit
    double rate; // unit: tokens per nanosecond (Giga tokens per second)
    double port_bandwidth;  // Gbps. The bandwidth of the port that the bucket is attached to.
    tw_stime last_update_time;
} token_bucket;

typedef struct REDdropper {
    double minth;  // minimum threshold
    double maxth;  // maximum threshold
    double maxp;  // maximum probability
    double wq;  // weights for calculating average queue length
    double avg;  // average queue length
    double q_time; // in ns
    queue_t *queue;  // pointer to the queue that the dropper is attached to
} REDdropper;

// QOS SCHEDULER STRUCTS -----------------------------
typedef struct {
    int num_queues;  // number of QoS queues (size of queue_list)
    int last_priority;  // For reverse computation only; last priority that has been served
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
    double CIR;  // (Unit: Mbps == bits per micro-sec) committed information rate: token generation rate;
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
void queue_take_reverse(queue_t *queue, const packet *pkt);


// QOS SHAPER FUNCTIONS -----------------------------

void token_bucket_init(token_bucket *bucket, long long capacity, double rate, double port_bandwidth);
void token_bucket_consume(token_bucket *bucket, const packet *pkt, tw_stime current_time);
void token_bucket_consume_reverse(token_bucket *bucket, token_bucket_state *bucket_state);
void token_bucket_snapshot(token_bucket *bucket, token_bucket_state *state);
tw_stime token_bucket_next_available_time(token_bucket *bucket, int packet_size);
int token_bucket_ready(token_bucket *shaper, int packet_size);

// QOS SCHEDULER FUNCTIONS -----------------------------

void sp_init(sp_scheduler *scheduler, queue_t *queue_list, int num_queues, token_bucket *shaper);
node_t *sp_update(sp_scheduler* scheduler);
void sp_update_reverse(sp_scheduler* scheduler, const sp_scheduler_state *state);
int sp_has_next(const sp_scheduler *scheduler);
void sp_delta(sp_scheduler *scheduler, const packet *dequeued_pkt, sp_scheduler_state *state);

// QOS METER FUNCTIONS -----------------------------

void srTCM_init(srTCM *meter, const params_srTCM *params);
int srTCM_update(srTCM *meter, const tw_message *msg, tw_stime current_time);
void srTCM_update_reverse(srTCM *meter, const srTCM_state *meter_state);
void srTCM_snapshot(const srTCM *meter, srTCM_state* state);

// QOS DROPPER FUNCTIONS -----------------------------
void REDdropper_init(REDdropper *dropper, double minth, double maxth, double maxp, double wq, queue_t* queue);
int REDdropper_update(REDdropper *dropper, tw_stime current_time);
void REDdropper_snapshot(REDdropper *dropper, REDdropper_state *state);
void REDdropper_update_reverse(REDdropper *dropper, REDdropper_state *state);
void REDdropper_time_update(REDdropper *dropper, tw_stime current_time);

// ===================================
// LP
// ===================================

//LP STATE STRUCTS -----------------------------

typedef struct {
     int num_packets_sent;
     int num_packets_recvd;
     packets pks;
} terminal_state;

typedef struct {
    unsigned long pid;  // packet ID
    double delay; // delay of this packet in ns
    unsigned char drop;  // 1 for dropped, 0 for not dropped
} record;

typedef struct {
    unsigned long long num_packets_dropped;  // the packet dropped in this switch
    unsigned long long num_packets_sent;  // the packet sent from this switch
    unsigned long long received; // the packet received by this switch whose final dest is not this switch
    unsigned long long num_packets_recvd; // the packet whose final dest is this switch
    record *records;
    unsigned long long records_capacity; // the max number of record the `records` can contain.

//    double **total_delay;   // [src switch][priority] in ns
//    double **jitter;  // [src switch][priority]
//    int **count;  // number of received packets [src switch][priority]
} stats;  // stats of the switch

typedef struct {
    int num_qos_levels;
     int num_queues;
     queue_t *qos_queue_list;
     int num_meters;
     srTCM *meter_list;
     int num_schedulers;
     sp_scheduler *scheduler_list;
     int num_shapers;
     token_bucket *shaper_list;
    REDdropper *dropper_list;
     // Ports
     int num_ports;  // number of ports that connect to switches (not to terminals)
     int *port_flags;
     double *bandwidths;  // the bandwidth of each to-switch port (Gbps)
     tw_stime *ports_available_time; // the next available time of each port (due to busy with packet injection)
     tw_stime *propagation_delays;  // the propagation delay of each out port's physical cable
     // Routing table
     route *routing;  // Routing table. The index is the final destination switch's GID/LID
     int routing_table_size; // number of records in the routing table

     config *conf;

    stats *stats;
} switch_state;


//MAPPING -----------------------------
enum lpTypeVals
{
     TERMINAL = 0,
     SWITCH = 1
};
extern tw_lpid lpTypeMapper(tw_lpid gid);
extern tw_peid network_map(tw_lpid gid);
extern tw_peid custom_mapping_lp_to_pe(tw_lpid gid);
extern void custom_mapping_setup();
extern tw_lp *custom_mapping_lpgid_to_local(tw_lpid gid);


// UTILS FUNCTIONS -------------------------------
tw_stime calc_injection_delay(int bytes, double Gbps);
extern void print_message(const tw_message *msg);
extern void print_switch_stats(const switch_state *s, tw_lp *lp);
extern void switch_init_stats(switch_state *s, tw_lp *lp);
extern void switch_free_stats(switch_state *s);
extern void write_switch_stats_to_file(const switch_state *s, tw_lp *lp);
extern void switch_update_stats(stats *st, unsigned long pid, double delay, unsigned char drop);
extern void switch_update_stats_reverse(stats *st);

//DRIVER -----------------------------

extern void terminal_init(terminal_state *s, tw_lp *lp);
extern void terminal_prerun(terminal_state *s, tw_lp *lp);
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
extern char *trace_path;
extern char *route_dir_path;
extern char *home_dir; //used for output directory
extern char *out_dir; //output dir

extern int queue_capacity_0; // in bytes
extern int queue_capacity_1; // in bytes
extern int queue_capacity_2; // in bytes

extern int srTCM_CBS;
extern int srTCM_EBS;

#endif
