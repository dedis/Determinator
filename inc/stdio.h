#ifndef _INC_STDIO_H_
#define _INC_STDIO_H_

#include <inc/stdarg.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif /* !NULL */

// lib/stdio.c
void	putchar(int c);
int	getchar(void);

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat,
			const char *fmt, va_list ap);

// lib/printf.c
int	printf(const char*, ...);
int	vprintf(const char*, va_list);

// lib/sprintf.c
int	snprintf(char*, int, const char*, ...);
int	vsnprintf(char*, int, const char*, va_list);

// lib/fprintf.c
int	fprintf(int fd, const char*, ...);
int	vfprintf(int fd, const char*, va_list);

// lib/readline.c
char *	readline(const char *prompt);

#endif	// not _INC_STDIO_H_
