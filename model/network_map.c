#include <assert.h>
#include "network.h"

tw_peid *pe_to_num_lps;
tw_lpid *lp_to_pe;
tw_lpid *lp_to_lid;
tw_lpid *local_gids;

tw_lpid lpTypeMapper(tw_lpid gid)
{
     // printf("%llu\n",gid);
     if (gid < total_switches)
     {
         // printf("%llu >= %i\n",gid,total_terminals);
         return SWITCH;
     }
     else
     {
         // printf("%llu < %i\n",gid,total_terminals);
         return TERMINAL;
     }

}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid custom_mapping_lp_to_pe(tw_lpid gid) {
    return lp_to_pe[gid];
}

//Map LP to the local ID on the PE
tw_lp * custom_mapping_lpgid_to_local(tw_lpid gid) {
    tw_lpid local_index = lp_to_lid[gid];
    return g_tw_lp[local_index];
}


void custom_mapping_setup(void) {
    tw_kpid kpid;
    tw_lpid total_lps = total_terminals + total_switches;
    tw_lpid nkp_per_pe = g_tw_nkp;

    // By default 16 kps per pe
    for(kpid = 0; kpid < nkp_per_pe; kpid++) {
        tw_kp_onpe(kpid, g_tw_pe);
    }

    // figure out how many LPs are on this PE
    tw_lpid lps_per_pe = pe_to_num_lps[g_tw_mynode];
    printf("Node %ld: g_tw_nlp %llu, g_tw_nkp %lu, lps_per_pe %llu\n", g_tw_mynode, g_tw_nlp, g_tw_nkp, lps_per_pe);


    // set up the LPs
    for (int lp_index = 0; lp_index < lps_per_pe; lp_index++) {
        // get LP's GID
        tw_lpid lp_gid = local_gids[lp_index];
        // map LP to PE
        tw_lp_onpe(lp_index, g_tw_pe, lp_gid);
        // map LP to KP
        kpid = lp_index % nkp_per_pe;
        tw_lp_onkp(g_tw_lp[lp_index], g_tw_kp[kpid]);
    }
}



void init_partition(char *filename, tw_lpid total_lps) {
    // read file
    // calculate PE -> numLP
    // calculate LPid -> PE
    FILE *fptr;
    char *line = NULL;
    size_t read;
    size_t len = 0;

    fptr = fopen(filename, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(EXIT_FAILURE);
    }


    pe_to_num_lps = (tw_peid *)calloc(tw_nnodes(), sizeof(tw_peid));
    lp_to_pe = (tw_lpid *)calloc(total_lps, sizeof(tw_lpid));
    lp_to_lid = (tw_lpid *)calloc(total_lps, sizeof(tw_lpid));


    tw_lpid lpid = -1;
    tw_peid peid;
    char *endptr;
    while ((read = getline(&line, &len, fptr)) != -1)
    {
        lpid++;
        line[read-1] = '\0';
        peid = strtol(line, &endptr, 10);
        if (*endptr != '\0') {
            printf("Conversion failed: input string is not a valid integer. Unconverted characters: %s\n", endptr);
            exit(EXIT_FAILURE);
        }
        // Set global variables:
        lp_to_pe[lpid]=peid;
        lp_to_lid[lpid]=pe_to_num_lps[peid];
        pe_to_num_lps[peid]++;
    }
    assert(lpid + 1 + total_terminals == total_lps);
    assert(total_terminals == 1); // Now this function only works for one ``abstract'' terminal

    // Assign the terminal LP to the last PE
    lpid++;
    lp_to_pe[lpid]=peid;
    lp_to_lid[lpid]=pe_to_num_lps[peid];
    pe_to_num_lps[peid]++;

    // Store all local GIDs into local_gids.
    tw_lpid num_local_lps = pe_to_num_lps[g_tw_mynode];
    local_gids = (tw_lpid *)malloc( num_local_lps * sizeof(tw_lpid));
    tw_lpid index = 0;
    for (tw_lpid gid = 0; gid < total_lps; gid++) {
        if (lp_to_pe[gid] == g_tw_mynode) { // If this LP belongs to this PE
            local_gids[index] = gid;
            if(index != lp_to_lid[gid]) {
                printf("ERROR: index %llu, lp_to_lid[i] %llu, gid %llu\n", index, lp_to_lid[gid], gid);
            }
            assert(index == lp_to_lid[gid]);
            index++;
        }
    }

    fclose(fptr);
    printf("Loading partitions done on node %lu\n" , g_tw_mynode);
    for(int i = 0; i < num_local_lps; i++) {
        printf("%llu ", local_gids[i]);
    }
    printf("\n");
}