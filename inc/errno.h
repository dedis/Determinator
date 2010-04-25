#if LAB >= 4
/*
 * Unix-compatibility API - error number definitions.
 */
#ifndef PIOS_INC_ERRNO_H
#define PIOS_INC_ERRNO_H

#include <file.h>


// A process/thread's errno variable is in the filestate structure,
// so that it won't get merged and will behave as thread-private data.
#define	errno		(files->err)

// Error numbers - keep consistent with strerror() in lib/string.c!
#define EINVAL		1	/* Invalid argument */
#define ENOENT		2	/* No such file or directory */
#define EFBIG		3	/* File too large */
#define EMFILE		4	/* Too many open files */
#define ENOTDIR		5	/* Not a directory */
#define ENAMETOOLONG	6	/* File name too long */
#define ENOSPC		7	/* No space left on device */
#define EAGAIN		8	/* Resource temporarily unavailable */
#define ECHILD		9	/* No child processes */
#define ECONFLICT	10	/* Conflict detected (PIOS-specific) */
#if LAB >= 9

// C standard error numbers that PIOS doesn't need, but applications do.
#define EDOM		11	/* Argument out of domain */
#define ERANGE		12	/* Result too large */

// Unix standard error numbers that PIOS doesn't need, but applications may.
#define E2BIG		13	/* Argument list too long */
#define EACCES		14	/* Permission denied */
#define EADDRINUSE	15	/* Address in use */
#define EADDRNOTAVAIL	16	/* Address not available */
#define EAFNOSUPPORT	17	/* Address family not supported */
#define EALREADY	18	/* Connection already in progress */
#define EBADF		19	/* Bad file descriptor */
#define EBADMSG		20	/* Bad message */
#define EBUSY		21	/* Device busy */
#define ECANCELED	22	/* Operation canceled */
#define ECONNABORTED	23	/* Connection aborted */
#define ECONNREFUSED	24	/* Connection refused */
#define ECONNRESET	25	/* Connection reset */
#define EDEADLK		26	/* Resource deadlock avoided */
#define EDESTADDREQ	27	/* Destination address required */
#define EEXIST		28	/* File exists */
#define EFAULT		29	/* Bad address */
#define EHOSTUNREACH	30	/* Host is unreachable */
#define EIDRM		31	/* Identifier removed */
#define EILSEQ		32	/* Illegal byte sequence */
#define EINTR		33	/* Interrupted system call */
#define EIO		34	/* Input/output error */
#define EISCONN		35	/* Socket is connected */
#define EISDIR		36	/* Is a directory */
#define ELOOP		37	/* Too many levels of symbolic links */
#define EMLINK		38	/* Too many links */
#define EMSGSIZE	39	/* Message too large */
#define ENETDOWN	40	/* Network is down */
#define ENETRESET	41	/* Connection aborted by network */
#define ENETUNREACH	42	/* Network unreachable */
#define ENFILE		43	/* Max files open in system */
#define ENOBUFS		44	/* No buffer space available */
#define ENODEV		45	/* Op not supported by device */
#define ENOEXEC		46	/* Executable format error */
#define ENOLCK		47	/* No locks available */
#define ENOMEM		48	/* Cannot allocate memory */
#define ENOMSG		49	/* No message of desired type */
#define ENOPROTOOPT	50	/* Protocol not available */
#define ENOSYS		51	/* Function not implemented */
#define ENOTCONN	52	/* Socket is not connected */
#define ENOTEMPTY	53	/* Directory not empty */
#define ENOTSOCK	54	/* Not a socket */
#define ENOTSUP		55	/* Not supported */
#define ENOTTY		56	/* Inappropriate I/O control operation */
#define ENXIO		57	/* Device not configured */
#define EOPNOTSUPP	58	/* Operation not supported on socket */
#define EOVERFLOW	59	/* Value too large to be stored in data type */
#define EPERM		60	/* Op not permitted */
#define EPIPE		61	/* Broken pipe */
#define EPROTO		62	/* Protocol error */
#define EPROTONOSUPPORT	63	/* Protocol not supported */
#define EPROTOTYPE	64	/* Protocol wrong type for socket */
#define EROFS		65	/* Read-only file system */
#define ESPIPE		66	/* Illegal seek */
#define ESRCH		67	/* No such process */
#define ETIMEDOUT	68	/* Connection timed out */
#define EXDEV		69	/* Cross-device link */

#define EWOULDBLOCK	EAGAIN	/* Operation would block */
#endif

#endif	// !PIOS_INC_ERRNO_H
#endif	// LAB >= 4
