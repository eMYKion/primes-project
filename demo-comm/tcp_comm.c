/* @(#)tcp_comm.c	1.1 04/10/17 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <fcntl.h>
#else
#include <sys/filio.h>
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

#include <fastmath.h>
#include <comm.h>

int       comm_debug    = COMM_DBG_ON /* | COMM_DBG_COM | COMM_DBG_OFF */ ;
int       comm_nbytes   = 0;
long long comm_cmd_time = 0;

char serial_number[128];

#ifdef linux
#define PING_FMT     "ping %s -c 1 -W 1 1>/dev/null 2>&1"
#else
#define PING_FMT    "/usr/sbin/ping %s 1 1>/dev/null 2>&1"
#endif    

/* Description of the TCP/IP port used to talk to the Tcp comm: */
TCP_PORT_DEF comm_port = { -1, TCPPORT_CLOSED, 0, DEFAULT_HOST, CONTROL_PORT, {COMM_COMM_TIMEOUT, 0}, "\r", 1 };

static char *signames[] = {"",
	"HUP (hangup)",
	"INT (interrupt)",
	"QUIT (quit)",
	"ILL (illegal instruction)",
	"TRAP (trace trap)",
	"ABRT (abort)",
	"EMT (emulator trap)",
	"FPE (arithmetic exception)",
	"KILL (kill)",
	"BUS (bus error)",
	"SEGV (no mapping at the fault address)",
	"SYS (bad argument to system call)",
	"PIPE (write on a pipe or other socket with no one to read it)",
	"ALRM (alarm clock)",
	"TERM (software termination signal)",
	"URG (urgent condition present on socket)",
	"STOP (stop)",
	"TSTP (stop signal generated from keyboard)",
	"CONT (continue after stop)",
	"CHLD (child status has changed)",
	"TTIN (background read attempted from control terminal)",
	"TTOU (background write attempted to control terminal)",
	"IO (I/O is possible on a descriptor)",
	"XCPU (cpu time limit exceeded)",
	"XFSZ (file size limit exceeded)",
	"VTALRM (virtual time alarm)",
	"PROF (profiling timer alarm)",
	"WINCH (window changed)",
	"LOST (resource lost)",
	"USR1 (user-defined signal 1)",
        "USR2 (user-defined signal 2)"
};


void sig_announce_and_exit(int sig)
{
    char *signame = NULL;

    if (sig < (sizeof(signames) / sizeof(char *))) signame = signames[sig];

    if ((sig == SIGHUP) || (sig == SIGINT) || (sig == SIGTERM))
       {
	if (signame)
	    fprintf(stderr, "Program terminated by signal %s\n", signame);
	else
	    fprintf(stderr, "Program terminated by signal %d\n", sig);
	}
    else
       {
	if (signame)
	    fprintf(stderr, "Program caught signal %s", signame);
	else
	    fprintf(stderr, "Program caught signal %d", sig);
	}

    exit(sig);
 }


static void init_sig_handlers()
{
static int sig_initialized = 0;
    struct sigaction sa;        /* signal handler structure for sigaction() */

    if (sig_initialized) return;

    sig_initialized = 1;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sig_announce_and_exit;

    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);

    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
 }


/* Removes trailing whitespace from the end of a string. */

void ipg_trim_right(str)
char *str;			/* string to trim */
{
    char *s;			/* utility string ptr */

    if(*str == '\0') return;  	/* do nothing for empty strings */
    
    for (s = str + strlen (str) - 1; s > str; --s) {
	if (isspace ((int)*s))
	    *s = '\0';
	else
	    break;
    }
}


/* Removes trailing whitespace from the beginning of a string. */

void ipg_trim_left(s)
char *s;			/* string to trim */
{
    char *p;			/* utility string ptr */
    
    p = s;

    while ((*p != '\0') && isspace((int)*p)) p++;

    if (s != p ) memmove(s, p, strlen(p) + 1);
 }


