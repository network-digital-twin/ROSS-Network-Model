#include "parse_new_config.h"
#include "ip.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


config *parseConfigFile(char *path)
{
    FILE *file;

    // Buffer to store each line, initially NULL
    char *line = NULL;
    size_t len = 0;
    size_t read;

    const char delimiter[] = ",";

    config *conf = (config *)malloc(sizeof(config));
    conf->num_ports = 0;
    conf->num_routes = 0;
    conf->num_topos = 0;
    conf->ports = NULL;
    conf->routes = NULL;
    conf->topos = NULL;

    file = fopen(path, "r");
    if (file == NULL) {
        printf("ERROR! Cannot open file %s\n", path);
        exit(-1);
    }

    // Read and print each line
    while ((read = getline(&line, &len, file)) != -1) {
        line[read - 1] = '\0'; // strip new lines

        // Use strtok to split the line
        char *type = strtok(line, delimiter);
        
        if(strcmp(type, "info") == 0){
            parseInfo(conf, delimiter);
        }
        else if (strcmp(type, "port") == 0){
            parsePort(conf, delimiter);
        }
        else if (strcmp(type, "route") == 0){
            parseRoute(conf, delimiter);
        }
        else if (strcmp(type, "topo") == 0){
            parseTopo(conf, delimiter);
        }
    }

    // Assign the port pointer in each route record
    int flag = 0;
    for (int i = 0; i < conf->num_routes; i++) {
        flag = 0;
        for (int j = 0; j < conf->num_ports; j++) {
            if(strcmp(conf->routes[i].srcPort, conf->ports[j].name) == 0) {
                conf->routes[i].srcPortPtr = &conf->ports[j];
                // TODO: here the info of the port and routing may not match due to the dataset.
                // if (conf->routes[i].srcPortPtr->destNode != conf->routes[i].nextHopID) {
                //     printf("WARNING! Mismatch in port's destNode %d and routing's nextHopID %d in file %s\n", conf->routes[i].srcPortPtr->destNode, conf->routes[i].nextHopID, path);
                // }
                flag = 1;
                break;
            }
        }
        if (flag == 0) {
            printf("ERROR! Missing port name [%s] for routes in file %s\n", conf->routes[i].srcPort, path);
            printf("Please make sure all srcPort in the ``routes'' can be found in ``ports''\n");
            exit(-1);
        }
    }

    return conf;
}

void parseInfo(config* conf, const char delimiter[]){
    /*
        FORMAT:
            info,id,type        
    */

    char* id = strtok(NULL, delimiter);
    conf->id = strtol(id, NULL, 10);

    char* type = strtok(NULL, delimiter);
    strncpy(conf->type, type, sizeof(conf->type));
}

void parsePort(config* conf, const char delimiter[]){
    /*
        FORMAT:
            port,name,bw,ip,mask
    */
    port p;

    char* portName = strtok(NULL, delimiter);
    strncpy(p.name, portName, sizeof(p.name));

    char* portBW = strtok(NULL, delimiter);
    p.bw = strtoul(portBW, NULL, 10);

    char* portIP = strtok(NULL, delimiter);
    strncpy(p.ip, portIP, sizeof(p.ip));

    char* portMask = strtok(NULL, delimiter);
    p.mask = strtol(portMask, NULL, 10);

    p.id = conf->num_ports;
    
    // Initialise default value for the connected next node
    p.destNode = -1;
    strncpy(p.destPort, "", sizeof(p.destPort));

    conf->num_ports += 1;
    conf->ports = realloc(conf->ports, conf->num_ports * sizeof(port));
    conf->ports[conf->num_ports-1] = p;
}

void parseRoute(config* conf, const char delimiter[]){
    /*
        FORMAT:
            route,networkip,mask,dest_ip,dest_id,src_port,dest_port
    */

    route r;

    char* netIp = strtok(NULL, delimiter);
    strncpy(r.netIP, netIp, sizeof(r.netIP));

    char* mask = strtok(NULL, delimiter);
    r.mask = strtol(mask, NULL, 10);
    
    char* nextIp = strtok(NULL, delimiter);
    strncpy(r.nextHopIP, nextIp, sizeof(r.nextHopIP));

    char* nextId = strtok(NULL, delimiter);
    r.nextHopID = strtol(nextId, NULL, 10);
    
    char* srcPort = strtok(NULL, delimiter);
    strncpy(r.srcPort, srcPort, sizeof(r.srcPort));

    conf->num_routes += 1;
    conf->routes = realloc(conf->routes, conf->num_routes * sizeof(route));
    conf->routes[conf->num_routes-1] = r;
}

