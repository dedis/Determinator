#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#ifndef _ERROR_H_
#define _ERROR_H_

// Kernel error codes -- keep in sync with list in kern/printf.c.
#define E_UNSPECIFIED	1	// Unspecified or unknown problem
#define E_BAD_ENV       2       // Environment doesn't exist or otherwise
				// cannot be used in requested action
#define E_INVAL		3	// Invalid parameter
#define E_NO_MEM	4	// Request failed due to memory shortage
#define E_NO_FREE_ENV   5       // Attempt to create a new environment beyond
				// the maximum allowed
#define E_IPC_NOT_RECV  6	// Attempt to send to env that is not recving.

#define MAXERROR 6


// File system error codes
#define E_NO_SPACE	100	// No free space left on disk
#define E_MAX_OPEN	101	// Open-file limit reached


#endif // _ERROR_H_
#endif // LAB >= 2
