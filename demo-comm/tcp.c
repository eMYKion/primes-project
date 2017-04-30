/* @(#)tcp.c	1.1 04/10/17 */

/* tcp utilites:

   ****************************
   * A note on initialization *
   ****************************
   Both Sun and VxWorks processes should make this call:

    	    	    on_exit(tcpExit, getpid())

   but a Sun client that doesn't use tcpPublicSocket can omit it.


   A VxWorks task must call 'rpcTaskInit()' before calling 

    	tcpPublicSocket
    	tcpAttach

    	==============================================================

    tcpPublicSocket(unsigned int progID)

    	- create a socket, and bind a port to it.  Register the port
    	  with the portmapper so other processes can connect to it.  The
    	  program number used is progID.  Make a listen call so
	  connection queues are set up.

    	    Returns:  -1  failure
	    	      fd  socket descriptor

    	  Note:  The socket created is added to a list that will be closed
    	         when the task exits, unless a call to 'tcpPermSocket' is made.
    	==============================================================

    tcpAttach(char *name, unsigned int progID)

    	- create a socket, and attach it to a socket set up by
	  'tcpPublicSocket', using progID.

    	    Returns:  -1  failure
	    	      fd  socket descriptor

    	  Note:  The socket created is added to a list that will be closed
    	         when the task exits, unless a call to 'tcpPermSocket' is made.
    	==============================================================

    tcpSend(int fd, char *msg)

    	- send a message on the connection specified by 'fd'.  The amount
	  of data is specified by 'msg->msglen'.

    	    Returns:  -1  failure
	    	       0  success

    	  For servers, when a connection is closed, the 'errno' value
	  can be either EBADF or ENOENT.

    	==============================================================

    tcpRecv(int fd, char *msg, int len)

    	- receive a message on the connection specified by 'fd'.  The parameter
	  'len' defines the size of the buffer pointed to by 'msg', including
    	  the header.  The actual amount of data received is put in
	  'msg->msglen'.

    	    Returns:  -1  failure
    	    	       0  success
		       1  buffer overflow (extra data is thrown away, as 
    	    	    	  much as possible is put in 'msg')

    	==============================================================

    tcpClosedHook(int fd, void (*cf)())

    	- register a function to be called when tcpSend/tcpRecv
    	  recognizes that a connection has been broken.

    	==============================================================

    tcpClose(int fd)

    	- Do all necessary cleanup and dispose of the socket.

    	==============================================================

    tcpConnDropped(int stat)

    	- return TRUE if the current 'errno' value indicates that the 
    	  connection on a socket has been broken.  'stat' is the return
    	  status from the read/write call.

    	==============================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#ifdef USE_RPC
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#endif

#include <errno.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <fcntl.h>
#else
#include <sys/filio.h>
#endif
#include <pwd.h>

#ifndef VX
#ifndef _LINUX_C_LIB_VERSION
/* include file with sys functions undefined by Sun */
#endif
#endif

#include <ipg-tcp.h>

static struct FDLIST {
    struct FDLIST *link;
    void (*cf)(int fdc);	/* closed connection hook, if any */
    int pid;
    int fd;
    unsigned int pnum;		/* non-zero if a RPC entry was made */
    } *fdlist = NULL;


/* Returns time in milliseconds. */

long long ipg_systime()
{
    struct timeb tp;

    ftime(&tp);
    return(1000 * (long long)tp.time + (long long)tp.millitm);
 }


void ipg_perror(char *s)
{
    if ((s != NULL) && (*s != '\0'))
	 fprintf(stderr, "%s: %s\n", s, strerror(errno));
    else fprintf(stderr, "%s\n", strerror(errno));
 }


static int hostGetByName(char *name)
{
    struct hostent *hp;
    int host_addr;
    
    if (!(hp = gethostbyname(name))) return(-1);

    bcopy(hp->h_addr_list[0], (char *)&host_addr, hp->h_length);
    return(host_addr);
 }


