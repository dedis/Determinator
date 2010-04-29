#if LAB >= 1
// Formatted printing to strings,
// based on the formatting code in printfmt.c.

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/assert.h>

struct sprintbuf {
	char *buf;
	char *ebuf;
	int cnt;
};

static void
sprintputch(int ch, struct sprintbuf *b)
{
	b->cnt++;
	if (b->buf < b->ebuf)
		*b->buf++ = ch;
}

int
vsprintf(char *buf, const char *fmt, va_list ap)
{
	assert(buf != NULL);
	struct sprintbuf b = {buf, (char*)(intptr_t)~0, 0};

	// print the string to the buffer
	vprintfmt((void*)sprintputch, &b, fmt, ap);

	// null terminate the buffer
	*b.buf = '\0';

	return b.cnt;
}

int
sprintf(char *buf, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vsprintf(buf, fmt, ap);
	va_end(ap);

	return rc;
}

int
vsnprintf(char *buf, int n, const char *fmt, va_list ap)
{
	assert(buf != NULL && n > 0);
	struct sprintbuf b = {buf, buf+n-1, 0};

	// print the string to the buffer
	vprintfmt((void*)sprintputch, &b, fmt, ap);

	// null terminate the buffer
	*b.buf = '\0';

	return b.cnt;
}

int
snprintf(char *buf, int n, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vsnprintf(buf, n, fmt, ap);
	va_end(ap);

	return rc;
}

#endif // LAB >= 1
