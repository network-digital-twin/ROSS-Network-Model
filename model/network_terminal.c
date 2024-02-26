//
// Created by Nan on 2024/1/11.
//
//The C driver file for a ROSS model
//This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

//Includes
#include "network.h"
#include <time.h>
#include <sys/stat.h>
#include <assert.h>

//--------------Terminal stuff-------------

static void write_terminal_stats_to_file(unsigned long long ***count);
static unsigned long long get_max_num_packets_for_a_switch(unsigned long long ***count);

void terminal_parse_workload(terminal_state *s, tw_lp *lp)
{
    s->pks = parseWorkload(trace_path);

    printf("----------------------------------------------------\n");
    printf("----------------------------------------------------\n");
    printf("I am terminal: Testing parsing of WL by printing the first 100 packets...\n");
    for (int i = 0; i < 10; i++)
    {
        printf("packet: %d, %d %d, %d, %d, %f, %d \n", s->pks.pks[i].id, s->pks.pks[i].message_id, s->pks.pks[i].src, s->pks.pks[i].dest, s->pks.pks[i].size, s->pks.pks[i].timestamp, s->pks.pks[i].priority_level);
    }
    printf("----------------------------------------------------\n");
    printf("----------------------------------------------------\n");
}

void terminal_init (terminal_state *s, tw_lp *lp)
{
    terminal_parse_workload(s, lp);

    tw_lpid self = lp->gid;

    // init state data
    s->num_packets_sent = 0;


}

void terminal_prerun(terminal_state *s, tw_lp *lp)
{
    int src, dest;
    tw_stime ts;

     // [src][dest][priority]
    unsigned long long ***count = (unsigned long long ***)malloc(sizeof(unsigned long long **) * total_switches); // TODO: this is not memory efficient!
    for(int i = 0; i < total_switches; i++) {
        count[i] = (unsigned long long **)malloc(sizeof(unsigned long long *) * total_switches);
        for(int j = 0; j < total_switches; j++) {
            count[i][j] = (unsigned long long *)malloc(sizeof(unsigned long long) * NUM_QOS_LEVEL);
            for(int k = 0; k < NUM_QOS_LEVEL; k++) {
                count[i][j][k] = 0;
            }
        }
    }


    for (int i = 0; i < s->pks.num; i++)
    {
        src = s->pks.pks[i].src;
        dest = s->pks.pks[i].dest;
        ts = s->pks.pks[i].timestamp; // We need to use absolute time here, because time will not proceed in this function.

        if(ts >= g_tw_ts_end) {
            break;
        }
        //printf("To be scheduled at %f\n", ts);

        tw_event *e = tw_event_new(src, ts, lp);
        tw_message *out_msg = tw_event_data(e);
        out_msg->packet.pid = s->pks.pks[i].id;
        out_msg->packet.send_time = s->pks.pks[i].timestamp;
        out_msg->packet.src = src;
        out_msg->packet.dest = dest;
        out_msg->packet.prev_hop = -1;
        out_msg->packet.next_hop = -1;
        out_msg->packet.type = s->pks.pks[i].priority_level;
        out_msg->packet.size_in_bytes = s->pks.pks[i].size;
        out_msg->type = ARRIVE;
        out_msg->port_id = -1;  // this variable is of no use here, so set it to -1.
        tw_event_send(e);

        s->num_packets_sent++;
        count[src][dest][s->pks.pks[i].priority_level]++;


    }
    printf("Schedule done\n");

    unsigned long long max = get_max_num_packets_for_a_switch(count);
    printf("Max number of packets that is sent to one switch: %llu\n", max);
    if(max > MAX_RECORDS) {
        printf("----------------------------------------------------\n");
        printf("----------------------------------------------------\n");
        printf("Please consider increase the macro MAX_RECORDS, which now is: %llu\n", MAX_RECORDS);
        printf("Because the max number of packets that a switch can receive is %llu\n", max);
        printf("We use MAX_RECORDES to specify the allocated memory size of each switch to store the statistics\n");
        printf("With small MAX_RECORDS, realloc will be called repeatedly when there is not enough memory to store the statistics\n");
        printf("BUT realloc IS TIME CONSUMING!\n");
        printf("----------------------------------------------------\n");
        printf("----------------------------------------------------\n");

    }

    write_terminal_stats_to_file(count);

    // Free int ***count
    for(int i = 0; i < total_switches; i++) {
        for(int j = 0; j < total_switches; j++) {
            free(count[i][j]);
        }
        free(count[i]);
    }
    free(count);


    // printf("%d: I am a terminal assigned to PO %llu\n",self,assigned_switch);
}

static void write_terminal_stats_to_file(unsigned long long ***count) {// Write the statistics to a file
    char str[150];
    puts(out_dir);
    strcpy(str, out_dir);
    strcat(str, "/generate.csv");
    FILE *fptr;
    fptr = fopen(str, "w");

    // print all value of count that is positive
    if(fptr != NULL) {
        fprintf(fptr, "src,dest,priority,pkt_sent\n");
    }
    for(int i = 0; i < total_switches; i++) {
        for(int j = 0; j < total_switches; j++) {
            for(int k = 0; k < NUM_QOS_LEVEL; k++) {
                if(count[i][j][k] > 0) {
                    if(fptr != NULL) {
                        fprintf(fptr, "%d,%d,%d,%llu\n", i, j, k, count[i][j][k]);
                    }
                }
            }
        }
    }
    fclose(fptr);
}


void terminal_final(terminal_state *s, tw_lp *lp)
{
    int self = lp->gid;
    printf("Terminal %d: S:%d packets\n", self, s->num_packets_sent);
}

// Calculate what is the maximum number of packets that a switch can receive as the final destination.
static unsigned long long get_max_num_packets_for_a_switch(unsigned long long ***count) {
    unsigned long long max = 0;
    unsigned long long tmp;
    for(int i = 0; i < total_switches; i++) {
        for(int j = 0; j < total_switches; j++) {
            tmp = 0;
            for(int k = 0; k < NUM_QOS_LEVEL; k++) {
                tmp += count[i][j][k];
            }
            if(tmp > max) {
                max = tmp;
            }
        }
    }
    return max;
}