int convert_line(unsigned char *p, unsigned char *s, int bufsize, int len)
{
    int slen, n;

    s[bufsize - 1] = '\0';

    for (n = 0; *p && (bufsize > 1) && (n < len); p++, s++, n++, bufsize--)
       {
	if ((int)*p < 0x20)
    	   {
	    *s =0x5e;	/* '^'*/
	    s++;
	    bufsize--;

	    if (bufsize <= 1) return(n);

	    *s = 0x40 + *p; /*conv to a letter */
	    }
    	else if ((int)*p > 0x7f)
	       {
		sprintf((char *)s, "%Xh", (int)*p); /* print hex number */
		slen = strlen((char *)s) - 1;
		s += slen;
		bufsize -= slen;

		if (bufsize <= 1) return(n);
	    	}
    	else *s = *p;
    	}

    *s = '\0';

    return(n);
 }


void print_debug_line(char *prefix, char *line, FILE *fp, int len)
{
    unsigned char buff[COMM_MAX_CMD_SIZE];
    char *p;
#if 0
    int n;

    n = 
#endif
	convert_line((unsigned char *)line, buff, sizeof(buff), len);

    if (!fp) fp = stdout;
    
    if (prefix) p = prefix;
    else        p = "";

#if 0
    fprintf(fp, "%d bytes %s%s\n", n, p, buff);
#else
    fprintf(fp, "%s%s\n", p, buff);
#endif
 }


static int get_comm_host(char *hostname, char *envname, int defport, char *host, int *port)
{
    int portnum , len;
    char *end, *h;

    if (port != NULL) *port = -1; /* Initialize port to an illegal value. */

    if (envname != NULL)
       {
	h = getenv(envname);

	if (h != NULL) hostname = h;
	}

    if (hostname == NULL) return(1); 

    if ((end = strrchr(hostname, ':')) != NULL)
       {
	if (end[1] && sscanf(&end[1], "%d", &portnum) != 1)
    	   {
    	    fprintf(stderr, "Invalid tcp port %s\n", end);
	    portnum = defport;
	    }		
	}
    else
       { 
	portnum = defport;
	end = &hostname[strlen(hostname)];
	}

    if ((len = end - hostname) <= 0) return(1);

    strncpy(host, hostname, (size_t)len);
    host[len]='\0';

    if (portnum <= 0) return(1);

    if (port != NULL) *port = portnum;

    return(0);
 }


/* You have to run comm_init_port before to make sure the name is set */
int host_is_alive(char *host)
{
    char ping_cmd[256];

    sprintf(ping_cmd, PING_FMT, host);

    if (system(ping_cmd) == 0) return(1);

    fprintf(stderr, "Failed to \"ping\" host \"%s\".\n", host);
    return(0);
 }


int comm_cmd_drain(TCP_PORT_DEF *tcp_port, int timeout)
{
    char err_msg[COMM_MAX_CMD_SIZE], msg[256];
    int bytes, try = 0, bytes_read = 0, verbose = 1;
    long long stime;

    if (tcp_port->fd < 0) return(0);
    
    if (timeout < 0)
       {
	timeout = -timeout;
	verbose = 0;
	}

    stime = ipg_systime();

    /* Read data using zero time-out (non-blocking read). */ 
    while ((bytes = tcpRecv2(tcp_port->fd, err_msg, sizeof(err_msg) - 1, 0)) >= 0)
       {
	if (bytes == 0)
	   {
	    if (++try >= timeout) break;

	    usleep(10000);
	    continue;
	    }
	
	try = 0;

	if (verbose)
	   {
	    err_msg[bytes] = '\0';
	    sprintf(msg, "%lld ms, %d bytes: laser drain<", ipg_systime() - stime, bytes);
	    print_debug_line(msg, err_msg, stdout, bytes);
	    }

    	bytes_read += bytes;
	}

    if (bytes_read && (comm_debug & COMM_DBG_COM))
	fprintf(stderr, " total bytes drained - %d\n", bytes_read);

    if (bytes < 0) 
       { 
	comm_close_port(tcp_port, 0);
	fprintf(stderr, "laser_cmd_drain: error reading command port\n");
	return(-1);
     	}

    return(0);
 }


/* Closes communications channel opened with open_port */

