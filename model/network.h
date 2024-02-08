/*
network.h
Network System Simulator
*/

#ifndef _network_h
#define _network_h

#include "ross.h"
#include "parsers.h"

#define MEAN_TERMINAL_WAIT .005
#define MEAN_SWITCH_PROCESS_WAIT .01
// #define MEAN_SWITCH_PROCESS_WAIT 45.0

#define MSG_PER_TERMINAL 1

// STRUCTS ------------------------------

typedef enum
{
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
     int packet_type;
     message_type type;
} message;

typedef struct
{
     packets pks;

     int num_packets_sent;
     int num_packets_recvd;
     // TODO consider an inbox so that there could maybe be ad-hoc p2p messaging, interesting model
} terminal_state;

typedef struct
{
     config *conf;

     int num_packets_sent;
     int num_packets_recvd;
     // TODO consider an inbox so that there could maybe be ad-hoc p2p messaging, interesting model
} switch_state;

// MAPPING -----------------------------
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

// DRIVER STUFF -----------------------------

extern void terminal_init(terminal_state *s, tw_lp *lp);
extern void terminal_prerun(terminal_state *s, tw_lp *lp);
extern void terminal_event_handler(terminal_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void terminal_RC_event_handler(terminal_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void terminal_final(terminal_state *s, tw_lp *lp);
extern void terminal_commit(terminal_state *s, tw_bf *bf, message *m, tw_lp *lp);

extern void switch_init(switch_state *s, tw_lp *lp);
extern void switch_prerun(switch_state *s, tw_lp *lp);
extern void switch_event_handler(switch_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void switch_RC_event_handler(switch_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void switch_final(switch_state *s, tw_lp *lp);
extern void switch_commit(switch_state *s, tw_bf *bf, message *m, tw_lp *lp);

// MAIN STUFF-----------------------------

extern tw_lptype model_lps[];

extern tw_stime lookahead;
extern unsigned int nlp_per_pe;
extern unsigned int num_LPs_per_pe;

extern int total_terminals;
extern int total_switches;

#endif
