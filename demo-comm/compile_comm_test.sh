#!/bin/sh
#
gcc -Wall -m32 -g -I. tcp.c -c -o tcp.o
gcc -Wall -m32 -g -I. tcp_comm.c -c -o tcp_comm.o
gcc -Wall -m32 -g -I. comm_test.c -c -o comm_test.o
gcc -m32 -g comm_test.o tcp_comm.o tcp.o -o comm_test