int comm_close_port(TCP_PORT_DEF *tcp_port, int drain)
{
    struct timeval tm;

    tm.tv_sec  = 0;
    tm.tv_usec = 10000;

    if (tcp_port->state != TCPPORT_CLOSED) 
       {
	if (tcp_port->fd >= 0)
    	   {
	    if (tcp_port == &comm_port)
		 fprintf(stderr, "Closing laser command connection.\n");
	    else fprintf(stderr, "Closing laser dictionary connection.\n");

	    if (drain)
	       {
		if (comm_cmd_drain(tcp_port, 50) < 0) return(-1);
	        }

	    if (close(tcp_port->fd) != 0)
	       {
		ipg_perror("tcpCloseSocket");
		}
    	    }

    	/* A 10 ms wait for socket to clean up */
    	select(0, NULL, NULL, NULL, &tm);
    	}
    
    tcp_port->fd = -1;
    tcp_port->state = TCPPORT_CLOSED;

    return(0);    
 }


/* Copy input data into tcp_port struct so we can close it an reopen */

int comm_init_port(char *hostname, char *envname, int defport, TCP_PORT_DEF *tcp_port)
{
    char laser_host[64];
    int laser_portnum;

    init_sig_handlers();

    if (get_comm_host(hostname, envname, defport, laser_host, &laser_portnum))
       {
	fprintf(stderr, "Hostname or port number was not specified.\n");
	return(1);
	}

    strcpy(tcp_port->name, laser_host);

    tcp_port->port = laser_portnum;

    comm_close_port(tcp_port, 1); /* just in case*/

    if (!host_is_alive(tcp_port->name)) return(1);

    return(0);
 }


static int comm_write(char *command, int len, char *errmsg)
{
    char msg[200], *dmsg, *func = "comm_write";
    int n;

    if (len == 0) return(0);

    if (comm_port.fd < 0) return(COMM_NO_CONNECTION);

    if (comm_debug & COMM_DBG_COM)
       {
	if (*errmsg) dmsg = "comm terminator>";
	else	     dmsg = "comm>";

	print_debug_line(dmsg, command, stdout, len);
        }

    if ((n = write(comm_port.fd, command, len)) != len)
       {
	if (n < 0)
	   {
	    if (tcpConnDropped(n)) 
	         sprintf(msg, "%s: connection lost during %swrite", 
							func, errmsg);
	    else sprintf(msg, "%s: %swrite failed", func, errmsg);

	    ipg_perror(msg);
    	    comm_close_port(&comm_port, 1);
	    }

	fprintf(stderr, 
		"%s: wrong number of bytes (%d) written during %swrite.\n", 
							   func, n, errmsg);
	return(-1);
     	}

    return(n);
 }


/* Write string to port */

static int write_string(char *command)
{
    unsigned int len;
    char *p, *str;
    int num = comm_port.num_term;

    len = strlen(command);
    
    if (len)
       {
	if (comm_write(command, len, "") < 0) return(-1);

    	if (len >= num)
    	   {
    	    str = &command[len - num]; 
	    
    	    for (p = comm_port.termin, len = 0; *p; p++, str++)
    	       {
		if (*p != *str) 
    	    	   {
		    len = num;
		    break;
	    	    }
	    	}	    
	    }
    	else len = num; /* force writing terminator */
	}
    else len = num;

    return(comm_write(comm_port.termin, len, "second "));
 }


/* Read one line */

