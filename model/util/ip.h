#ifndef NETWORK_MODEL_IP_H
#define NETWORK_MODEL_IP_H

unsigned int ip_to_int(const char *ip);
void int_to_ip(unsigned int ip, char* ip_str);
unsigned int subnet_mask_to_int(int netMask);
int is_ip_in_subnet(const char *destIP, const char *subnetIP, int subnetMask);


#endif