/*
 * Standard I/O definitions, mostly inline with the standard C/Unix API
 * (except for the PIOS-specific "console printing" routines cprintf/cputs,
 * which are intended for debugging purposes only).
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_STDIO_H
#define PIOS_INC_STDIO_H

#include <types.h>
#include <stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#ifndef SEEK_SET
#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#endif

#define EOF		(-1)	/* return value indicating end-of-file */

typedef struct filedesc FILE;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;


// lib/stdio.c
int	fputc(int c, FILE *fh);
int	fgetc(FILE *fh);
#if LAB >= 9			// "Real function" versions
int	putc(int c, FILE *f);
int	putchar(int c);
int	getc(FILE *f);
int	getchar(void);
int	ungetc(int c, FILE *f);
int	puts(const char *str);
int	fputs(const char *str, FILE *f);
#endif
#define putchar(c)	fputc(c, stdout)
#define putc(c,fh)	fputc(c, fh)
#define getchar()	fgetc(stdin)
#define getc(fh)	fgetc(fh)

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat,
		const char *fmt, va_list);
int	sprintf(char *str, const char *fmt, ...);
int	vsprintf(char *str, const char *fmt, va_list args);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list args);

#if LAB >= 9
// lib/scanfmt.c
int	vscanfmt(int (*lookch)(int action, void *scandat), void *scandat,
		const char *fmt, va_list);
int	sscanf(const char *str, const char *fmt, ...);
int	vsscanf(const char *str, const char *fmt, va_list arg);

#endif
// lib/cputs.c (user space impl) or kern/console.c (kernel impl)
void	cputs(const char *str);

// lib/cprintf.c
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);

// lib/fprintf.c
int	printf(const char *fmt, ...);
int	vprintf(const char *fmt, va_list args);
int	fprintf(FILE *f, const char *fmt, ...);
int	vfprintf(FILE *f, const char *fmt, va_list args);
#if LAB >= 9
int	scanf(const char *fmt, ...);
int	vscanf(const char *fmt, va_list args);
int	fscanf(FILE *f, const char *fmt, ...);
int	vfscanf(FILE *f, const char *fmt, va_list args);

void	perror(const char *s);
#endif

// lib/stdio.c
FILE *	fopen(const char *filename, const char *mode);
FILE *	freopen(const char *filename, const char *mode, FILE *fh);
int	fclose(FILE *fh);
size_t	fread(void *ptr, size_t size, size_t count, FILE *fh);
size_t	fwrite(const void *ptr, size_t size, size_t count, FILE *fh);
int	fseek(FILE *fh, off_t offset, int whence);
long	ftell(FILE *fh);
int	feof(FILE *fd);
int	ferror(FILE *fd);
void	clearerr(FILE *fd);
#if LAB >= 99
int	fileno(FILE *fd);
#endif
int	fflush(FILE *fd);

// lib/readline.c
char*	readline(const char *prompt);

#if LAB >= 9
// lib/dir.c
int	rename(const char *oldname, const char *newname);
#endif // LAB >= 9

#endif /* !PIOS_INC_STDIO_H */
