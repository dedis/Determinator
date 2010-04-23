#ifndef PIOS_INC_STDLIB_H
#define PIOS_INC_STDLIB_H

#include <gcc.h>
#include <types.h>


#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#define EXIT_SUCCESS	0	// Success status for exit()
#define EXIT_FAILURE	1	// Failure status for exit()


static gcc_inline gcc_pure2 int abs(int x)
	{ return x >= 0 ? x : -x; }
static gcc_inline gcc_pure2 long labs(long x)
	{ return x >= 0 ? x : -x; }
static gcc_inline gcc_pure2 long long llabs(long long x)
	{ return x >= 0 ? x : -x; }

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

void *	malloc(size_t size);
void *	calloc(size_t nelt, size_t eltsize);
void *	realloc(void *ptr, size_t newsize);
void	free(void *ptr);

void	exit(int status) gcc_noreturn;
void	abort(void) gcc_noreturn;

#endif /* !PIOS_INC_STDLIB_H */