static int read_line(char *reply, size_t reply_size, char ack, int timeout)
{
    int nbytes_now, nbytes_read, ret, n, fd, ack_received = 0;
    char buff[COMM_MAX_CMD_SIZE], *s, *p;
    unsigned char msg[64];
    struct timeval tv, ts;
    fd_set rd;

    if ((fd = comm_port.fd) < 0) return(COMM_NO_CONNECTION);
    
    if (ack == '\0') ack = '\r';

    if (timeout == 0) tv = comm_port.tv;
    else
       {
    	tv.tv_sec  = timeout;
    	tv.tv_usec = 0;
        }
    
    nbytes_read = 0;

    while (1)
       {
	while (1)
	   {
            FD_ZERO(&rd);
            FD_SET(fd, &rd);
	    ts = tv;

            if ((ret = select(fd + 1, &rd, NULL, NULL, &ts)) >= 0) 
							     break;
	    if (errno == EINTR) continue;

	    if (nbytes_read > 0) 
			    break;

    	    ipg_perror("Tcp comm: select failed");
    	    comm_close_port(&comm_port, 0);
            return(-1);
	    }
   
        if (ret == 0) /* Select call timed out. */
           {
	    if (nbytes_read > 0)
	       {
		if (comm_debug & COMM_DBG_COM) fprintf(stderr, 
			"laser timed out after %ld ms and receiveing %d bytes.\n",  
			      tv.tv_sec * 1000 + tv.tv_usec / 1000, nbytes_read);
		break;
		}

	    if (comm_debug & COMM_DBG_COM) fprintf(stderr, 
			"laser timed out after %ld ms without reading anything.\n",  
			      		 tv.tv_sec * 1000 + tv.tv_usec / 1000);
            return(COMM_ERR_TIMEOUT); /* We didn't read anything. */
            }

        /* If we are here, there should be something to read
	   because select did not time out. */
	while (reply_size > (nbytes_read + 1))
	   {
	    if (ioctl(fd, FIONREAD, (int)&nbytes_now) < 0)
	       {
		ipg_perror("Tcp comm: Ioctl FIONREAD error");
		comm_close_port(&comm_port, 0);
		return(COMM_IOCTL_FAIL);
		}
        
	    if (nbytes_now <= 0) /* There is nothing to read this time. */
	       {
		if (ret == 0) break; /* It's not just after select call. */

		fprintf(stderr, "Tcp comm: No bytes to read\n");
 		comm_close_port(&comm_port, 1);
		return(-1); /* This can happen only if Comm was disconnected. */
		}

	    ret = 0; /* Indicate that it's not just after select call. */

    	    if (nbytes_now > sizeof(buff))
	       {
		fprintf(stderr,
			"Tcp comm: read buffer %d too small - need %d\n",
			     sizeof(buff), nbytes_now);
		nbytes_now = sizeof(buff);
	    	}

	    
    	    for (p = buff; nbytes_now > 0; p += n, nbytes_now -= n)
	       {		
    	    	if ((n = read(fd, p, nbytes_now)) <= 0)
    	    	   {
		    if (tcpShouldRetry(n)) continue;

		    if (tcpConnDropped(n))
			 ipg_perror("Tcp comm: connection lost during read");
		    else ipg_perror("Tcp comm: read failed");

    	    	    comm_close_port(&comm_port, 0);
    	    	    return(-1);
	    	    }
		}

	    /* Copy bytes read into the appropriate buffers. */
	    for (s = buff; s < p; s++)
	       {
		if (nbytes_read < (reply_size - 2))
			reply[nbytes_read++] = *s;
		else fprintf(stderr,
			"Tcp comm: reply buffer %d too small - need %d\n",
				     reply_size, reply_size + (p - s) + 2);
		}
	    }

    	if (reply[nbytes_read - 1] == ack) break;

    	tv.tv_sec  = 0;	     /* Shorten timeout for next time; */
    	tv.tv_usec = 500000; /* 500 ms */
	}

    comm_nbytes += nbytes_read;

    if (comm_debug & COMM_DBG_COM)
	print_debug_line("laser<", reply, stdout, nbytes_read);

    reply[nbytes_read--]   = '\0';

    if (reply[nbytes_read] == ack)
       {
	ack_received = 1;
	reply[nbytes_read--] = '\0';
	}

    while ((nbytes_read >= 0) && isspace((int)reply[nbytes_read]))
					reply[nbytes_read--] = '\0';
    if (strchr(reply, '?'))
       {
	fprintf(stderr, "Received \"?\" reply from laser.\n");
	return(COMM_REPLY_ERROR);
	}

    if (!ack_received)
       {
	convert_line((unsigned char *)&ack, msg, sizeof(msg), 1);

	fprintf(stderr, "Ack byte (%s) was not received from laser.\n", msg);
	return(COMM_CMD_ERROR);
	}
/*
    if (strchr(reply, (int)ack))
       {
	 convert_line((unsigned char *)&ack, msg, sizeof(msg), 1);

	fprintf(stderr, "Received multiple ack byte (%s) from laser.\n", msg);
	return(COMM_REPLY_ERROR);
	}
*/
    return(nbytes_read + 1);
 }


/* returns file descriptor or (-1) */

