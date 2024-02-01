#include <assert.h>
#include "network.h"


// Define LP types for Terminal and Switch
tw_lptype model_lps[] =
{
        {
            (init_f) terminal_init,
            (pre_run_f) terminal_prerun,
            (event_f) terminal_event_handler,
            (revent_f) terminal_RC_event_handler,
            (commit_f) NULL,
            (final_f) terminal_final,
            (map_f) network_map,
            sizeof(terminal_state)
        },
        {
            (init_f) switch_init,
            (pre_run_f) switch_prerun,
            (event_f) switch_event_handler,
            (revent_f) switch_RC_event_handler,
            (commit_f) NULL,
            (final_f) switch_final,
            (map_f) network_map,
            sizeof(switch_state)
            },
        { 0 },
};


//Define command line arguments default values
int total_terminals= 2;
int total_switches = 3;

//Command line opts
const tw_optdef model_opts[] = {
        TWOPT_GROUP("Network Model"),
        TWOPT_UINT("terminals", total_terminals, "Number of terminals in simulation"),
        TWOPT_UINT("switches", total_switches, "Number of switches in simulation"),
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
        printf("Network Model Configuration:\n");
        printf("\t nnodes: %i\n", tw_nnodes());
        printf("\t g_tw_nlp: %llu\n", g_tw_nlp);
        printf("\t custom_LPs_per_pe: %i\n\n", num_LPs_per_pe);

        printf("\t total_terminals: %i\n", total_terminals);
        printf("\t total_switches: %i\n\n", total_switches);

        printf("\tGID:\n");
        for(int i = 0; i < total_switches; i++)
        {
            tw_lpid attached_terminal = get_attached_terminal_LID(i);
            if (attached_terminal == -1) {
                printf("\t%i:   Switch\n", i);
            } else {
                printf("\t%i:   Switch attached with Terminal %llu\n", i, attached_terminal);
            }
        }

        for(int i = 0; i < total_terminals; i++)
        {
            int gid = i + total_switches;
            printf("\t%i:   Terminal %i\n",gid, get_terminal_LID(gid));
        }



        for (int i = 0; i < 30; i++)
        {
            printf("*");
        }
        printf("\n");

    }
}


//for doxygen
#define network_main main
int network_main(int argc, char** argv, char **env)
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

    g_tw_nlp = total_terminals + total_switches;
    num_LPs_per_pe = g_tw_nlp / tw_nnodes();
    g_tw_lookahead = 1;

    // Set up LID-GID mapping
    assert(total_switches == 3);
    int num_switches_t = total_terminals;  // Number of switches that has terminals attached. One switch can only have one attached terminal
    tw_lpid switch_LIDs_t[] = {0, 2};  // The LIDs of the switches that have terminals attached to them
    assert(num_switches_t == sizeof(switch_LIDs_t) / sizeof(switch_LIDs_t[0]));
    tw_lpid *ptr = switch_LIDs_t;
    init_mapping(num_switches_t, ptr);

    displayModelSettings();

    //set up LPs within ROSS
    tw_define_lps(num_LPs_per_pe, sizeof(tw_message));
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
