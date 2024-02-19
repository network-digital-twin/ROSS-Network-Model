//
// Created by George.
//
#ifndef NETWORK_MODEL_PARSER_H
#define NETWORK_MODEL_PARSER_H

#include "ross.h"
// switch configuration structs

typedef struct port
{
    int id;
    char *name;
    double bandwidth;
} port;

typedef struct route
{
    int dest;
    int nextHop;
    int port_id;
    char *portName;
} route;

typedef struct config
{
    int id; // ID of the switch
    char *type; // "access", "mixed", or "core"
    port *ports; // To-switch port list
    int numPorts; // number of to-switch ports
    route *routing;  // Routing table
    int routingTableSize; // number of records in the routing table
} config;

// workload structs

typedef struct packet_load
{
    int id;
    int message_id;
    int src;
    int dest;
    int size;
    tw_stime timestamp;
    int priority_level;

} packet_load;

typedef struct packets
{
    packet_load *pks;
    int num;
} packets;

/*
    SWITCH CONFIGURATION
*/

// parsing
extern char* getNameSegment(char *line);
extern void parsePort(char *line, port *portList, int curIndex);
extern int getColLocation(char *s);
extern int getStartLocation(char *s);

// functionalities
extern config *parseConfigFile(char *path, int id);
extern void printConfig(config *conf);
extern double getPortBandwidth(config *conf, char *portName);
extern char *getNextHopPort(config *conf, int dest);

/* 
    WORKLOAD
*/

packets parseWorkload(char *path);
packet_load packetFromLine(char *line);

#endif