#if LAB >= 1

#ifndef _INC_STDIO_H_
#define _INC_STDIO_H_

#include <inc/stdarg.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif /* !NULL */

// lib/stdio.c
int	putchar(int c);
int	getchar(void);

// lib/printf.c
int	printf(const char*, ...);
int	snprintf(char*, int, const char*, ...);
int	vsnprintf(char*, int, const char*, va_list);
int	fprintf(int fd, const char*, ...);

// lib/readline.c
char *	readline(const char *prompt);

#endif	// not _INC_STDIO_H_
#endif	// LAB >= 1