int open_tcp_port(char *name, int port)
{   
  int fd, opt, i;
    size_t len;
    struct sockaddr_in sadrin;
    char *ip, mess[50];
    struct linger linger;
    struct hostent *hp;

    if (!name || !name[0] || (port < 1)) 
       {
	fprintf(stderr, "Tcp comm: hostname is not defined\n");
    	return(-1);
	}

    /* Create the address of the server */
    len = sizeof(struct sockaddr_in);
    
    memset(&sadrin, 0, len);
    
    sadrin.sin_family = AF_INET;
    sadrin.sin_port = htons(port);

    /* Look up our host's network address */
    if ((hp = gethostbyname(name)) == NULL)
       {
	fprintf(stderr, "tcp_client: Unknown host: %s\n", name);
        return(-1);
    	}

    ip = *hp->h_addr_list;

    memcpy(&sadrin.sin_addr, ip, (size_t)hp->h_length);

    /* Create a socket in the INET domain */
    
    if ((fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) < 0)
       {
        ipg_perror("tcp_client:create socket error");
        return(-1);
	}
 
    opt = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt)) != 0)
       {
        ipg_perror("tcp_client:setsocket KEEPALIVE failed");
    	}

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
       {
	ipg_perror("tcp_client:setsocket REUSEADDR failed");
	}

    linger.l_onoff  = 1;
    linger.l_linger = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&linger,
	    	    	    	    	    	sizeof(linger)) < 0)
       {
        ipg_perror("tcp_client:setsocket LINGER failed");
        }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
       {
	ipg_perror("tcpConnectSocket: can't set O_NONBLOCK to socket");
    	close(fd);
        return(-1);
        }

    /* Connect to the server with 2 seconds timeout */
    for (i = 0; i < 200; i++)
       {
	if (connect(fd, (struct sockaddr *)&sadrin, sizeof(struct sockaddr_in)) >= 0)
	   {
	    if (i) printf("open_tcp_port success after %d attempts.\n", i);
	    usleep(100000);
	    return(fd);
	    }

	usleep(10000);
        }

    sprintf(mess, "Error connecting to %s on port %d", name, port);
    ipg_perror(mess);
    close(fd);
    return(-1);
 }


int comm_get_variable(char *variable, double *value, int quiet)
{
    char cmd[COMM_MAX_CMD_SIZE], response[COMM_MAX_CMD_SIZE], *s;
    int ret = -1;

    if ((variable == NULL) || (value == NULL)) return(-1);

    sprintf(cmd, "%s\r", variable);

    if (comm_execute_cmd(cmd, response, sizeof(response)) == 0)
       {
	if ((s = strstr(response, variable)))
	   {
	    s += strlen(variable) + 1;
	    ipg_trim_left(s);

	    ret = sscanf(s, "%lf", value);
	    }
	}

    if (ret != 1)
       {
	if (quiet) printf("Failed to get laser variable \"%s\".\n", variable);
	else fprintf(stderr, "Failed to get laser variable \"%s\".\n", variable);
	return(-1);
	}

    return(0);
 }


