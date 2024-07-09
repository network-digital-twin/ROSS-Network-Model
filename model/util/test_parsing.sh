#!/bin/bash
gcc ip.c parse_new_config.c test_parsing.c -o test_parsing
./test_parsing