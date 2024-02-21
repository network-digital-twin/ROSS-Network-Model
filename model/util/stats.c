//
// Created by Nan on 2024/2/20.
//
#include "network.h"


void switch_init_stats(switch_state *s, tw_lp *lp) {
    int num_qos_levels = s->num_qos_levels;

    s->stats = (stats *)malloc(sizeof(stats));
    s->stats->num_packets_recvd = 0;
    s->stats->num_packets_sent = 0;

    s->stats->total_delay = (double **)malloc(sizeof(double *) * total_switches);
    s->stats->jitter = (double **)malloc(sizeof(double *) * total_switches);
    s->stats->count = (int **)malloc(sizeof(int *) * total_switches);
    for(int i = 0; i < total_switches; i++) {
        s->stats->total_delay[i] = (double *)malloc(sizeof(double) * num_qos_levels);
        s->stats->jitter[i] = (double *)malloc(sizeof(double) * num_qos_levels);
        s->stats->count[i] = (int *)malloc(sizeof(int) * num_qos_levels);
        for(int j = 0; j < num_qos_levels; j++) {
            s->stats->total_delay[i][j] = 0;
            s->stats->jitter[i][j] = 0;
            s->stats->count[i][j] = 0;
        }
    }

}

void switch_free_stats(switch_state *s) {
    for(int i = 0; i < total_switches; i++) {
        free(s->stats->total_delay[i]);
        free(s->stats->jitter[i]);
        free(s->stats->count[i]);
    }
    free(s->stats->total_delay);
    free(s->stats->jitter);
    free(s->stats->count);
    free(s->stats);
}


void print_switch_stats(const switch_state *s, tw_lp *lp) {
    printf("src, dest, qos, avg_delay, jitter, pkt_received\n");
    for (int i = 0; i < total_switches; ++i) {
        for (int q = 0; q < s->num_qos_levels; ++q) {
            if(s->stats->count[i][q] > 0) {
                printf("%llu,%d,%d,%f,%f,%d\n",
                       lp->gid,
                       i,
                       q,
                       s->stats->total_delay[i][q]/s->stats->count[i][q],
                       s->stats->jitter[i][q],
                       s->stats->count[i][q]
                );
            }

        }
    }
}

void write_switch_stats_to_file(const switch_state *s, tw_lp *lp) {// Write the statistics to a file
    char str[150];
    strcpy(str, out_dir);
    strcat(str, "/received.csv");
    FILE *fptr;
    fptr = fopen(str, "a");

    // print all value of count that is positive
    if(fptr == NULL) {
        fclose(fptr);
        exit(-1);
    }

//    if(lp->gid == 0) {
//        fprintf(fptr, "src, dest, qos, avg_delay, jitter, pkt_received\n");
//    }
    for (int i = 0; i < total_switches; ++i) {
        for (int q = 0; q < s->num_qos_levels; ++q) {
            if(s->stats->count[i][q] > 0) {
                fprintf(fptr,
                        "%llu,%d,%d,%f,%f,%d\n",
                        lp->gid,
                        i,
                        q,
                        s->stats->total_delay[i][q]/s->stats->count[i][q],
                        s->stats->jitter[i][q],
                        s->stats->count[i][q]
                );
            }

        }
    }
    // TODO: use MPI to gather all the data to rank 0 and write to file
    fclose(fptr);

}
