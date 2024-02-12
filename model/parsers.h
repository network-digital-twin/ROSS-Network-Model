#ifndef _parsers_h_
#define _parsers_h_

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
    char *portName;
} route;

typedef struct config
{
    int id;
    char *type;
    port *ports;
    int numPorts;
    route *routing;
    int routingTableSize;
} config;

// workload structs

typedef struct packet
{
    int id;
    int src;
    int dest;
    float size;
    float timstamp;
} packet;

typedef struct packets
{
    packet *pks;
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
packet packetFromLine(char *line);

#endif