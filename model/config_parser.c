#include "parsers.h"

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
    int numRoutes = 0;
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

    config *conf = (config *)malloc(sizeof(config));
    conf->id = id;

    fptr = fopen(path, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fptr)) != -1)
    {
        // this will run on "top level" segments i.e., ports, routing, type etc...
        if (line[0] != ' ')
        {
            getNameSegment(line, &currentSegment);
            lineInSeg = -1; // still on the top level line
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
            }
            else // for every line that contains actual port data
            {
                conf->ports = realloc(conf->ports, (lineInSeg + 1) * sizeof(port));

                parsePort(line, conf->ports, lineInSeg);

                conf->numPorts++;
                lineInSeg++;
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
            }
            else
            {
                if (lineInGroup == 0)
                {
                    routes = realloc(routes, (currentGroup + 1) * sizeof(route));

                    /*
                      in the first line of the subgroup we get the dest node | dest:\n
                    */

                    start = getStartLocation(line);

                    dest = (char *)malloc(strlen(line) - start - 2);       // skipping : and new_line
                    strncpy(dest, line + start, strlen(line) - start - 2); // as above

                    routes[currentGroup].dest = atoi(dest);

                    // find max dest id
                    if (routes[currentGroup].dest > maxDestId)
                        maxDestId = routes[currentGroup].dest;

                    free(dest);
                    lineInGroup++;
                }
                else if (lineInGroup == 1)
                {
                    /*
                        in the second line of the subgroup get the next hop node | \t next_hop: neighbour_id
                    */
                    colPos = getColLocation(line);

                    nextHop = (char *)malloc(strlen(line) - colPos - 3); // -3 cause -> ':', space and new_line
                    strncpy(nextHop, line + colPos + 2, strlen(line) - colPos - 3);

                    routes[currentGroup].nextHop = atoi(nextHop);

                    free(nextHop);

                    lineInGroup++;
                }
                else if (lineInGroup == 2)
                {
                    /*
                        on the third line of the subgroup get the port name | port: port_name
                    */
                    colPos = getColLocation(line);

                    routes[currentGroup].portName = (char *)malloc((strlen(line) - colPos - 3)); // -3 cause -> ':', space and new_line
                    strncpy(routes[currentGroup].portName, line + colPos + 2, strlen(line) - colPos - 3);

                    lineInGroup = 0;
                    currentGroup++;
                    numRoutes = currentGroup;
                }
            }
        }
        else if (strcmp(currentSegment, "type") == 0)
        {
            typeStart = strlen(currentSegment) + 2;
            typeSize = strlen(line) - typeStart - 1;

            conf->type = (char *)malloc(typeSize);

            strncpy(conf->type, line + typeStart, typeSize);
        }
    }

    fclose(fptr);

    // populate the routing table using the 'bucket' list data struct

    conf->routing = (route *)malloc(maxDestId * sizeof(route));
    conf->routingTableSize = maxDestId;

    for (int i = 0; i < currentGroup; i++)
    {
        conf->routing[routes[i].dest] = routes[i];
    }

    return conf;
}

void getNameSegment(char *line, char **currentSegment)
{
    int colLoc = getColLocation(line);
    char *segName = (char *)malloc(colLoc);
    strncpy(segName, line, colLoc);

    if (*currentSegment == NULL)
        free(*currentSegment);

    *currentSegment = (char *)malloc(strlen(segName));
    strncpy(*currentSegment, segName, strlen(segName));

    free(segName);
}

void parsePort(char *line, port *portList, int curIndex)
{
    port temp;

    int colPos = getColLocation(line);
    int start = getStartLocation(line);

    temp.id = curIndex;

    // get port name
    temp.name = (char *)malloc(colPos - 2);
    strncpy(temp.name, line + start, colPos - 2);

    // get port bw
    int bwStart = colPos + 2;                // skiping ": "
    int bwSize = strlen(line) - bwStart - 1; // skipping new line

    char *bw = (char *)malloc(bwSize);
    strncpy(bw, line + bwStart, bwSize);

    temp.bandwidth = strtod(bw, NULL);

    portList[curIndex] = temp;

    free(bw);
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
    int cnt, i = 0;
    printf("--------------------\n");
    printf("Config for node: %d\n", conf->id);
    printf("\tType: %s\n", conf->type);
    printf("\tPort list:\n");
    for (int i = 0; i < conf->numPorts; i++)
    {
        printf("\t\tid: %d: %s | %f\n", conf->ports[i].id, conf->ports[i].name, conf->ports[i].bandwidth);
    }
    printf("\tRouting Table: \n");
    printf("\t\t%d records! showing first 5 that are not NULL...\n", conf->routingTableSize);
    while (cnt < 5 || i == conf->routingTableSize - 1)
    {
        if (conf->routing[i].portName != NULL)
        {
            printf("\t\tdest: %d | next hop: %d | port: %s \n", conf->routing[i].dest, conf->routing[i].nextHop, conf->routing[i].portName);
            cnt++;
        }
        i++;
    }
    printf("--------------------\n");
}

int getColLocation(char *s)
{
    int colPos = -1;
    for (int i = 0; i < strlen(s); i++)
    {
        if (s[i] == ':')
        {
            colPos = i;
            break;
        }
    }
    return colPos;
}

int getStartLocation(char *s)
{
    int start = 0;
    for (int i = 0; i < strlen(s); i++)
    {
        if (s[i] == ' ')
            start += 1;
        else
            break;
    }
    return start;
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