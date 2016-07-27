/*
mail.h
Mail System Simulator
7-15-2016
Neil McGlohon
*/


#ifndef _mail_h
#define _mail_h

#include "ross.h"

#define MEAN_MAILBOX_WAIT 10.0
// #define MEAN_PO_PROCESS_WAIT 45.0

#define LET_PER_MAILBOX 1


//STRUCTS ------------------------------

typedef struct
{
     tw_lpid sender;
     tw_lpid final_dest;
     tw_lpid next_dest;
} letter;


typedef struct
{
     int num_letters_sent;
     int num_letters_recvd;
     //TODO consider an inbox so that there could maybe be ad-hoc p2p messaging, interesting model
} mailbox_state;

typedef struct
{
     int num_letters_sent;
     int num_letters_recvd;
     //TODO consider an inbox so that there could maybe be ad-hoc p2p messaging, interesting model
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




//DRIVER STUFF -----------------------------

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




//MAIN STUFF-----------------------------


extern unsigned int setting_1;


extern tw_lptype model_lps[];


// tw_stime lookahead = 10.0;
// unsigned int ttl_lps = 0;
// unsigned int offset_lpid = 0;
// unsigned int nlp_per_pe = 1;
// unsigned int custom_LPs_per_pe = 1;

tw_stime lookahead;
unsigned int nlp_per_pe;
unsigned int custom_LPs_per_pe;

int total_mailboxes;
int total_post_offices;

// static tw_stime mean = 1.0;

// static char run_id[1024] = "undefined";





#endif
