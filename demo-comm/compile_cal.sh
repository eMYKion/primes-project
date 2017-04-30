#!/bin/sh
#
gcc -Wall -m32 -g -I. tcp.c -c -o tcp.o
gcc -Wall -m32 -g -I. tcp_comm.c -c -o tcp_comm.o
gcc -Wall -m32 -g -I. cal_test.c -c -o cal_test.o
gcc -Wall -m32 -g -I. power_cal.c -c -o power_cal.o
gcc -m32 -g cal_test.o tcp_comm.o tcp.o power_cal.o -o cal_test
