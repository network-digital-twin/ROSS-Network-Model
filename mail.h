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

#define LET_PER_MAILBOX 10

// static int letters_per_mailbox = 10;

// typedef struct mailbox_state mailbox_state;
// typedef struct post_office_state post_office_state;


typedef struct
{
  tw_lpid sender;
  tw_lpid final_dest;
  tw_lpid next_dest;
} letter;


typedef struct
{
  int num_letters_recvd;
  //TODO consider an inbox so that there could maybe be ad-hoc p2p messaging, interesting model
} mailbox_state;

typedef struct
{
  int num_letters_recvd;
  //TODO consider an inbox so that there could maybe be ad-hoc p2p messaging, interesting model
} post_office_state;

// struct post_office_state
// {
//   struct letter po_letter_storage[];
//   int num_letters_processed;
// }

extern unsigned int setting_1;


extern tw_lptype model_lps[];


extern void mailbox_init(mailbox_state *s, tw_lp *lp);
extern void mailbox_event_handler(mailbox_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void mailbox_RC_event_handler(mailbox_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void mailbox_final(mailbox_state *s, tw_lp *lp);
extern void mailbox_commit(mailbox_state *s, tw_bf *bf, letter *m, tw_lp *lp);


extern void post_office_init(post_office_state *s, tw_lp *lp);
extern void post_office_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void post_office_RC_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp);
extern void post_office_final(post_office_state *s, tw_lp *lp);
extern void post_office_commit(post_office_state *s, tw_bf *bf, letter *m, tw_lp *lp);



extern tw_peid mail_map(tw_lpid gid);
extern int get_mailbox_GID(tw_lpid lpid);
extern int get_post_office_GID(tw_lpid lpid);
extern int get_assigned_post_office_LID(tw_lpid lpid);


static tw_stime lookahead = 10.0;
// static unsigned int stagger = 0;
// static unsigned int offset_lpid = 0;
// static tw_stime mult = 1.4;
// static int g_phold_start_events = 1;
// static int optimistic_memory = 100;
// static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 1;

static int total_mailboxes = 10;
static int total_post_offices = 3;

// static tw_stime mean = 1.0;

// static char run_id[1024] = "undefined";





#endif
