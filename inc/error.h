#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#ifndef _ERROR_H_
#define _ERROR_H_

/* Kernel error codes -- keep in sync with list in kern/printf.c. */
#define E_UNSPECIFIED	1	/* Unspecified or unknown problem */
#define E_BAD_ENV       2       /* Environment doesn't exist or otherwise
				   cannot be used in requested action */
#define E_INVAL		3	/* Invalid parameter */
#define E_NO_MEM	4	/* Request failed due to memory shortage */
#define E_NO_FREE_ENV   5       /* Attempt to create a new environment beyond
				   the maximum allowed */
#define E_IPC_BLOCKED   6       /* Attempt to ipc to env blocking ipc's */

#define MAXERROR 6

#endif /* _ERROR_H_ */
#endif /* LAB >= 2 */