int comm_open_port()
{    
static int in_open = 0;
    int silent = 1;
    char buf[32736], *p, *s, *err_msg;

    if (comm_debug) silent = 0;
    
    if ((comm_port.state == TCPPORT_OPEN) && (comm_port.fd >= 0)) return(0);

    if (in_open)
       {
	fprintf(stderr, "comm_open_port was called recursively.\n");
	return(-1);
	}

    in_open = 1;
    serial_number[0] = '\0';

    if ((comm_port.fd = open_tcp_port(comm_port.name, comm_port.port)) < 0)
       {
	comm_port.state = TCPPORT_CLOSED;
	comm_port.fd    = -1;
	in_open = 0;
	return(-1);
	}

    comm_port.state	= TCPPORT_OPEN;
    comm_port.tv.tv_sec	= COMM_COMM_TIMEOUT;
    comm_port.tv.tv_usec	= 0;
     
    if (!silent)
	fprintf(stdout, "Connected to %s port %d\n", comm_port.name, comm_port.port);

    if (write_string("\r\r") < 0)
       {
	err_msg = "Tcp comm failed to write command";
	goto OPEN_FAILED;
    	}
    
    comm_cmd_drain(&comm_port, 10);

    /* get info and get firmware versiton */
    s = NULL;

    if (comm_execute_cmd("RFV\r", buf, sizeof(buf)) == 0)
       {
	p = "RFV:";

	if ((s = strstr(buf, p)))
	   {
	    s += strlen(p);

	    for (p = s; (*p != '\0') && (*p != '\r') && (*p != '\n'); p++);

	    *p = '\0';
	    }
    	}

    if (s == NULL)
       {
	err_msg = "Failed to read firmware version";
	goto OPEN_FAILED;
	}

    ipg_trim_left(s);
	
    if (!silent) fprintf(stdout, "Laser firmware: %s\n", s);

    /* get info and get firmware versiton */
    s = NULL;

    if (comm_execute_cmd("RSN\r", buf, sizeof(buf)) == 0)
       {
	p = "RSN:";

	if ((s = strstr(buf, p)))
	   {
	    s += strlen(p);

	    for (p = s; (*p != '\0') && (*p != '\r') && (*p != '\n'); p++);

	    *p = '\0';
	    }
    	}

    if (s == NULL)
       {
	err_msg = "Failed to read serial number";
	strcpy(serial_number, "ND");
	}
    else
       {
	ipg_trim_left(s);
	ipg_trim_right(s);
	strcpy(serial_number, s);
	
	if (!silent) fprintf(stdout, "Laser S/N: %s\n", s);

	if (strstr(serial_number, "1234567")) strcpy(serial_number, "ND");
        }

    in_open = 0;
    return(0);

OPEN_FAILED:
    in_open = 0;
    fprintf(stderr, "comm_open_port: %s.\n", err_msg);
    comm_close_port(&comm_port, 0);
    return(-1);
 }


static int comm_one_cmd(char *cmd, char *reply, size_t reply_size)
{    
    int ret;

    if (write_string(cmd) < 0)
       {
	fprintf(stderr, "Tcp comm failed to write command %s\n", cmd);
    	return(COMM_CMD_ERROR);
    	}
    
    if (reply == NULL) return(0);

    reply[0] = '\0';

    ret = read_line(reply, reply_size, 0, 0);

    if (ret > 0) ret = 0; /* read_line returns number of bytes read - no error. */

    return(ret);
 }


int comm_execute_cmd(char *command, char *reply, size_t reply_size)
{   
    char buff[COMM_MAX_CMD_SIZE], cmd[COMM_MAX_CMD_SIZE], *p, *s;
    int ret = 0;
    
    if (comm_port.state != TCPPORT_OPEN)  
       {
	/* I expect it to be allready set comm_init_port(NULL, -1) */
	if (comm_open_port() != 0) 
    	   {
    	    return(COMM_NO_CONNECTION);
	    }
	}

    comm_nbytes = 0;

    if (comm_debug & COMM_DBG_TIME) comm_cmd_time = ipg_systime();
    
    /* There may be several commands in command line, parse then and execute
       one by one, we need to know exactly what command we execute, to figure
       what reply to expect */     
    if (strlen(command) > COMM_MAX_CMD_SIZE)
       {
	fprintf(stderr, "Tcp comm: Command %s too long\n", command);
	return(COMM_CMD_ERROR);
    	}

    strcpy(cmd, command);
    ipg_trim_left(cmd);

    s = cmd;
    
    while (*s)
       {
	for (p = buff; *s; p++)
    	   {
	    *p = *s++; 

    	    if ((*p == ';') || (*p == '\n')) break;
	    }

	*p = '\0';

    	if (p != buff) /* command is not empty */
    	   {
	    comm_cmd_drain(&comm_port, 0);
            ret = comm_one_cmd(buff, reply, reply_size);

	    if (ret < 0) break;
	    }
    	}

    if ((comm_debug & COMM_DBG_COM) &&
	(comm_debug & COMM_DBG_TIME))
    	{	
    	 comm_cmd_time = ipg_systime() - comm_cmd_time;
    	 fprintf(stdout, 
    	    "LaserCmdTime %lld ms; Nbytes %d; Speed %f kB/sec\n", 
    	    	comm_cmd_time, comm_nbytes, 
	    (double)comm_nbytes / comm_cmd_time / 1.024);
	}

    return(ret);
 }
