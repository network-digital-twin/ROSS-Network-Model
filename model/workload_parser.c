#include "parsers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

packets parseWorkload(char *path)
{
    FILE *fptr;
    char *line = NULL;
    size_t len = 0;
    size_t read;

    fptr = fopen(path, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    packets pks;
    pks.num = 0;
    pks.pks = (packet *)malloc(pks.num * sizeof(packet));

    while ((read = getline(&line, &len, fptr)) != -1)
    {
        packet temp;
        temp = packetFromLine(line);
        pks.pks = realloc(pks.pks, (pks.num + 1) * sizeof(packet));
        pks.pks[pks.num] = temp;
        pks.num++;
    }

    return pks;
}

packet packetFromLine(char *line)
{
    packet pkt;
    char *word;
    int col;

    line[strcspn(line, "\n")] = 0; // stripping new line

    word = strtok(line, " ");
    col = 0;
    while (word != NULL)
    {
        switch (col)
        {
        case 0: // unique id
            pkt.id = atoi(word);
            break;
        case 1:
            pkt.message_id = atoi(word);
            break;
        case 2: // src
            pkt.src = atoi(word);
            break;
        case 3: // dest
            pkt.dest = atoi(word);
            break;
        case 4: // packet size
            pkt.size = atoi(word);
            break;
        case 5: // timestamp
            pkt.timestamp = atof(word);
            break;
        case 6:
            pkt.priority_level = atoi(word);
            break;
        }
        word = strtok(NULL, " ");
        col += 1;
    }

    return pkt;
}