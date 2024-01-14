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
#include "qos/queue_fifo.h"
#include <string.h>

#define NUM_QOS_LEVEL 3
#define QOS_QUEUE_CAPACITY 65536 // 64KB


//-------------Post office stuff-------------

void qos_enqueue(post_office_state *s, const letter *in_msg);

void post_office_init (post_office_state *s, tw_lp *lp)
{
     int self = lp->gid;

     // init state data
     s->num_letters_recvd = 0;
     s->qos_queue_list = malloc(sizeof(queue_t) * NUM_QOS_LEVEL);
     for(int i = 0; i < NUM_QOS_LEVEL; i++)
     {
         s->qos_queue_list[i] = *queue_init(QOS_QUEUE_CAPACITY);
     }
     // TODO: CHECK HERE IF THE MEMORY ALLOCATION IS CORRECT!
}

void post_office_prerun (post_office_state *s, tw_lp *lp)
{
     int self = lp->gid;
     // printf("%d: I am a post office\n",self);
}

void handle_arrive_event(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_stime ts  = 1;

    // modify the state here
    s->num_letters_recvd++;

    //qos_enqueue(s, in_msg);


    // Schedule a SEND event to myself
    tw_event *e = tw_event_new(self,ts,lp);
    letter *let = tw_event_data(e);
    memcpy(let, in_msg, sizeof(letter));
    let->type = SEND;
    tw_event_send(e);

}

void qos_enqueue(post_office_state *s, const letter *in_msg)
{
    int qos_level = in_msg->packet_type;
    letter *let = malloc(sizeof(letter));
    memcpy(let, in_msg, sizeof(letter));
    queue_t *q = &s->qos_queue_list[qos_level];
    int ret = queue_put(q, *let);
    if(ret == -1)
    {
        printf("ERROR: QOS queue is full\n");
        return;
    }

}

void handle_send_event(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
    tw_lpid self = lp->gid;
    tw_lpid final_dest;
    tw_lpid next_dest;

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

void post_office_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{

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



void post_office_RC_event_handler(post_office_state *s, tw_bf *bf, letter *in_msg, tw_lp *lp)
{
    switch (in_msg->type) {
        case ARRIVE :
        {
            s->num_letters_recvd--;
            break;
        }
        case SEND :
        {
            tw_rand_reverse_unif(lp->rng);
            s->num_letters_sent--;
            break;
        }
        default :
        {
            printf("ERROR: Invalid message type\n");
            return;
        }
    }

}

void post_office_final(post_office_state *s, tw_lp *lp)
{
     int self = lp->gid;
//    for(int i = 0; i < NUM_QOS_LEVEL; i++)
//    {
//        queue_destroy(&s->qos_queue_list[i]);
//    }
//    free(s->qos_queue_list);
     printf("Post Office %d: S:%d R:%d messages\n", self,s->num_letters_sent, s->num_letters_recvd);
}
