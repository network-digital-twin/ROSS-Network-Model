#include "parse_new_config.h"
#include "ip.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


void test1() {
    printf("TEST1\n");
    config *conf = parseConfigFile("../../data/zte_parsed_data_06242024/george_ZTE_data/0.txt");
    printConf(conf);

    const route* next_hop = get_next_hop(conf->routes[0].netIP, conf->routes, conf->num_routes);
    assert(next_hop != NULL);
    assert(strcmp(next_hop->nextHopIP, "8.23.251.64") == 0);

    next_hop = get_next_hop(conf->routes[2].netIP, conf->routes, conf->num_routes);
    assert(next_hop != NULL);
    assert(strcmp(next_hop->nextHopIP, "8.23.251.62") == 0);
}


void test2() {
    printf("TEST2\n");
    config *conf = parseConfigFile("../../data/zte_parsed_data_06242024/george_ZTE_data/436.txt");
    const port *port = get_port_for_next_hop(conf, "8.25.223.84");
    assert(port != NULL);
    assert(strcmp(port->name, "smartgroup2") == 0);
    int ret = 10;
    assert(get_port_for_next_hop(conf, "8.25.246.165", &ret) == NULL);
}

void test3() {
    printf("TEST3\n");
    char *ip = "192.168.1.10";
    char *ip2 = malloc(16);
    int_to_ip(ip_to_int(ip), ip2);
    assert(strcmp(ip, ip2) == 0);
}


int main(int argc, char **argv)
{
    test1();
    test2();
    test3();
    printf("TEST SUCCESS!\n");
    return 0;
}