/* Scan the list, delink entries matching the PID, and add entries matching
   the PID value to a list for later processing.  Until we know better,
   malloc/free calls aren't allowed to be made within taskLock/taskUnlock
   blocks.
 */

void tcpExit(int status, void *pid)
{
    struct FDLIST *p, *q, *r, *newlist;
#ifndef VX
    struct passwd *pas;
    int uid;
#else
    rpcTaskInit();
#endif

    newlist = NULL;
    p = fdlist;
    q = NULL;

    taskLock();	    /* no 'free' calls allowed within taskLock/taskUnlock */

    while (p)
       {
	 if ((int)pid == p->pid)
    	   {
    	    r = p->link;
	    if (q) q->link = r;
	    else fdlist = r;

	    p->link = newlist;
	    newlist = p;

	    p = r;
	    }
    	else
    	   {
	    q = p;
	    p = p->link;
	    }
    	}      

    taskUnlock();

    while (newlist)
       {
	p = newlist->link;

    	/* Call closed socket hook */
    	if (newlist->cf)
		(*(newlist->cf))(newlist->fd);

    	if (newlist->pnum)
    	   {
#ifndef VX
    	    uid = geteuid();
    	    if (setuid(0) < 0) uid = -1;
#endif

#ifdef USE_RPC
	    printf("pmap_unset(%d,0)\n", newlist->pnum);
	    pmap_unset(newlist->pnum, 0);
#endif

#ifndef VX
    	    if (uid >= 0)
       	       {
	    	if ((pas = getpwuid(uid)))
       	    	   {
    	    	    if ((setegid(pas->pw_gid) < 0) || 
    	    	    	(seteuid(pas->pw_uid) < 0))
    	    	    	     fprintf(stderr, "Cannot set operator uid\n");
	    	    }
            	}
#endif
	    }

	close(newlist->fd);
	free((char *)newlist);

	newlist = p;
    	}
 }


static int tcpAddFd(int fd, unsigned int progID)
{
    struct FDLIST *fn;

    /* Record socket descriptor for close during exit */
    if (!(fn = (struct FDLIST *)malloc(sizeof(struct FDLIST)))) return(-1);

    fn->pid = taskIdSelf();
    fn->cf = NULL;
    fn->fd = fd;
    fn->pnum = progID;

    taskLock();
    fn->link = fdlist;
    fdlist = fn;
    taskUnlock();

    return(0);
 }


static int tcpSockOpt(int fd, int reuseaddr, int keepalive, int nodelay)
{
    struct linger linger;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
    	    	    	     (char *)&reuseaddr, sizeof(int)) < 0) return(-1);

    if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY,
    	    	    	     (char *)&nodelay,  sizeof (int)) < 0) return(-1);
    
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
		    	     (char *)&keepalive, sizeof(int)) < 0) return(-1);
    linger.l_onoff  = 1;
    linger.l_linger = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, 
			     (char *)&linger, sizeof(linger)) < 0) return(-1);
    return(0);
 }


static int tcpConnectSocket(struct sockaddr_in *addr, int nonblock, int quiet)
{
    char *errmsg;
    int fd, i;
    
    /* make a socket, and attempt to connect */
    if ((fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) < 0)
       {
    	errmsg = "tcpConnectSocket: can't create socket";
	goto CSOCKET_ERROR2;
    	}

    if (tcpSockOpt(fd, 1, 0, 1) < 0)
       {
    	errmsg = "tcpConnectSocket: setsockopt";
	goto CSOCKET_ERROR;
    	}

    if (nonblock)
       {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	   {
    	    errmsg = "tcpConnectSocket: can't set O_NONBLOCK to socket";
	    goto CSOCKET_ERROR;        
	    }
        }

    for (i = 0; ; i++)
       {
	if (connect(fd, (struct sockaddr *)addr, sizeof(struct sockaddr)) >= 0)
	   {
	    if (i) printf("tcpConnectSocket success after %d attempts.\n", i);
	    break;
	    }

	if ((i > 1000) || !nonblock)
	   {
	    errmsg = "tcpConnectSocket: can't connect";
	    goto CSOCKET_ERROR;
	    }

	usleep(1000);
    	}

#ifndef VX
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
       {
    	errmsg = "tcpConnectSocket: can't set FD_CLOEXEC to socket";
	goto CSOCKET_ERROR;        
        }
#endif
    return(fd);
    
CSOCKET_ERROR:
    close(fd);

CSOCKET_ERROR2:
    ipg_perror(errmsg);
    return(-1);
 }


