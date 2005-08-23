#if LAB >= 1
/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ERROR_H
#define JOS_INC_ERROR_H

// Kernel error codes -- keep in sync with list in lib/printfmt.c.
#define E_UNSPECIFIED	1	// Unspecified or unknown problem
#define E_BAD_ENV	2	// Environment doesn't exist or otherwise
				// cannot be used in requested action
#define E_INVAL		3	// Invalid parameter
#define E_NO_MEM	4	// Request failed due to memory shortage
#define E_NO_FREE_ENV	5	// Attempt to create a new environment beyond
				// the maximum allowed
#define E_IPC_NOT_RECV	6	// Attempt to send to env that is not recving
#define E_EOF		7	// Unexpected end of file

#if LAB >= 5
// File system error codes -- only seen in user-level
#define	E_NO_DISK	8	// No free space left on disk
#define E_MAX_OPEN	9	// Too many files are open
#define E_NOT_FOUND	10 	// File or block not found
#define E_BAD_PATH	11	// Bad path
#define E_FILE_EXISTS	12	// File already exists
#define E_NOT_EXEC	13	// File not a valid executable

#define MAXERROR	13
#else	// !LAB >= 5
#define	MAXERROR	7
#endif	// LAB >= 5

#endif	// !JOS_INC_ERROR_H */
#endif	// LAB >= 1
