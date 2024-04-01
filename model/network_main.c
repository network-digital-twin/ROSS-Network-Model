#include <assert.h>
#include "network.h"
#include <time.h>
#include <sys/stat.h>
//#define DEBUG

// Define LP types for Terminal and Switch
tw_lptype model_lps[] =
{

        {
            (init_f) terminal_init,
            (pre_run_f) terminal_prerun,
            (event_f) NULL,
            (revent_f) NULL,
            (commit_f) NULL,
            (final_f) terminal_final,
            (map_f) custom_mapping_lp_to_pe,
            sizeof(terminal_state)
        },
        {
            (init_f) switch_init,
            (pre_run_f) switch_prerun,
            (event_f) switch_event_handler,
            (revent_f) switch_RC_event_handler,
            (commit_f) NULL,
            (final_f) switch_final,
            (map_f) custom_mapping_lp_to_pe,
            sizeof(switch_state)
            },
        { 0 },
};


//Define command line arguments default values

tw_lpid total_terminals= 1;
tw_lpid total_switches = 5237;

char home_path[1024] = "/home/nan42/codes-dev/ROSS-Network-Model";
char partition_path[1024] = "/home/nan42/codes-dev/ROSS-Network-Model/partition/graph-for-metis-w-size.txt.part.8";
char trace_file[1024] = "/home/nan42/codes-dev/ROSS-Network-Model/WL_generation/traces/lightweight/1000ms/trace_0_FLOW_THROUGHPUT-1250000__SIMULATION_TIME-1000000000__PAIRS_PER_SRC-1-0__MSG_SIZE-10000__PACKET_SIZE-1400__BANDWIDTH-1250000__PRIO_LEVELS-3";
char route_path[1024] = "/home/nan42/codes-dev/ROSS-Network-Model/WL_generation/topologies/final_topology_0";

char *trace_path = trace_file;
char *route_dir_path = route_path;
char *home_dir = home_path;
char *partition_file = partition_path;
char *out_dir = NULL;

int queue_capacity_0 = 5000000; // 5MB: ~3571 packets
int queue_capacity_1 = 20000000; // 20MB: ~14285 packets
int queue_capacity_2 = 20000000; // 20MB: ~14285 packets

uint32_t srTCM_CBS = 1400*50*8;
uint32_t srTCM_EBS = 1400*500*8;

tw_stime propagation_delay = 4000000; // 4000000ns = 4ms


//Command line opts
const tw_optdef model_opts[] = {
        TWOPT_GROUP("Network Model"),
        TWOPT_UINT("switches", total_switches, "Number of switches in simulation"),
        TWOPT_CHAR("trace-path", trace_file, "Path to the trace file"),
        TWOPT_CHAR("model-home", home_path, "Path to the home directory of the model, used for determine output paths"),
        TWOPT_CHAR("partition-path", partition_path, "Path to the partition file"),
        TWOPT_CHAR("route-path", route_path, "Path to the routing directory"),
        TWOPT_STIME("propagation-delay", propagation_delay, "Propagation delay in ns"),
        TWOPT_UINT("queue-capacity-0", queue_capacity_0, "Queue capacity (bytes) for priority 0"),
        TWOPT_UINT("queue-capacity-1", queue_capacity_1, "Queue capacity (bytes) for priority 1"),
        TWOPT_UINT("queue-capacity-2", queue_capacity_2, "Queue capacity (bytes) for priority 2"),
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

        printf("\t total_terminals: %llu\n", total_terminals);
        printf("\t total_switches: %llu\n\n", total_switches);


        for (int i = 0; i < 30; i++)
        {
            printf("*");
        }
        printf("\n");

    }
}

// 0 for success, -1 for failure
int create_out_dir() {
    // get current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char str_time[25];
    sprintf(str_time, "%d-%02d-%02d_%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    char str[100];
    // create the directories
    strcpy(str, home_dir);
    strcat(str, "/");
    strcat(str, "output");
    if(g_tw_mynode == 0){
        mkdir(str, 0777);
    }

    strcat(str, "/");
    strcat(str, str_time);
    puts(str);

    out_dir = (char *)malloc(strlen(str) + 1);
    strcpy(out_dir, str);

    int result = 0;
    if(g_tw_mynode == 0) {
        result = mkdir(str, 0777);
        strcat(str, "/");
        strcat(str, "raw");
        result += mkdir(str, 0777);
    }
    return result;
}

//for doxygen
#define network_main main
int network_main(int argc, char** argv, char **env)
{
    // Re-set parameters here
    g_tw_ts_end = 1000.0*1000.0*1000.0;  // simulation end time

    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    if(tw_nnodes() > 1) {
	    printf("%s\n",partition_file);
    }
#ifdef DEBUG
    // debug mode, need to attach a debugger after run
    // See: https://www.jetbrains.com/help/clion/openmpi.html#debug-procedure
    if(tw_nnodes() > 1)
    {
        int i = 0;
        while (!i)
            sleep(5);
    }
#endif
    g_tw_events_per_pe = 10000000;

    //Useful ROSS variables and functions
    // tw_nnodes() : number of nodes/processors defined
    // g_tw_mynode : my node/processor id (mpi rank)

    //Custom Mapping

    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &custom_mapping_setup;
    g_tw_custom_lp_global_to_local_map = &custom_mapping_lpgid_to_local;;

    //Useful ROSS variables (set from command line)
    // g_tw_events_per_pe
    // g_tw_lookahead
    // g_tw_nlp
    // g_tw_nkp
    // g_tw_synchronization_protocol

    tw_lpid total_lps = total_terminals + total_switches;

    // figure out how many LPs are on this PE

    init_partition(partition_file, total_lps);
    num_LPs_per_pe = pe_to_num_lps[g_tw_mynode];


    g_tw_lookahead = 1;

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

    printf("simulation end time: %f\n", g_tw_ts_end);

    // prepare output directory
    int result = create_out_dir();
    assert(result == 0);

    tw_run();
    tw_end();

    return 0;
}
