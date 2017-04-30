/* %W% %G% */

#include <stdio.h>
#include <unistd.h>

#include <fastmath.h>

#include <comm.h>

#define PCAL_TIME_STEP	(60000)
#define PCAL_NSTEPS	(5)

static long long last_time = 0;

static int power_min, power_max, em_state = 0, proc_done = 1;
static double power_step, cur_setpoint = 0.0;

extern int power_pd_cal(char *iname, int power_min, int power_max, double *pdlvl, double *pdpwr);


static int emission_status(int quiet)
{
    double dval;
    int status;

    if (comm_get_variable("STA", &dval, 0) != 0)
       {
	fprintf(stderr, "Failed to read laser status\n");
	return(-1);
	}

    status = iround(dval);

    if (!quiet) fprintf(stdout, "Status is: 0x%X\n", status);

    return(status & 0x1);
 }


static int emission_on()
{
    char buf[32736];
    int status;

    if (comm_execute_cmd("RERR\r", buf, sizeof(buf)) != 0)
       {
	fprintf(stderr, "Failed to reset errors\n");
	return(-1);
	}

    usleep(100000);

    if (comm_execute_cmd("EMON\r", buf, sizeof(buf)) != 0)
       {
	fprintf(stderr, "Failed to turn emission ON\n");
	return(-1);
	}

    usleep(1000000);

    status = emission_status(0);

    if (status != 1)
    	    fprintf(stderr, "Failed to turn emission ON\n");
    return(status);
 }


static int emission_off()
{
    char buf[32736];
    int status;

    if ((status = emission_status(0)) == 0) return(status);

    if (comm_execute_cmd("EMOFF\r", buf, sizeof(buf)) != 0)
       {
	fprintf(stderr, "Failed to turn emission OFF\n");
	return(-1);
	}

    usleep(1000000);

    status = emission_status(0);

    if (status != 0)
    	    fprintf(stderr, "Failed to turn emission OFF\n");
    return(status);
}


int read_laser_power(double *dval)
{
    double pmin, pmax;

    if (comm_get_variable("RPS", dval, 1) == 0)
       {
	if (comm_get_variable("RNP", &pmin, 0) == 0)
	   power_min = iround(pmin);

	if (comm_get_variable("RMP", &pmax, 0) == 0)
	   power_max = iround(pmax);

	return(0);
	}

    fprintf(stderr, "Failed to read laser power\n");
    return(-1);
 }


int set_laser_power(double value)
{
    char buf[32736], cmd[256];
    double dval;

    if (read_laser_power(&dval) != 0) return(-1);

    fprintf(stdout, "Setting power to %.2f", value);

    fprintf(stdout, "W\n");

    if (value < power_min) value = power_min;
    else if (value > power_max) value = power_max;

    sprintf(cmd, "%s %.2f\r", "SPS", value);

    if (comm_execute_cmd(cmd, buf, sizeof(buf)) != 0)
       {
	fprintf(stderr, "Failed to set power\n");
	return(-1);
	}

    if (read_laser_power(&dval) != 0) return(-1);
    
    if (iround(dval) != iround(value))
	fprintf(stderr, "Read back power value %d differ from the set value %.2f.\n", iround(dval), value);

    return(0);
 }


static void power_cal(long long cur_time, int first_time)
{
    double pwr, dval;

    if (first_time)
       {
	proc_done = 0;
	read_laser_power(&dval);
	power_step = ((double)power_max - (double)power_min - 0.01) / PCAL_NSTEPS;

	if (power_step < 1) power_step = 1; 

	last_time = cur_time;
	cur_setpoint = 0.0;

	if (set_laser_power(cur_setpoint) < 0) goto pcal_err;

	emission_off();
	em_state = 0;
	}

    if (em_state == 1)
       {
	if (comm_get_variable("ROP", &pwr, 0) != 0)
	   {
	    fprintf(stderr, "Failed to read module output power\n");
    	    goto pcal_err;
    	    }

	fprintf(stdout, "Setpoint = %.2f W, Output = %.3f W\n", cur_setpoint, pwr); 
	}

    if ((cur_time < 10000) || proc_done) return;

    if (em_state == 0)
       {
	cur_setpoint = power_min;

	if (set_laser_power(cur_setpoint) < 0) goto pcal_err;

	if (emission_on() < 0) goto pcal_err;

	em_state = 1;
	last_time = cur_time;
	return;
	}

    if ((cur_time - last_time) < PCAL_TIME_STEP) return;

    last_time = cur_time;
    cur_setpoint += power_step;

    if (cur_setpoint > power_max)
       {
	cur_setpoint = 0.0;
	proc_done = 1;
	}

    if (set_laser_power(cur_setpoint) >= 0)
       {
	if (proc_done) goto pcal_done;

	return;
	}

pcal_err:
    fprintf(stderr, "Power calibration error.\n");
pcal_done:
    emission_off();
    em_state = 0;
    proc_done = 1;
 }


void do_cal()
{
    long long cur_time, start_time;
    int first_time = 1;

    start_time = ipg_systime();

    while (1)
       {
	cur_time = ipg_systime() - start_time;

	power_cal(cur_time, first_time);

	if (proc_done) return;

	first_time = 0;
	usleep(1000000);
	}
}
