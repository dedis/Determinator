#if LAB >= 1
// Simple implementation of sprintf() based on printfmt() primitives.
// This code is used by both the kernel and user programs.

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/error.h>


struct sprintbuf {
	char *buf;
	char *ebuf;
	int cnt;
};

static void
putch(int ch, struct sprintbuf *b)
{
	b->cnt++;
	if (b->buf < b->ebuf)
		*b->buf++ = ch;
}

int
vsnprintf(char *buf, int n, const char *fmt, va_list ap)
{
	struct sprintbuf b = {buf, buf+n-1, 0};

	if (buf == NULL || n < 1)
		return -E_INVAL;

	// print the string to the buffer
	vprintfmt((void*)putch, &b, fmt, ap);

	// null terminate the buffer
	*b.buf = '\0';

	return b.cnt;
}

int
snprintf(char *buf, int n, const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vsnprintf(buf, n, fmt, ap);
	va_end(ap);

	return cnt;
}

#endif // LAB >= 1
