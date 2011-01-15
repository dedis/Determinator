/*
 * Generic C standard library definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_STDLIB_H
#define PIOS_INC_STDLIB_H

#include <cdefs.h>
#include <types.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */


// Process exit
#define EXIT_SUCCESS	0	// Success status for exit()
#define EXIT_FAILURE	1	// Failure status for exit()

void	exit(int status) gcc_noreturn;
void	abort(void) gcc_noreturn;


#if LAB >= 9
typedef struct {
	int	quot;		// quotient
	int	rem;		// remainder
} div_t;

typedef struct {
	long	quot;		// quotient
	long	rem;		// remainder
} ldiv_t;

#define RAND_MAX	0x7fffffff	// Maximum value returned from lrand48()



// Absolute value
static gcc_inline gcc_pure2 int abs(int x)
	{ return x >= 0 ? x : -x; }
static gcc_inline gcc_pure2 long labs(long x)
	{ return x >= 0 ? x : -x; }
static gcc_inline gcc_pure2 long long llabs(long long x)
	{ return x >= 0 ? x : -x; }

// Integer division returning both quotient and remainder
static gcc_inline gcc_pure2 div_t div(int numer, int denom) {
	div_t d;
	asm("cdq; idiv %1"
		: "=a" (d.quot), "=d" (d.rem)
		: "a" (numer), "r" (denom)
		: "cc");
	return d;
}
static gcc_inline gcc_pure2 ldiv_t ldiv(int numer, int denom) {
	ldiv_t d;
	asm("cdq; idiv %1"
		: "=a" (d.quot), "=d" (d.rem)
		: "a" (numer), "r" (denom)
		: "cc");
	return d;
}

// Number conversions
long strtol(const char *ptr, char **endptr, int base);
unsigned long strtoul(const char *ptr, char **endptr, int base);
long long strtoll(const char *ptr, char **endptr, int base);
unsigned long long strtoull(const char *ptr, char **endptr, int base);

float strtof(const char *ptr, char **endptr);
double strtod(const char *ptr, char **endptr);
long double strtold(const char *ptr, char **endptr);

static gcc_inline intmax_t strtoimax(const char *ptr, char **endptr, int base)
	{ return strtoll(ptr, endptr, base); }
static gcc_inline uintmax_t strtouimax(const char *ptr, char **endptr, int base)
	{ return strtoull(ptr, endptr, base); }

static gcc_inline int atoi(const char *str)
	{ return strtol(str, NULL, 10); }
static gcc_inline long atol(const char *str)
	{ return strtol(str, NULL, 10); }
static gcc_inline long long atoll(const char *str)
	{ return strtoll(str, NULL, 10); }
static gcc_inline double atof(const char *str)
	{ return strtod(str, NULL); }

// Memory allocation
void *	malloc(size_t size);
void *	calloc(size_t nelt, size_t eltsize);
void *	realloc(void *ptr, size_t newsize);
void	free(void *ptr);

// lib/string.c
int	atoi(const char * nptr);
long	atol(const char * nptr);

// lib/lrand48.c
void	srand48(long seedval);
long	lrand48(void);

// Environment variables
char *	getenv(const char *name);
int	putenv(char *string);
int	setenv(const char *name, const char *val, int overwrite);
int	unsetenv(const char *name);
#endif	// LAB >= 9

#endif /* !PIOS_INC_STDLIB_H */