static int tcpAcceptSocket(struct sockaddr_in *addra)
{
    struct sockaddr_in addr;
    int fd;
    unsigned int len;
    char *errmsg;

    if ((fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) < 0)
       {
    	errmsg = "tcpAcceptSocket: can't create socket";
	goto ASOCKET_ERROR2;
    	}

    if (tcpSockOpt(fd, 1, 0, 1) < 0)
       {
    	errmsg = "tcpAcceptSocket: setsockopt";
	goto ASOCKET_ERROR;
    	}

    /* Get a port established for the socket */
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr,  sizeof(addr)) < 0)
       {
    	errmsg = "tcpAcceptSocket: can't bind port to socket";
	goto ASOCKET_ERROR;
    	}

    if (listen(fd, 5) < 0)
       {
    	errmsg = "tcpAcceptSocket: can't listen to socket";
	goto ASOCKET_ERROR;
    	}

    /* Now find out what the port is and register a program number for the
       public port.
     */
    len = sizeof(struct sockaddr);
    if (getsockname(fd, (struct sockaddr *)addra, &len) < 0)
       {
    	errmsg = "tcpAcceptSocket: can't get port of socket";
	goto ASOCKET_ERROR;
    	}

#ifndef VX
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
       {
    	errmsg = "tcpAcceptSocket: can't set FD_CLOEXEC to socket";
	goto ASOCKET_ERROR;        
        }
#endif
    return(fd);
    
ASOCKET_ERROR:
    close(fd);

ASOCKET_ERROR2:
    ipg_perror(errmsg);
    return(-1);
 }


#if 0

/* Create a socket and make it available for connections.  Return the 
   socket file descriptor to the caller.  The caller is responsible for
   issuing an 'accept' call on the socket.
 */

int tcpPublicSocket(unsigned int progID)
{
    struct sockaddr_in addr;
    char *errmsg;
#ifdef USE_RPC
    char buff[200];
#endif
    unsigned short port;
    int fd;

    if ((fd = tcpAcceptSocket(&addr)) < 0) return(-1);

    port = htons(addr.sin_port);    

#ifdef USE_RPC
    if (!pmap_set(progID, 0, IPPROTO_TCP, port))
       {
    	sprintf(buff,
    	    	    "tcpPublicSocket: can't assign port %d to program %X",
    	    	    	    	    	    	       addr.sin_port, progID);
    	errmsg = buff;
	goto SOCKET_ERROR;
    	}
#else
    port = progID;
#endif

    /* Record socket descriptor for close during exit */
    tcpAddFd(fd, progID);
    return(fd);

#ifdef USE_RPC
SOCKET_ERROR:
#endif
    close(fd);
    ipg_perror(errmsg);
    return(-1);
 }


