//
// Created by Nan on 2024/2/20.
//
#include "network.h"

void switch_init_stats(switch_state *s, tw_lp *lp) {

    s->stats = (stats *)malloc(sizeof(stats));
    s->stats->num_packets_recvd = 0;
    s->stats->num_packets_dropped = 0;
    s->stats->num_packets_sent = 0;
    s->stats->received = 0;
    s->stats->records_capacity = MAX_RECORDS;
    s->stats->records = (record *)malloc(sizeof(record) * s->stats->records_capacity);

}

void switch_update_stats(stats *st, unsigned long pid, double delay, unsigned char drop) {
    unsigned long long index = st->num_packets_recvd + st->num_packets_dropped;
    if(index + 1 >= st->records_capacity) {
        st->records_capacity *= 2;
        st->records = (record *)realloc(st->records, sizeof(record) * st->records_capacity);
        if(st->records == NULL) {
            printf("Error: realloc failed in stats.c\n");
            exit(-1);
        }
    }
    st->records[index].pid = pid;
    st->records[index].delay = delay;
    st->records[index].drop = drop;
    if(drop) {
        st->num_packets_dropped++;
    } else {
        st->num_packets_recvd++;
    }
}

void switch_update_stats_reverse(stats *st) {
    // TODO: notice here we do not reverse the memory allocation, because it will be too costly.
    unsigned long long index = st->num_packets_recvd + st->num_packets_dropped - 1;
    if (st->records[index].drop) {
        st->num_packets_dropped--;
    } else {
        st->num_packets_recvd--;
    }
    st->records[index].pid = -1;
    st->records[index].delay = -1;
    st->records[index].drop = -1;
}

void switch_free_stats(switch_state *s) {
    free(s->stats->records);
    free(s->stats);
}


void print_switch_stats(const switch_state *s, tw_lp *lp) {
//    printf("src, dest, qos, avg_delay, jitter, pkt_received\n");
//    for (int i = 0; i < total_switches; ++i) {
//        for (int q = 0; q < s->num_qos_levels; ++q) {
//            if(s->stats->count[i][q] > 0) {
//                printf("%llu,%d,%d,%f,%f,%d\n",
//                       lp->gid,
//                       i,
//                       q,
//                       s->stats->total_delay[i][q]/s->stats->count[i][q],
//                       s->stats->jitter[i][q],
//                       s->stats->count[i][q]
//                );
//            }
//
//        }
//    }
}

void write_switch_stats_to_file(const switch_state *s, tw_lp *lp) {// Write the statistics to a file
    char str[150];
    char filename[25];
    char str_header[150];
    char str_script[150];
    sprintf(filename, "%llu.csv", lp->gid);
    strcpy(str, out_dir);
    strcat(str, "/raw/");
    strcat(str, filename);
    strcpy(str_header, out_dir);
    strcat(str_header, "/received_header.txt");
    strcpy(str_script, out_dir);
    strcat(str_script, "/prepare.sh"); // scripts for gathering raw data together


    FILE *fptr;
    fptr = fopen(str, "a");

    if(fptr == NULL) {
        fclose(fptr);
        exit(-1);
    }

    if(lp->gid == 0) {
        FILE *fptr_header;
        fptr_header = fopen(str_header, "w");
        fprintf(fptr_header, "pid, delay, drop\n");
        fclose(fptr_header);

        fptr_header = fopen(str_script, "w");
        fprintf(fptr_header, "cat ./raw/*.csv > received.csv\n");
        fclose(fptr_header);

    }

    for (unsigned long long i = 0; i < s->stats->num_packets_recvd + s->stats->num_packets_dropped; ++i) {
        fprintf(fptr, "%lu,%f,%d\n", s->stats->records[i].pid, s->stats->records[i].delay, s->stats->records[i].drop);
    }

    // TODO: use MPI to gather all the data to rank 0 and write to file
    fclose(fptr);

}
