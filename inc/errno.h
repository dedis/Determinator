#if LAB >= 4
/*
 * Unix-compatibility API - error number definitions.
 */
#ifndef PIOS_INC_ERRNO_H
#define PIOS_INC_ERRNO_H

#include <inc/file.h>


// A process/thread's errno variable is in the unixstate structure,
// so that it won't get merged and will behave as thread-private data.
#define	errno		(files->err)


// Error numbers - keep consistent with strerror() in lib/string.c!
#define EINVAL		1	/* Invalid argument */
#define ENOENT		2	/* No such file or directory */
#define EBADF		3	/* Bad file descriptor */
#define EEXIST		4	/* File exists */
#define EFBIG		5	/* File too large */
#define EMFILE		6	/* Too many open files */
#define EISDIR		7	/* Is a directory */
#define ENOTDIR		8	/* Not a directory */
#define ENOTEMPTY	9	/* Directory not empty */
#define ENAMETOOLONG	10	/* File name too long */
#define ENOSPC		11	/* No space left on device */
#define ENOTTY		12	/* Inappropriate I/O control operation */
#define EAGAIN		13	/* Resource temporarily unavailable */
#define ECHILD		14	/* No child processes */

#if LAB >= 99
#define EDOM		X	/* Argument out of domain */
#define ERANGE		X	/* Result too large */

#define E2BIG		X	/* Argument list too long */
#define EACCES		X	/* Permission denied */
#define EBUSY		X	/* Device busy */
#define EDEADLK		X	/* Resource deadlock avoided */
#define EFAULT		X	/* Bad address */
#define EINTR		X	/* Interrupted system call */
#define EIO		X	/* Input/output error */
#define EMLINK		X	/* Too many links */
#define ENFILE		X	/* Max files open in system */
#define ENODEV		X	/* Op not supported by device */
#define ENOEXEC		X	/* Exec format error */
#define ENOLCK		X	/* No locks available */
#define ENOMEM		X	/* Cannot allocate memory */
#define ENOSYS		X	/* Function not implemented */
#define ENXIO		X	/* Device not configured */
#define EPERM		X	/* Op not permitted */
#define EPIPE		X	/* Broken pipe */
#define EROFS		X	/* Read-only file system */
#define ESPIPE		X	/* Illegal seek */
#define ESRCH		X	/* No such process */
#define EXDEV		X	/* Cross-device link */
#endif

#endif	// !PIOS_INC_ERRNO_H
#endif	// LAB >= 4