int tcpAccept(int cfd, int *id, int *ofd, int *efd, void *client_data)
{
    unsigned int addrlen;
    int s;
    struct sockaddr addr;
    struct sockaddr_in from;
    unsigned short outport, errport;
    char *errmsg = NULL;
    
    addrlen = sizeof(addr);
    if ((s = accept(cfd, &addr, &addrlen)) < 0)
       {
    	errmsg = "tcpAccept: accept failed";
	goto ACCEPT_ERROR2;
	}

    addrlen = sizeof(from);
    if (getpeername(s, (struct sockaddr *)&from, &addrlen) < 0)
       {
    	errmsg = "tcpAccept: getpeername failed";
	goto ACCEPT_ERROR;
	}

    if (read(s, (char *)id, sizeof(*id)) != sizeof(*id))
       {
    	errmsg = "tcpAccept: read failed";
	goto ACCEPT_ERROR;
    	}

    if (read(s, (char *)&outport, sizeof(outport)) != sizeof(outport))
       {
    	errmsg = "tcpAccept: read failed";
	goto ACCEPT_ERROR;
    	}

    if (outport)
       {
    	from.sin_port = outport;

    	/* make a socket, and attempt to connect */
    	if ((*ofd = tcpConnectSocket(&from, 0, FALSE)) < 0) goto ACCEPT_ERROR;

    	tcpAddFd(*ofd, 0);
        }
    else *ofd = -1;
	
    if (read(s, (char *)&errport, sizeof(errport)) != sizeof(errport))
       {
    	errmsg = "tcpAccept: read failed";
	goto ACCEPT_ERROR;
    	}

    if (errport)
       {
    	from.sin_port = errport;

    	/* make a socket, and attempt to connect */
    	if ((*efd = tcpConnectSocket(&from, 0, FALSE)) < 0) goto ACCEPT_ERROR;

    	tcpAddFd(*efd, 0);
        }
    else *efd = -1;
	
    /* Record socket descriptor for close during exit */
    tcpAddFd(s, 0);
    return(s);

ACCEPT_ERROR:
    close(s);

ACCEPT_ERROR2:
    if (errmsg) ipg_perror(errmsg);
    return(-1);
 }

#endif

/* Create a socket, and attach it to a process.  The socket file descriptor
   is returned to the caller. */

int tcpAttach(char *name, unsigned int progID, int id, int *ofd, int *efd, int client_type, int nonblock, int quiet)
{
    char buff[200], *errmsg = NULL;
    struct sockaddr_in addr;
    struct sockaddr addra;
    int fd, fd2, fd3, host_addr;
    unsigned int len;
    unsigned short outport, errport, port;
    int process_id, proc_name_len, display_len, val;
    char *proc_name, *display;
    
    if ((host_addr = hostGetByName(name)) == -1)
       {
    	sprintf(buff, "tcpAttach: net address lookup for %s failed", name);
    	errmsg = buff;
	goto ATTACH_ERROR2;
    	}

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = host_addr;

#ifdef USE_RPC
    /* query the portmapper to find the port */
    if (!(port = pmap_getport(&addr, progID, 0, IPPROTO_TCP)))
       {
	if (!quiet)
    	    fprintf(stderr, "tcpAttach: no remote port for program %d on %s\n",
    	    	    	    	    	    	    	    	 progID, name);
    	strncpy(buff, clnt_spcreateerror("tcpAttach"), sizeof(buff) - 1);
	buff[sizeof(buff) - 1] = 0;
	fprintf(stderr, "%s\n", buff);
	goto ATTACH_ERROR2;
    	}
#else
    port = progID;
#endif

    addr.sin_port = htons(port);

    /* make a socket, and attempt to connect */
    if ((fd = tcpConnectSocket(&addr, nonblock, quiet)) < 0) return(-1);

    write(fd, (char *)&id, sizeof(id));

    if (ofd)
       {
    	if ((fd2 = tcpAcceptSocket(&addr)) < 0) goto ATTACH_ERROR;

    	outport = addr.sin_port;
    	write(fd, (char *)&outport, sizeof(outport));

    	len = sizeof(addra);
    	*ofd = accept(fd2, &addra, &len);
	close(fd2);

    	if (*ofd < 0)
       	   {
    	    errmsg = "tcpAccept: accept failed";
	    goto ATTACH_ERROR;
	    }

    	tcpAddFd(*ofd, 0);
    	}
    else
       {
    	outport = 0;
	write(fd, (char *)&outport, sizeof(outport));
    	}

    if (efd)
       {
    	if ((fd3 = tcpAcceptSocket(&addr)) < 0) goto ATTACH_ERROR;

    	errport = addr.sin_port;
    	write(fd, (char *)&errport, sizeof(errport));

    	len = sizeof(addra);
    	*efd = accept(fd3, &addra, &len);
	close(fd3);

    	if (*efd < 0)
       	   {
    	    errmsg = "tcpAccept: accept failed";
	    goto ATTACH_ERROR;
	    }

    	tcpAddFd(*efd, 0);
    	}
    else
       {
    	errport = 0;
	write(fd, (char *)&errport, sizeof(errport));
    	}

    // 4 more items get sent to the server now
    process_id = taskIdSelf();
    proc_name = "";

#ifndef VX
    if ((display = getenv("DISPLAY")) == NULL)
#endif
        display = "";

    proc_name_len = proc_name == NULL? 0 : strlen(proc_name)+1;
    display_len   = strlen(display)+1;

#if 0
    val = htonl(client_type);
#else
    val = client_type;
#endif
    write(fd, (char *)&val, sizeof(val));

#if 0
    val = htonl(process_id);
#else
    val = process_id;
#endif
    write(fd, (char *)&val, sizeof(val));

#if 0
    val = htonl(proc_name_len);
#else
    val = proc_name_len;
#endif
    write(fd, (char *)&val, sizeof(val));
    write(fd, proc_name, (unsigned int)proc_name_len);

#if 0
    val = htonl(display_len);
#else
    val = display_len;
#endif
    write(fd, (char *)&val, sizeof(val));
    write(fd, display, (unsigned int)display_len);

    /* Record socket descriptor for close during exit */
    tcpAddFd(fd, 0);
    return(fd);

ATTACH_ERROR:
    close(fd);

ATTACH_ERROR2:
    if (errmsg && !quiet) 
	ipg_perror(errmsg);
    return(-1);
 }