void parseTopo(config* conf, const char delimiter[]) {
    /*
        FORMAT:
            topo,destNode,srcPort,destPort
    */
    topo t;

    char* destNode = strtok(NULL, delimiter);
    t.destNode = strtol(destNode, NULL, 10);

    char* srcPort = strtok(NULL, delimiter);
    strncpy(t.srcPort, srcPort, sizeof(t.srcPort));

    char* destPort = strtok(NULL, delimiter);
    strncpy(t.destPort, destPort, sizeof(t.destPort));

    conf->num_topos += 1;
    conf->topos = realloc(conf->topos, conf->num_topos * sizeof(topo));
    conf->topos[conf->num_topos-1] = t;

    for (int i = 0; i < conf->num_ports; i++) {
        if(strcmp(srcPort, conf->ports[i].name) == 0) {
            conf->ports[i].destNode = t.destNode;
            strncpy(conf->ports[i].destPort, t.destPort, sizeof(conf->ports[i].destPort));
        }
    }



}

void printConf(config* conf){
    printf("Switch: %d\n\tType: %s \n", conf->id, conf->type);
    printf("\t%d ports: \n", conf->num_ports);
    for(int i=0; i<conf->num_ports;i++){
        printf("\t\t name: %s | id: %d | BW: %ld | ip: %s | mask:%d | destNode:%d | destPort:%s\n", conf->ports[i].name, conf->ports[i].id, conf->ports[i].bw, conf->ports[i].ip, conf->ports[i].mask, conf->ports[i].destNode, conf->ports[i].destPort);
    }

    printf("\t%d routes: \n", conf->num_routes);
    for(int i=0; i<conf->num_routes;i++){
        printf("\t\t networkIP: %s  | networkMask: %d | nextHopIP: %s | nextHopID: %d | srcPort: %s || srcPortIP: %s\n", conf->routes[i].netIP, conf->routes[i].mask, conf->routes[i].nextHopIP, conf->routes[i].nextHopID, conf->routes[i].srcPort, conf->routes[i].srcPortPtr->ip);
    }

    printf("\t%d topos: \n", conf->num_topos);
    for(int i=0; i<conf->num_topos;i++){
        printf("\t\t destNode: %d | srcPort: %s | destPort: %s\n", conf->topos[i].destNode, conf->topos[i].srcPort, conf->topos[i].destPort);
    }
}


// Compare function for qsort, largest to smallest
int compare_routes(const void *a, const void *b) {
    return ((route *)b)->mask - ((route *)a)->mask;
}

// Given the dest_id, get the pointer to the route record of next hop. Return NULL if not found.
const route *get_next_hop(const char *dest_ip, const route *routes, int num_routes) {
    int * matched_routes_idx = (int *)malloc(sizeof(routes) * num_routes);
    int j = 0;
    // Sift all matched routes
    for (int i = 0; i < num_routes; i++) {
        if(is_ip_in_subnet(dest_ip, routes[i].netIP, routes[i].mask)) {
            matched_routes_idx[j] = i;
            j++;
        }
    }
    if (j == 0) {
        return NULL;
    }
    // use quick sort to sort all matched_routes_idx by the size of mask
    qsort(matched_routes_idx, j, sizeof(int), compare_routes);
    return (const route *) &routes[matched_routes_idx[0]];
}

// Return the specifc output port, NULL if it is myself or not found.
// Assign a return code to ``ret'': 
//         0 if this is the final destination;
//         1 if the next_hop exist;
//         -1 if there is no matching route in the routing table;
//         -2 if there is a matching route, but the next_hop port is not connected to any other switch
const port *get_port_for_next_hop(config *conf, const char *dest_ip, int *ret) {
    // First check if this is the final destination, by matching `dest_ip' with the IP of each port
    for(int i = 0; i <= conf->num_ports; i++) {
        if (strcmp(conf->ports[i].ip, dest_ip) == 0) {
            *ret = 0;
            return NULL;
        }
    }

    // If this is not the final deestination, then match the routing table
    const route *next_hop = get_next_hop(dest_ip, conf->routes, conf->num_routes);
    if (next_hop == NULL) {
        // printConf(conf);
        printf("ERROR: Next hop not found for ip %s in switch %d\n", dest_ip, conf->id);
        *ret = -1;
        return NULL;
    }
    if (next_hop->srcPortPtr->destNode < 0) {
        // printConf(conf);
        printf("ERROR: Next hop port %s in switch %d is not connected to any other switch\n", next_hop->srcPortPtr->name, conf->id);
        *ret = -2;
        return NULL;
    }
    *ret = 1;
    return next_hop->srcPortPtr;
}






