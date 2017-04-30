/* %W% %G% */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <comm.h>
#include <fastmath.h>


static void comm_exit(void)
{
    fprintf(stderr, "Shutting down laser command connection.\n");
    comm_close_port(&comm_port, 1);
 }


int main(int argc, char *argv[])
{
    char buf[32736], *hostname = NULL;
    double dval = 0;
    int i;

    comm_debug = COMM_DBG_ON /* | COMM_DBG_COM | COMM_DBG_TIME */ ;

    if (argc > 1) hostname = argv[1];

    if (comm_init_port(hostname, "LASER_HOST", 10001, &comm_port) != 0) 
       {
	fprintf(stderr, "Can't open communication with Galil controller\n");
	exit(1);
	}

    atexit(comm_exit);

    if (comm_open_port() != 0) 
       {
    	exit(1);
	}

    if (comm_execute_cmd("EMON\r", buf, sizeof(buf)) != 0)
       {
	fprintf(stderr, "Failed to turn emission ON\n");
	exit(1);
	}

    for (i = 0; ; i++)
       {
	if (comm_get_variable("STA", &dval, 0) != 0)
	   {
	    fprintf(stderr, "Failed to read laser status\n");
	    exit(1);
	    }

	fprintf(stdout, "Laser status is: 0x%X\n", (int)dval);

	if ((int)dval & 0x1) break;

	if (i >= 10)
	   {
	    fprintf(stderr, "Failed to turn emission ON\n");
	    break;
	    }
	    
	usleep(100000);
	}

    if (comm_get_variable("RCT", &dval, 0) != 0)
       {
	fprintf(stderr, "Failed to read RCT\n");
	exit(1);
        }

    fprintf(stdout, "Laser case temperature - %.2f\n", dval);

    if (comm_execute_cmd("EMOFF\r", buf, sizeof(buf)) != 0)
       {
	fprintf(stderr, "Failed to turn emission OFF\n");
	exit(1);
	}

    if (comm_get_variable("STA", &dval, 0) != 0)
       {
	fprintf(stderr, "Failed to read laser status\n");
	exit(1);
	}

    if ((int)dval & 0x1)
       {
	fprintf(stderr, "Failed to turn emission OFF\n");
	}
	
    fprintf(stdout, "Laser status is: 0x%X\n", (int)dval);
    return(0);
 }