/* Do all necessary cleanup to close a socket. */

void tcpClose(int fd)
{
    struct FDLIST *p, *q;

    /* Scan 'fdlist' for socket entry */
    taskLock();
    p = fdlist;
    q = NULL;
    while (p && p->fd != fd)
       {
    	q = p;
	p = p->link;
    	}

    if (p)
       {
    	/* Delink record */
	if (q) q->link = p->link;
	else fdlist = p->link;    
    	}

    taskUnlock();
    
    if (p)
       {
    	if (p->cf)
	    (*(p->cf))(fd);			/* Call closed socket hook */

#ifdef USE_RPC
    	if (p->pnum) pmap_unset(p->pnum, 0);	/* de-register port */
#endif
    	free((char *)p);
    	}
    
    close(fd);
 }


/* Register a function to be called when tcpSend/tcpRecv is called and the
   connection on the socket is closed.
 */
int tcpClosedHook(int fd, void (*cf)(int fdc))
{
    struct FDLIST *p;
    int s;

    taskLock();
    p = fdlist;

    while (p && p->fd != fd) p = p->link;

    if (p)
       {
	p->cf = cf;
	s = 0;	    	/* success */
    	}
    else s = -1;	/* failure */

    taskUnlock();
    return(s);
 }


int tcpSend(int fd, char *msg) 	    	    /* send a message */
{
    char *p;
    int n;
    unsigned short ldata, lsize;

    p = (char *)msg;
    ldata = lsize = *((unsigned short *)msg);
    *((unsigned short *)msg) = htons(ldata);

    do {
	if ((n = write(fd, p, (unsigned int)ldata)) < 0)
    	   {
    	    if (!tcpConnDropped(n)) ipg_perror("tcpSend");

	    tcpClose(fd);
	    *((unsigned short *)msg) = lsize;
	    return(-1);
    	    }

    	ldata -= n;
    	p += n;
    	} while (ldata);

    *((unsigned short *)msg) = lsize;
    return(0);
 }


