#ifndef _mail_h
#define _mail_h

#include "ross.h"

#define MEAN_MAILBOX_WAIT .005
#define MEAN_PO_PROCESS_WAIT .01
// #define MEAN_PO_PROCESS_WAIT 45.0

#define LET_PER_MAILBOX 1


//MESSAGE STRUCTS ------------------------------

typedef enum {
    KICKOFF,
    ARRIVE,
    SEND,
} message_type;

typedef struct
{
    tw_lpid sender;
    tw_lpid final_dest;
    tw_lpid next_dest;
    int packet_size;
    int packet_type;  // ToS (type of service)
    message_type type;
} letter; // It should not contain any pointers, otherwise the operations with queue will be affected.


//QUEUE -----------------------------


typedef struct node_t {
    letter data;
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
void queue_put(queue_t *queue, const letter *msg);
void queue_put_reverse(queue_t *queue);
node_t *queue_take(queue_t *queue);
void queue_take_reverse(queue_t *queue, const letter *msg);


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
void token_bucket_consume(token_bucket *bucket, const letter *msg, tw_stime current_time);
int token_bucket_update_reverse(token_bucket *bucket, letter *msg);


//SCHEDULER -----------------------------

typedef struct {
    int num_queues;  // number of QoS queues (size of queue_list)
    int last_priority;  // last priority that has been served
    queue_t *queue_list;  // pointer to existing list of queues, index of the queue indicates the priority. 0 is the highest priority.
    token_bucket *shaper;  // pointer to the shaper
} sp_scheduler;

void sp_init(sp_scheduler *scheduler, queue_t *queue_list, int num_queues, token_bucket *shaper);
node_t *sp_update(sp_scheduler* scheduler);
void sp_update_reverse(sp_scheduler* scheduler, const letter *msg, int priority);


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
int srTCM_update(srTCM *meter, const letter *msg, tw_stime current_time);
void srTCM_update_reverse(srTCM *meter, const srTCM_state *meter_state);
void *srTCM_snapshot(const srTCM *meter, srTCM_state* state);


//STATE STRUCTS -----------------------------

typedef struct {
     int num_letters_sent;
     int num_letters_recvd;
} mailbox_state;

typedef struct {
     int num_letters_sent;
     int num_letters_recvd;
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
} post_office_state;


//MAPPING -----------------------------
enum lpTypeVals
{
     MAILBOX = 0,
     POSTOFFICE = 1
};

extern tw_lpid lpTypeMapper(tw_lpid gid);
extern tw_peid mail_map(tw_lpid gid);
extern int get_mailbox_GID(int lpid);
extern int get_post_office_GID(int lpid);
extern int get_assigned_post_office_LID(int lpid);
extern int get_assigned_post_office_GID(int lpid);

//DRIVER -----------------------------

extern void mailbox_init(mailbox_state *s, tw_lp *lp);
extern void mailbox_prerun(mailbox_state *s, tw_lp *lp);
extern void mailbox_event_handler(mailbox_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void mailbox_RC_event_handler(mailbox_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void mailbox_final(mailbox_state *s, tw_lp *lp);
extern void mailbox_commit(mailbox_state *s, tw_bf *bf, letter *m, tw_lp *lp);


extern void post_office_init(post_office_state *s, tw_lp *lp);
extern void post_office_prerun(post_office_state *s, tw_lp *lp);
extern void post_office_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void post_office_RC_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void post_office_final(post_office_state *s, tw_lp *lp);
extern void post_office_commit(post_office_state *s, tw_bf *bf, letter *m, tw_lp *lp);

//MAIN -----------------------------

extern tw_lptype model_lps[];

tw_stime lookahead;
unsigned int nlp_per_pe;
unsigned int num_LPs_per_pe;

int total_mailboxes;
int total_post_offices;


#endif
