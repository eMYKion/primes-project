/* @(#)ipg-tcp.h	1.1 04/10/17 */

#ifndef __TCP_H__
#define __TCP_H__

#ifndef _LIMITS_H
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#include <limits.h>
#endif

#ifndef _NETDB_H
#include <netdb.h>
#endif
#ifdef __linux__
#include <linux/param.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define taskLock() ;        /* nop for Sun stuff */
#define taskUnlock() ;      /* nop for Sun stuff */
#define taskIdSelf getpid

#ifndef max
#define max(x, y)	(((x) < (y)) ? (y) : (x))
#endif

#ifndef min
#define min(x, y)	(((x) < (y)) ? (x) : (y))
#endif

/* NOTE:  When a connection is broken by one side closing a socket, the side
   reading on the socket will first drain any data in the socket, and then
   'read' will return 0 (EOF).       */

#define tcpShouldRetry(stat) ((stat < 0) && \
    	    	  (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))

#define tcpWouldBlock(stat) ((stat < 0) && (errno == EWOULDBLOCK))

#define tcpConnDropped(stat) ((stat == 0) || \
    	    	        (stat < 0 && (errno == EBADF || errno == ENOENT \
		     	    	   || errno == EPIPE || errno == ECONNRESET \
    	    	    	    	   || errno == ETIMEDOUT)))

extern void tcperror(char *msg);
extern int  tcpAccept(int cfd, int *id, int *ofd, int *efd, void *client_data);
  extern int  tcpAttach(char *name, unsigned int progID, int id, int *ofd, int *efd, int client_type, int nonblock, int quiet);
extern void tcpClose(int fd);
extern int  tcpClosedHook(int fd, void (*cf) ());
extern void tcpExit(int status, void *pid);
extern int  tcpIsSuspended(int fd);
extern int  tcpGetCurrClientId(void);
extern int  tcpGetData(int fd, char *msg, int mlen);
extern unsigned int tcpGetProgId(void);
extern void *tcpGetUserData(void);
extern int  tcpPublicSocket(unsigned int progID);
extern int  tcpRecv(int fd, char *msg, int len, int timeout);
extern int  tcpRecv2(int fd, char *msg, int len, int timeout);
extern void tcpResumeFd(int fd);
extern void tcpResumeAll(void);
extern int  tcpSend(int fd, char *msg);
extern void tcpSuspendFd(int fd);
extern int  tcpNbytes(int fd);
extern void ipg_perror(char *s);

extern void tcpSrv(unsigned int progID, 
			void (*callbackfcn)(), 
			void (*opensockfcn)(),
			void (*closesockfcn)(), 
			int timeout, 
			void (*timeoutfcn)(),
			void *user_data);

extern long long ipg_systime(void);
extern void      ipg_trim_right(char *str);
extern void      ipg_trim_left(char *s);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_H__ */
