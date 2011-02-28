#if LAB >= 4
#include <inc/stdio.h>
#if LAB >= 9
#include <inc/errno.h>
#include <inc/assert.h>
#endif

char *
strerror(int err)
{
	static char *errtab[] = {
		"(no error)",
		"Invalid argument",			// EINVAL
		"No such file or directory",
		"File too large",
		"Too many open files",
		"Not a directory",
		"File name too long",
		"No space left on device",
		"Resource temporarily unavailable",
		"No child processes",
		"Conflict detected",
#if LAB >= 9
		"Argument out of domain",		// EDOM
		"Result too large",

		"Argument list too long",		// E2BIG
		"Permission denied",
		"Address in use",
		"Address not available",
		"Address family not supported",
		"Connection already in progress",
		"Bad file descriptor",
		"Bad message",
		"Device busy",
		"Operation canceled",
		"Connection aborted",
		"Connection refused",
		"Connection reset",
		"Resource deadlock avoided",
		"Destination address required",
		"File exists",
		"Bad address",
		"Host is unreachable",
		"Identifier removed",
		"Illegal byte sequence",
		"Interrupted system call",
		"Input/output error",
		"Socket is connected",
		"Is a directory",
		"Too many levels of symbolic links",
		"Too many links",
		"Message too large",
		"Network is down",
		"Connection aborted by network",
		"Network unreachable",
		"Max files open in system",
		"No buffer space available",
		"Op not supported by device",
		"Executable format error",
		"No locks available",
		"Cannot allocate memory",
		"No message of desired type",
		"Protocol not available",
		"Function not implemented",
		"Socket is not connected",
		"Directory not empty",
		"Not a socket",
		"Not supported",
		"Inappropriate I/O control operation",
		"Device not configured",
		"Operation not supported on socket",
		"Value too large to be stored in data type",
		"Op not permitted",
		"Broken pipe",
		"Protocol error",
		"Protocol not supported",
		"Protocol wrong type for socket",
		"Read-only file system",
		"Illegal seek",
		"No such process",
		"Connection timed out",
		"Cross-device link",		// EXDEV
#endif // LAB >= 9
	};
	static char errbuf[64];

	const int tablen = sizeof(errtab)/sizeof(errtab[0]);
#if LAB >= 9
	assert(tablen-1 == EXDEV);	// consistency check
#endif
	if (err >= 0 && err < sizeof(errtab)/sizeof(errtab[0]))
		return errtab[err];

	sprintf(errbuf, "Unknown error code %d", err);
	return errbuf;
}

#endif	// LAB >= 4
