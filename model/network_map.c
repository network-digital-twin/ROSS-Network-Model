#include "network.h"



int get_terminal_GID(int lpid)
{
     return lpid;
}

int get_switch_GID(int lpid)
{
     return total_terminals + lpid;
}

int get_assigned_switch_LID(int lpid)
{
     return lpid % total_switches;
}

int get_assigned_switch_GID(int lpid)
{
     return get_switch_GID(get_assigned_switch_LID(lpid));
}


tw_lpid lpTypeMapper(tw_lpid gid)
{
     // printf("%llu\n",gid);
     if (gid < total_terminals)
     {
          // printf("%llu < %i\n",gid,total_terminals);
          return TERMINAL;
     }
     else
     {
          // printf("%llu >= %i\n",gid,total_terminals);
          return SWITCH;
     }

}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid network_map(tw_lpid gid)
{
     return (tw_peid) gid / g_tw_nlp;
}


tw_peid custom_mapping_lp_to_pe(tw_lpid gid) {
    return gid % tw_nnodes();
}

//Map LP to the local ID on the PE
tw_lp * custom_mapping_lpgid_to_local(tw_lpid gid) {
    tw_lpid local_index = gid / tw_nnodes();
    return g_tw_lp[local_index];
}


void custom_mapping_setup(void) {
    tw_kpid kpid;
    int total_lps = total_terminals + total_switches;
    tw_lpid nkp_per_pe = g_tw_nkp;

    // By default 16 kps per pe
    for(kpid = 0; kpid < nkp_per_pe; kpid++) {
        tw_kp_onpe(kpid, g_tw_pe);
    }

    // figure out how many LPs are on this PE
    int min_num_lps_per_pe = floor(total_lps/tw_nnodes());
    int pes_with_extra_lp = total_lps - (min_num_lps_per_pe * tw_nnodes());
    int lps_per_pe = min_num_lps_per_pe;
    if (g_tw_mynode < pes_with_extra_lp) {
        lps_per_pe += 1;
    }

    printf("Node %ld: g_tw_nlp %llu, g_tw_nkp %lu, lps_per_pe %d\n", g_tw_mynode, g_tw_nlp, g_tw_nkp, lps_per_pe);


    // set up the LPs
    for (int lp_index = 0; lp_index < lps_per_pe; lp_index++) {
        // calculate LP's GID
        tw_lpid lp_gid = g_tw_mynode + (lp_index * tw_nnodes());

        // map LP to PE
        tw_lp_onpe(lp_index, g_tw_pe, lp_gid);
        // map LP to KP
        kpid = lp_index % nkp_per_pe;
        tw_lp_onkp(g_tw_lp[lp_index], g_tw_kp[kpid]);
    }
}

