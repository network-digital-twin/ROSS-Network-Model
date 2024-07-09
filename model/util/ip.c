#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Convert IP string to integer
unsigned int ip_to_int(const char *ip) {
    unsigned int result = 0;
    char *token;
    char *str = strdup(ip); // Copy the string to avoid modifying the original string
    char *delim = ".";

    for (int i = 0; i < 4; ++i) {
        token = strtok(i == 0 ? str : NULL, delim);
        result |= (atoi(token) << ((3 - i) * 8));
    }

    free(str);
    return result;
}

// Function to convert an integer to an IP string
void int_to_ip(unsigned int ip, char* ip_str) {
    sprintf(ip_str, "%u.%u.%u.%u",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF);
}

// Convert net mask
unsigned int subnet_mask_to_int(int netMask) {
    return (0xFFFFFFFF << (32 - netMask));
}

// Check if the IP is in the given subnet
int is_ip_in_subnet(const char *destIP, const char *subnetIP, int subnetMask) {
    unsigned int ip_int = ip_to_int(destIP);
    unsigned int netIP_int = ip_to_int(subnetIP);
    unsigned int netMask_int = subnet_mask_to_int(subnetMask);
    return (ip_int & netMask_int) == (netIP_int & netMask_int);
}
