#include <assert.h>
#include "network.h"

tw_peid *pe_to_num_lps;
tw_lpid *lp_to_pe;
tw_lpid *lp_to_lid;
tw_lpid *local_gids;
tw_lpid *switch_to_lp;
tw_lpid *lp_to_switch;

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
    printf("Node %ld: g_tw_nlp %lu, g_tw_nkp %lu, lps_per_pe %lu\n", g_tw_mynode, g_tw_nlp, g_tw_nkp, lps_per_pe);


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
    assert(total_terminals <= 1); // Now this function only works for one ``abstract'' terminal

    // Assign the terminal LP to the last PE
    if(total_terminals == 1) {
        lpid++;
        lp_to_pe[lpid]=peid;
        lp_to_lid[lpid]=pe_to_num_lps[peid];
        pe_to_num_lps[peid]++;
    }

    // Store all local GIDs into local_gids.
    tw_lpid num_local_lps = pe_to_num_lps[g_tw_mynode];
    local_gids = (tw_lpid *)malloc( num_local_lps * sizeof(tw_lpid));
    tw_lpid index = 0;
    for (tw_lpid gid = 0; gid < total_lps; gid++) {
        if (lp_to_pe[gid] == g_tw_mynode) { // If this LP belongs to this PE
            local_gids[index] = gid;
            if(index != lp_to_lid[gid]) {
                printf("ERROR: index %lu, lp_to_lid[i] %lu, gid %lu\n", index, lp_to_lid[gid], gid);
            }
            assert(index == lp_to_lid[gid]);
            index++;
        }
    }

    fclose(fptr);
    printf("Loading partitions done on node %lu\n" , g_tw_mynode);
    for(int i = 0; i < num_local_lps; i++) {
        printf("%lu ", local_gids[i]);
    }
    printf("\n");
}

void init_switch_to_lp(char *filename) {
    tw_lpid map_size = total_switches;
    switch_to_lp = (tw_lpid *)malloc(map_size * sizeof(tw_lpid));
    lp_to_switch = (tw_lpid *)malloc(map_size * sizeof(tw_lpid));
    if(filename == NULL) {
        // If no file is provided, then use the default mapping
        for(tw_lpid i = 0; i < map_size; i++) {
            switch_to_lp[i] = i;
            lp_to_switch[i] = i;
        }
    } else {
        if (g_tw_mynode == 0) {
            printf("Loading switch-to-lp mapping from file: %s\n", filename);
        }
        FILE *fptr;
        char *line = NULL;
        size_t read;
        size_t len = 0;
        tw_lpid count = 0;

        fptr = fopen(filename, "r");
        if (fptr == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", filename);
            exit(EXIT_FAILURE);
        }

        tw_lpid lpid = -1;
        tw_lpid switch_id;
        char *endptr;
        while ((read = getline(&line, &len, fptr)) != -1)
        {
            lpid++;
            if(line[read-1] == '\n') {
                line[read-1] = '\0';
            }
            switch_id = strtol(line, &endptr, 10);
            //printf("%d, %s\n",switch_id, );
            if (*endptr != '\0') {
                printf("Conversion failed: input string is not a valid integer. Unconverted characters: %s\n", endptr);
                exit(EXIT_FAILURE);
            }
            if (map_size - 1 < switch_id) {
                map_size = switch_id + 1;
                switch_to_lp = (tw_lpid *) realloc(switch_to_lp, map_size * sizeof(tw_lpid));
            }
            // Set global variables:
            switch_to_lp[switch_id] = lpid;
            lp_to_switch[lpid] = switch_id;
            count++;
        }
        if(count != total_switches) {
            printf("ERROR: %lu switches specified, but the number of lines [%lu] does not match: %s\n", total_switches, count, filename);
            exit(EXIT_FAILURE);
        }
        assert(count == total_switches);
        fclose(fptr);
    }

}

tw_lpid switch_id_to_lp_id(tw_lpid switch_id) {
    return switch_to_lp[switch_id];
}

tw_lpid lp_id_to_switch_id(tw_lpid lpid) {
    return lp_to_switch[lpid];
}