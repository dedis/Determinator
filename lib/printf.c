#if LAB >= 1
// Simple implementation of printf console output,
// based on printfmt() and putchar().
// This code is used by both the kernel and user programs.

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>


static void
putch(int ch, int *cnt)
{
	putchar(ch);
	*cnt++;
}

int
vprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

#endif // LAB >= 1
