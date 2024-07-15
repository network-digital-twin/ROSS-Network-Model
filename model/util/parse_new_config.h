#ifndef NETWORK_MODEL_PARSER_H
#define NETWORK_MODEL_PARSER_H

typedef struct port
{
    char name[50];
    int id;
    unsigned long int bw;  // bandwidth in bps
    int mask;
    char ip[16];
    int destNode;  // the network node (switch) ID this port connects to. -1 means invalid
    char destPort[50]; // the port of the network node (switch) this port connects to. Empty string means invalid.
} port;

typedef struct route
{
    int mask;
    char netIP[16];
    char nextHopIP[16];
    int nextHopID;
    char srcPort[50];
    const port *srcPortPtr; // pointer to the port struct
} route;

typedef struct topo
{
    int destNode; // destination node id, not LP id
    char srcPort[50];
    char destPort[50];
} topo;

typedef struct config
{
    int id;
    char type[50];
    port *ports;
    int num_ports;
    route *routes;
    int num_routes;
    topo *topos;
    int num_topos;
} config;

config *parseConfigFile(char *path);
void parseInfo(config* conf, const char delimiter[]);
void parsePort(config* conf, const char delimiter[]);
void parseRoute(config* conf, const char delimiter[]);
void parseTopo(config* conf, const char delimiter[]);
void printConf(config* conf);

const route *get_next_hop(const char *dest_ip, const route *routes, int num_routes);
const port *get_port_for_next_hop(config *conf, const char *dest_ip, int *ret);

#endif