/* Just like tcpRecv, except that the packet length has already been read. 
   Length of zero is a valid value. This routine is guaranteed to read 
   number of bytes specified by length, so there is no real need to compare 
   returned value with length. The file descriptor will be closed in case 
   of a failure.
    	    Returns:  	 -1    	    failure
    	    	      bytes read    success
 */
int tcpGetData(int fd, char *msg, int len)
{
    int n, mlen;
    
    mlen = len;
    
    while (mlen > 0)
       {
	if ((n = read(fd, msg, (unsigned int)mlen)) > 0)
    	   {
	    mlen -= n;  	    	/* adjust bytes remaining in msg */
    	    msg  += n;	    
	    }
    	else
    	   {
            if (!tcpShouldRetry(n))
    	       {
    	    	if (!tcpConnDropped(n))
	    	   fprintf(stderr, "tcpGetData: read failed\n");
    	    	tcpClose(fd);
    	    	return(-1);
	        }
    	    }
        }

    if (mlen < 0)
       {
	fprintf(stderr, "tcpGetData read %d extra bytes\n", -mlen);
    	tcpClose(fd);
	return(-1);
    	}

    return(len);
 }


/* read the number of bytes currently available of fd */

int  tcpNbytes(int fd)
{
    int i;
  
    if (ioctl(fd, FIONREAD, (int)&i) < 0) 
       {
	tcpClose(fd);
    	return(-1);
        }

    return(i);
 }


/* 
   if timeout > 0
     read min(tcpNbytes(fd), len) into msg 
     this routine should never block
   else
     read len bytes from fd into msg

    Returns:
                  - Number of bytes read or (may be zero)
	       -1 - system error
 */

int tcpRecv2(int fd, char *msg, int len, int timeout)
{
    long long etime;
    int n, mlen;
  
    mlen  = len;
    etime = ipg_systime() + timeout;
    
    while (mlen > 0)
       {
	if (timeout >= 0)
           {
	    if ((n = tcpNbytes(fd)) < 0) return(n); /* ioctl error - should never happen. */
    
	    if (n == 0)		/* No data is available */
	       {
		if (ipg_systime() > etime)	/* Time-out */
		   {
		    if (timeout > 0) return(-1);
		    else	     return(0);
		    }

		continue;	/* Keep waiting. */
		}

	    n = min(mlen, n);	/* Lesser of what we need and what's available. */
    	    }
	else n = mlen;		/* Just try to read all of what we need. */

	if (tcpGetData(fd, msg, n) != n) return(-1);

	mlen -= n;
	}

    return(len);
 }


/* receive a message

    Returns:
	        1 - buffer overflow, as much data as possible returned
    	    	0 - success
	       -1 - system error
 */
int tcpRecv(int fd, char *msg, int len, int timeout)
{
    unsigned short mlen;
    int n, status = 0;
    char buff[100];
    
    /* Get the packet size */
    if ((n = tcpRecv2(fd, msg, sizeof(mlen), timeout)) != sizeof(mlen)) return(-1);

    mlen = ntohs(*((unsigned short *)msg));
    *((unsigned short *)msg) = mlen;
 
    msg += n;	    /* next location to read into */

    if (len >= mlen)
       {    /* buffer is big enough for the message */
    	len = mlen - n;	/* we read that many bytes already from the socket */
	mlen = 0;   	/* nothing to drain */
    	}
    else
       {
        fprintf(stderr, "tcpRecv: message length %d is bigger than the buffer size %d.\n",
		                                                   mlen, len);
	mlen -= len;  /* we'll need to drain that many bytes */
	len  -= n;    /* we used up that many bytes already from the buffer */
        }

    if ((n = tcpRecv2(fd, msg, len, timeout)) != len) return(-1);

    while (mlen > 0)
       {
    	status = 1; 	/* drain rest of message */
    	len = min(sizeof(buff), mlen);

    	if ((n = tcpRecv2(fd, buff, len, timeout)) != len) return(-1);

	mlen -= n;  	/* adjust bytes remaining in the message */
        }

    return(status);
 }
