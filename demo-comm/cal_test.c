/* %W% %G% */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <comm.h>
#include <fastmath.h>

extern void do_cal();


static void comm_exit(void)
{
    fprintf(stderr, "Shutting down laser connections.\n");
    comm_close_port(&comm_port, 1);
 }


int main(int argc, char *argv[])
{
    char *hostname = NULL;

    comm_debug = COMM_DBG_ON /* | COMM_DBG_COM | COMM_DBG_TIME */ ;

    if (argc > 1) hostname = argv[1];

    if (comm_init_port(hostname, "LASER_HOST", 10001, &comm_port) != 0) 
       {
	fprintf(stderr, "Can't open communication with laser controller\n");
	exit(1);
	}

    atexit(comm_exit);

    if (comm_open_port() != 0) 
       {
    	exit(1);
	}

    do_cal();
    return(0);
 }
