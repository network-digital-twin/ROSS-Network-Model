#include "util/parsers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


config *parseConfigFile(char *path, int id)
{
    // for parsing YAML
    FILE *fptr;
    char *line = NULL;
    size_t len = 0;
    size_t read;

    char *currentSegment = NULL;
    int lineInSeg;

    // vars for routing
    int lineInGroup = 0;
    int currentGroup = 0;
    route *routes = NULL;
    int maxDestId = 0;
    char *dest;
    char *nextHop;

    // vars for type
    int typeStart;
    int typeSize;

    // vars for ports
    int bwStart;
    int bwSize;
    char *bw;

    // auxilary
    int start;
    int colPos;

    config *conf = (config*)malloc(sizeof(config));
    conf->id = id;

    fptr = fopen(path, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fptr)) != -1)
    {
        line[strcspn(line, "\n")] = 0; // stripping new line
        // this will run on "top level" segments i.e., ports, routing, type etc...
        if (line[0] != ' ')
        {
            char* word = strtok(line, ":");

            if (currentSegment != NULL){
                free(currentSegment);
            }

            copy_word_with_null_tem(&currentSegment, word);

            lineInSeg = -1; // still on the top level line
        }else{
            // removing all spaces to make parsing with strtok(line,":") easier
            // THIS HAS TO HAPPEN AFTER WE ENTER A SEGMENT CAUSE THE CHECK FOR ENTERING A SEGMENT RELIES ON SPACES
            remove_spaces(line);
        }

        // these will run for every line in that "top level segment"
        // and thus logic handling these sub-segments goes here
        if (strcmp(currentSegment, "ports") == 0)
        {
            // if this is the first line (still segment) just initialise the struct
            if (lineInSeg == -1)
            {
                lineInSeg++; // skip to the first data line
                conf->ports = (port *)malloc(sizeof(port));
                conf->numPorts = 0;

                while (strtok(NULL, ":") != NULL);
            }
            else // for every line that contains actual port data
            {
                conf->ports = realloc(conf->ports, (lineInSeg + 1) * sizeof(port));

                char* word = strtok(line, ":");
                copy_word_with_null_tem(&conf->ports[lineInSeg].name, word);

                word = strtok(NULL, ":");
                conf->ports[lineInSeg].bandwidth = atof(word);

                conf->ports[lineInSeg].id = lineInSeg;

                conf->numPorts++;
                lineInSeg++;

                while (strtok(NULL, ":") != NULL);
            }
        }
        else if (strcmp(currentSegment, "routing") == 0)
        {
            /*
                We need to parse routes in groups of 3 lines since the structure is the following
                    dest:
                        next_hop: neighbour
                        port: port_name

                Logic:
                    --- while parsing ---
                    - parse routing data into a list of route structs
                    - find the max dest node id

                    --- after parsing ---
                    - create a list of ports of size MAX_DEST_ID. What we called 'bucket'
                    - iterate over all routes and assigng them to their corespoding index
                        - based on the id of the dest node
            */
            if (lineInSeg == -1)
            {
                lineInSeg++; // skip to the first data line
                lineInGroup = 0;
                routes = (route *)malloc(0);

                while (strtok(NULL, ":") != NULL);
            }
            else
            {
                if (lineInGroup == 0)
                {
                    routes = realloc(routes, (currentGroup + 1) * sizeof(route));

                    char* word = strtok(line, ":");
                    routes[currentGroup].dest = atoi(word);

                    if (routes[currentGroup].dest > maxDestId)
                        maxDestId = routes[currentGroup].dest;

                    while (strtok(NULL, ":") != NULL);

                    lineInGroup++;
                }
                else if (lineInGroup == 1)
                {
                    /*
                        in the second line of the subgroup get the next hop node | \t next_hop: neighbour_id
                    */
                    strtok(line, ":");
                    char* word = strtok(NULL,":");
                    routes[currentGroup].nextHop = atoi(word);

                    while (strtok(NULL, ":") != NULL);

                    lineInGroup++;
                }
                else if (lineInGroup == 2)
                {
                    /*
                        on the third line of the subgroup get the port name | port: port_name
                    */

                    strtok(line, ":");
                    char* word = strtok(NULL, ":");

                    copy_word_with_null_tem(&routes[currentGroup].portName, word);

                    while (strtok(NULL, ":") != NULL);

                    lineInGroup = 0;
                    currentGroup++;
                }
            }
        }
        else if (strcmp(currentSegment, "type") == 0)
        {
            char* type = strtok(NULL, ":");
            remove_spaces(type);
            copy_word_with_null_tem(&conf->type, type);

            while (strtok(NULL, ":") != NULL);
        }
    }

    fclose(fptr);

    // populate the routing table using the 'bucket' list data struct
    conf->routingTableSize = maxDestId + 1;
    conf->routing = (route *)malloc(conf->routingTableSize * sizeof(route));

    route null_route;
    null_route.portName = NULL;
    null_route.nextHop = -1;
    null_route.dest = -1;

    for (int i=0; i<conf->routingTableSize; i++){
        conf->routing[i] = null_route;
    }

    for (int i = 0; i < currentGroup; i++)
    {
        conf->routing[routes[i].dest] = routes[i];
    }

    return conf;
}

void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}

void copy_word_with_null_tem(char** dest, char* word){
    *dest = malloc(strlen(word)+1);
    memset(*dest, 0, strlen(word)+1);
    strncpy(*dest, word, strlen(word));
}

double getPortBandwidth(config *conf, char *portName)
{
    for (int i = 0; i < conf->numPorts; i++)
    {
        if (strcmp(conf->ports[i].name, portName) == 0)
            return conf->ports[i].bandwidth;
    }
    return -1.0;
}

char *getNextHopPort(config *conf, int dest)
{
    return conf->routing[dest].portName;
}

void printConfig(config *conf)
{
    int cnt = 0, i = 0;
    printf("--------------------\n");
    printf("Config for node: %d\n", conf->id);
    printf("\tType: %s\n", conf->type);
    printf("\tPort list:\n");
    for (int i = 0; i < conf->numPorts; i++)
    {
        printf("\t\tid: %d: %s | %f\n", conf->ports[i].id, conf->ports[i].name, conf->ports[i].bandwidth);
    }
    printf("\tRouting Table: \n");
    printf("\t\t%d records! showing first 20 that are not NULL...\n", conf->routingTableSize);
    while (cnt < 20 && i < conf->routingTableSize)
    {
        //printf("%d, %d \n", cnt, i);
        if (conf->routing[i].portName != NULL)
        {
            printf("\t\tdest: %d | next hop: %d | port: %s \n", conf->routing[i].dest, conf->routing[i].nextHop, conf->routing[i].portName);
            cnt++;
        }
        i++;
    }
    printf("--------------------\n");
}


void freeConfig(config *conf)
{
    int cnt, i = 0;

    for (int i = 0; i < conf->numPorts; i++)
        free(conf->ports[i].name);

    free(conf->ports);

    for (i = 0; i < conf->routingTableSize; i++)
        free(conf->routing[i].portName);

    free(conf->routing);
    free(conf->type);
    free(conf);
}