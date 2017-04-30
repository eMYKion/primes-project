/* @(#)comm.h	1.1 04/10/17 */

#ifndef __TCP_H__
#include <ipg-tcp.h>
#endif

#define TCPPORT_OPEN   1
#define TCPPORT_CLOSED 0

#define COMM_COMM_TIMEOUT   6 /* sec */
#define COMM_MAX_CMD_SIZE   32768

#define COMM_REPLY_ERROR    -2

#define COMM_ERR_TIMEOUT    -50
#define COMM_CMD_ERROR      -49
// #define COMM_SELECT_FAIL    -48
#define COMM_IOCTL_FAIL     -47

// #define COMM_ERR_NO_DEVICE  -45
// #define COMM_ERR_WRITE      -44
#define COMM_NO_CONNECTION  -43

/* Debug flag bitmask  */
#define COMM_DBG_OFF   0
#define COMM_DBG_ON   	1   /* print ID string */
#define COMM_DBG_COM   2   /* print all commands/replies */
#define COMM_DBG_TIME	4   /* measure time and speed */

#define CONTROL_PORT   23    /* telnet port for this connection */

#define DEFAULT_HOST  ""

/* Description of the TCP/IP port used to talk to the Tcp Comm: */
typedef struct {
    int fd;                           /* file descriptor   */
    int state;                        /* Current state: 1 = open, 0 = closed */
    int error;                        /* error code */
    char name[30];                    /* host name    */
    int port;                         /* port number */
    struct timeval tv;                /* Timer for programming the timeout */
    char termin[4];   	    	      /* Terminating chars */
    int num_term;   	    	      /* Number Terminating chars */
} TCP_PORT_DEF;

extern TCP_PORT_DEF comm_port, dict_port;

extern int   laser_firmware;
extern char  firmware_name[128];
extern int   dict_debug;
extern int   comm_debug;

extern int   dict_open_port	(void);
extern int   dict_execute_cmd	(char *command, char *reply, size_t rsize);
extern int   dict_get_variable	(char *variable, double *value);
extern int   dict_get_error	(void);
extern int   dict_get_string_val(char *variable, char *value);

extern int   open_tcp_port     (char *name, int port);
extern int   comm_cmd_drain    (TCP_PORT_DEF *tcp_port, int timeout);
extern int   comm_init_port    (char *hostname, char *envname, int defport, TCP_PORT_DEF *tcp_port);
extern int   comm_open_port    (void);
extern int   comm_close_port   (TCP_PORT_DEF *tcp_port, int drain);
extern int   host_is_alive     (char *host);
extern int   comm_execute_cmd  (char *command, char *reply, size_t rsize);
extern int   comm_get_variable (char *variable, double *value, int quiet);
extern int   comm_get_error    (void);
extern int   convert_line      (unsigned char *p, unsigned char *s, int bufsize, int len);
extern void  print_debug_line  (char *prefix, char *line, FILE *fp, int len);
extern void  sig_announce_and_exit(int sig);
