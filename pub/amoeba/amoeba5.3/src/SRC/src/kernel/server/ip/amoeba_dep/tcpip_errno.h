/*	@(#)tcpip_errno.h	1.3	96/02/27 14:15:09 */
/*
tcpip_errno.h
*/

#ifndef TCPIP_ERRNO_H
#define TCPIP_ERRNO_H

#define ERROR		TCPIP_ERROR		/* generic error */
#define EGENERIC	TCPIP_ERROR		/* neneric error */
#define ENOENT		TCPIP_NOENT		/* no such file or directory */
#define EINVAL		STD_ARGBAD		/* invalid argument */
#define EINTR		STD_INTR		/* interrupted function call */
#define EAGAIN		STD_NOTNOW		/* resource temporarily
						 * unavailable */
#define ENOMEM        	STD_NOSPACE		/* not enough space */
#define EFAULT		TCPIP_FAULT		/* bad address */
#define ENXIO		TCPIP_ERROR		/* no such device or address */
#define ESRCH		TCPIP_NOENT		/* no such process */

#define EPACKSIZE	TCPIP_PACKSIZE		/* invalid packet size for some
						 * protocol */
#define EOUTOFBUFS	TCPIP_OUTOFBUFS		/* not enough buffers left */
#define EBADIOCTL	TCPIP_BADIOCTL		/* illegal ioctl for device */
#define EBADMODE	TCPIP_BADMODE		/* badmode in ioctl */
#define EBADDEST	TCPIP_BADDEST		/* not a valid destination
						 * address */
#define EDSTNOTRCH	TCPIP_DSTNOTRCH		/* destination not reachable */
#define EISCONN		TCPIP_ISCONN		/* all ready connected */
#define EADDRINUSE	TCPIP_ADDRINUSE		/* address in use */
#define ECONNREFUSED	TCPIP_CONNREFUSED	/* connection refused */
#define ECONNRESET	TCPIP_CONNRESET		/* connection reset */
#define ETIMEDOUT	TCPIP_TIMEDOUT		/* connection timed out */
#define EURG		TCPIP_URG		/* urgent data present */
#define ENOURG		TCPIP_NOURG		/* no urgent data present */
#define ENOTCONN	TCPIP_NOTCONN		/* no connection (yet or
						 * anymore) */
#define ESHUTDOWN	TCPIP_SHUTDOWN		/* a write call to a shutdown
						 * connection */
#define ENOCONN		TCPIP_NOCONN		/* no such connection */

#if 0
/* Here are the numerical values of the error numbers. */
#define _NERROR               70  /* number of errors */  

#define EPERM         (-1)  /* operation not permitted */
#define ESRCH         (-3)  /* no such process */
#define EIO           (-5)  /* input/output error */
#define ENXIO         (-6)  /* no such device or address */
#define E2BIG         (-7)  /* arg list too long */
#define ENOEXEC       (-8)  /* exec format error */
#define EBADF         (-9)  /* bad file descriptor */
#define ECHILD        (-10)  /* no child process */
#define EACCES        (-13)  /* permission denied */
#define ENOTBLK       (-15)  /* Extension: not a block special file */
#define EBUSY         (-16)  /* resource busy */
#define EEXIST        (-17)  /* file exists */
#define EXDEV         (-18)  /* improper link */
#define ENODEV        (-19)  /* no such device */
#define ENOTDIR       (-20)  /* not a directory */
#define EISDIR        (-21)  /* is a directory */
#define ENFILE        (-23)  /* too many open files in system */
#define EMFILE        (-24)  /* too many open files */
#define ENOTTY        (-25)  /* inappropriate I/O control operation */
#define ETXTBSY       (-26)  /* no longer used */
#define EFBIG         (-27)  /* file too large */
#define ENOSPC        (-28)  /* no space left on device */
#define ESPIPE        (-29)  /* invalid seek */
#define EROFS         (-30)  /* read-only file system */
#define EMLINK        (-31)  /* too many links */
#define EPIPE         (-32)  /* broken pipe */
#define EDOM          (-33)  /* domain error    	(from ANSI C std) */
#define ERANGE        (-34)  /* result too large	(from ANSI C std) */
#define EDEADLK       (-35)  /* resource deadlock avoided */
#define ENAMETOOLONG  (-36)  /* file name too long */
#define ENOLCK        (-37)  /* no locks available */
#define ENOSYS        (-38)  /* function not implemented */
#define ENOTEMPTY     (-39)  /* directory not empty */

/* The following errors relate to networking. */

#define EWOULDBLOCK	(-54)

/* The following are not POSIX errors, but they can still happen. */
#define ELOCKED      (-101)  /* can't send message */
#define EBADCALL     (-102)  /* error on send/receive */

/* The following error codes are generated by the kernel itself. */
#define E_BAD_DEST     -1001	/* destination address illegal */
#define E_BAD_SRC      -1002	/* source address illegal */
#define E_TRY_AGAIN    -1003	/* can't send-- tables full */
#define E_OVERRUN      -1004	/* interrupt for task that is not waiting */
#define E_BAD_BUF      -1005	/* message buf outside caller's addr space */
#define E_TASK         -1006	/* can't send to task */
#define E_NO_MESSAGE   -1007	/* RECEIVE failed: no message present */
#define E_NO_PERM      -1008	/* ordinary users can't send to tasks */
#define E_BAD_FCN      -1009	/* only valid fcns are SEND, RECEIVE, BOTH */
#define E_BAD_ADDR     -1010	/* bad address given to utility routine */
#define E_BAD_PROC     -1011	/* bad proc number given to utility */
#endif

#endif /* TCPIP_ERRNO_H */
