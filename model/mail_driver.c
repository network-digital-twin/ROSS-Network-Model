/*
mail_driver.c
Mail System Simulator
7-15-2016
Neil McGlohon
*/


//The C driver file for a ROSS model
//This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

//Includes
#include "mail.h"


//--------------Mail box stuff-------------

void mailbox_init (mailbox_state *s, tw_lp *lp)
{
     int self = lp->gid;

     // init state data
     s->num_letters_recvd = -1;

     int i;
     for(i = 0; i < LET_PER_MAILBOX; i++)
     {
          // tw_event *e = tw_event_new(self,tw_rand_exponential(lp->rng, MEAN_MAILBOX_WAIT),lp);
          // tw_stime ts = 1;
          // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_MAILBOX_WAIT) + lookahead;
          tw_stime ts = tw_rand_exponential(lp->rng, MEAN_MAILBOX_WAIT) + 1;
          tw_event *e = tw_event_new(self,ts,lp);
          letter *let = tw_event_data(e);
          let->sender = self;
          let->final_dest = self;
          tw_event_send(e);
     }
}

void mailbox_prerun(mailbox_state *s, tw_lp *lp)
{
     int self = lp->gid;

     tw_lpid assigned_post_office = get_assigned_post_office_LID(lp->gid);

     // printf("%d: I am a mailbox assigned to PO %llu\n",self,assigned_post_office);
}


void mailbox_event_handler(mailbox_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
     int self = lp->gid;
     tw_lpid final_dest;
     tw_lpid next_dest;

     // unsigned int ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
     // dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
     final_dest = tw_rand_integer(lp->rng, 0, total_mailboxes - 1);

     //Next destination from a mailbox is its assigned post office
     tw_lpid assigned_post_office = get_assigned_post_office_LID(lp->gid);
     next_dest = get_post_office_GID(assigned_post_office);

     // // initialize the bit field
     // *(int *) bf = (int) 0;

     s->num_letters_recvd++;

     //schedule a new letter
     // tw_event *e = tw_event_new(self,tw_rand_exponential(lp->rng, MEAN_MAILBOX_WAIT),lp);
     // tw_stime ts = 1;
     // tw_stime ts = tw_rand_exponential(lp->rng, mean) + lookahead + (tw_stime)(lp->gid % (unsigned int)g_tw_ts_end);
     // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_MAILBOX_WAIT) + lookahead;
     tw_stime ts = tw_rand_exponential(lp->rng, MEAN_MAILBOX_WAIT) + 1;
     tw_event *e = tw_event_new(next_dest,ts,lp);
     letter *let = tw_event_data(e);
     let->sender = self;
     let->final_dest = final_dest;
     let->next_dest = next_dest;
     tw_event_send(e);
     s->num_letters_sent++;
}

void mailbox_RC_event_handler(mailbox_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
     tw_rand_reverse_unif(lp->rng);
     tw_rand_reverse_unif(lp->rng);
     s->num_letters_recvd--;
     s->num_letters_sent--;
}

void mailbox_final(mailbox_state *s, tw_lp *lp)
{
     int self = lp->gid;
     printf("Mailbox %d: S:%d R:%d messages\n", self, s->num_letters_sent, s->num_letters_recvd);
}

// void mailbox_commit(state * s, tw_bf * bf, letter * m, tw_lp * lp)
// {
// }

//-------------Post office stuff-------------

void post_office_init (post_office_state *s, tw_lp *lp)
{
     int self = lp->gid;

     // init state data
     s->num_letters_recvd = 0;
}

void post_office_prerun (post_office_state *s, tw_lp *lp)
{
     int self = lp->gid;
     // printf("%d: I am a post office\n",self);
}


void post_office_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
     int self = lp->gid;
     tw_lpid final_dest;
     tw_lpid next_dest;

     s->num_letters_recvd++;


     //Determine: Are you the post office that is to deliver the message or do you need to route it to another one
     int assigned_post_office_gid = get_assigned_post_office_GID(in_msg-> final_dest);
     if(self == assigned_post_office_gid) //You are the post office to deliver the message
     {
          final_dest = in_msg -> final_dest;
          next_dest = in_msg -> final_dest;

          // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_PO_PROCESS_WAIT) + lookahead;
          tw_stime ts = tw_rand_exponential(lp->rng, MEAN_PO_PROCESS_WAIT) + 5;
          tw_event *e = tw_event_new(next_dest,ts,lp);
          letter *let = tw_event_data(e);
          let->sender = self;
          let->final_dest = final_dest;
          let->next_dest = next_dest;
          tw_event_send(e);
          s->num_letters_sent++;

     }
     else //You need to route it to another post offifce
     {
          final_dest = in_msg -> final_dest;
          next_dest = assigned_post_office_gid;

          // tw_stime ts = tw_rand_exponential(lp->rng, MEAN_PO_PROCESS_WAIT) + lookahead;
          tw_stime ts = tw_rand_exponential(lp->rng, MEAN_PO_PROCESS_WAIT) + 5;
          tw_event *e = tw_event_new(next_dest,ts,lp);
          letter *let = tw_event_data(e);
          let->sender = self;
          let->final_dest = final_dest;
          let->next_dest = next_dest;
          tw_event_send(e);
          s->num_letters_sent++;
     }


}

void post_office_RC_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
     tw_rand_reverse_unif(lp->rng);
     s->num_letters_recvd--;
}

void post_office_final(post_office_state *s, tw_lp *lp)
{
     int self = lp->gid;
     printf("Post Office %d: S:%d R:%d messages\n", self,s->num_letters_sent, s->num_letters_recvd);
}
