#ifndef PIOS_INC_STDLIB_H
#define PIOS_INC_STDLIB_H

#include <gcc.h>
#include <types.h>
#include <malloc.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#define EXIT_SUCCESS	0	// Success status for exit()
#define EXIT_FAILURE	1	// Failure status for exit()

#define RAND_MAX	0x7fffffff	// Maximum value returned from lrand48()


// lib/stdlib.c
void	exit(int status) gcc_noreturn;
void	abort(void) gcc_noreturn;

// lib/string.c
int	atoi(const char * nptr);
long	atol(const char * nptr);

// lib/lrand48.c
void	srand48(long seedval);
long	lrand48(void);

#endif /* !PIOS_INC_STDLIB_H */
