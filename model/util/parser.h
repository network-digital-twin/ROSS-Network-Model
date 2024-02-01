//
// Created by George.
//

#ifndef NETWORK_MODEL_PARSER_H
#define NETWORK_MODEL_PARSER_H

typedef struct port
{
    char *name;
    double bandwidth;
} port;

typedef struct route // one record in the routing table
{
    int dest;  // destination switch id
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


#endif //NETWORK_MODEL_PARSER_H
