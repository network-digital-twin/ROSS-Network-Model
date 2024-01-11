/*
mail_main.c
Mail System Simulator
7-15-2016
Neil McGlohon
*/

//includes
#include "mail.h"


// Define LP types for Mailbox and Post Office
tw_lptype model_lps[] =
{
     {
          (init_f) mailbox_init,
          (pre_run_f) mailbox_prerun,
          (event_f) mailbox_event_handler,
          (revent_f) mailbox_RC_event_handler,
          (commit_f) NULL,
          (final_f) mailbox_final,
          (map_f) mail_map,
          sizeof(mailbox_state)
     },
     {
          (init_f) post_office_init,
          (pre_run_f) post_office_prerun,
          (event_f) post_office_event_handler,
          (revent_f) post_office_RC_event_handler,
          (commit_f) NULL,
          (final_f) post_office_final,
          (map_f) mail_map,
          sizeof(post_office_state)
     },
     { 0 },
};


//Define command line arguments default values
int total_mailboxes= 1;
int total_post_offices = 1;

//Command line opts
const tw_optdef model_opts[] = {
     TWOPT_GROUP("Mail Model"),
     TWOPT_UINT("mailboxes", total_mailboxes, "Number of mailboxes in simulation"),
     TWOPT_UINT("postoffices", total_post_offices, "Number of post offices in simulation"),
     TWOPT_END()
};


//Displays simple settings of the simulation
void displayModelSettings()
{
     if(g_tw_mynode ==0)
     {
          for (int i = 0; i < 30; i++)
          {
               printf("*");
          }
          printf("\n");
          printf("Mail Model Configuration:\n");
          printf("\t nnodes: %i\n", tw_nnodes());
          printf("\t g_tw_nlp: %llu\n", g_tw_nlp);
          printf("\t custom_LPs_per_pe: %i\n\n", num_LPs_per_pe);

          printf("\t total_mailboxes: %i\n", total_mailboxes);
          printf("\t total_post_offices: %i\n\n", total_post_offices);

          printf("\tGID:\n");
          for(int i = 0; i < total_mailboxes; i++)
          {
               tw_lpid assigned_post_office = get_assigned_post_office_LID(i);

               printf("\t%i:   Mailbox assigned to PO %llu\n",i,assigned_post_office);
          }

          for(int i = 0; i < total_post_offices; i++)
          {
               int gid = i + total_mailboxes;
               printf("\t%i:   Post Office %i\n",gid,i);
          }



          for (int i = 0; i < 30; i++)
          {
               printf("*");
          }
          printf("\n");

     }
}


//for doxygen
#define mail_main main
int mail_main(int argc, char** argv, char **env)
{

    tw_opt_add(model_opts);
    tw_init(&argc, &argv);


    //Useful ROSS variables and functions
    // tw_nnodes() : number of nodes/processors defined
    // g_tw_mynode : my node/processor id (mpi rank)

    //Custom Mapping
    /*
    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &model_custom_mapping;
    g_tw_custom_lp_global_to_local_map = &model_mapping_to_lp;
    */

    //Useful ROSS variables (set from command line)
    // g_tw_events_per_pe
    // g_tw_lookahead
    // g_tw_nlp
    // g_tw_nkp
    // g_tw_synchronization_protocol

    g_tw_nlp = total_mailboxes + total_post_offices;
    num_LPs_per_pe = g_tw_nlp / tw_nnodes();
    g_tw_lookahead = 1;

    displayModelSettings();

    //set up LPs within ROSS
    tw_define_lps(num_LPs_per_pe, sizeof(letter));
    // note that g_tw_nlp gets set here by tw_define_lps

    // IF there are multiple LP types
    //    you should define the mapping of GID -> lptype index
    g_tw_lp_typemap = lpTypeMapper;

    // set the global variable and initialize each LP's type
    g_tw_lp_types = model_lps;
    tw_lp_setup_types();


    tw_run();
    tw_end();

    return 0;
}
