#if LAB >= 4
#include <inc/stdio.h>

// Collect up to 256 characters into a buffer
// and perform ONE system call to print all of them,
// in order to make the lines output to the console atomic
// and prevent interrupts from causing context switches
// in the middle of a console output line and such.
struct printbuf {
	FILE *fh;	// file descriptor
	int idx;	// current buffer index
	ssize_t result;	// accumulated results from write
	bool err;	// first error that occurred, 0 if none
	char buf[256];
};


static void
writebuf(struct printbuf *b)
{
	if (!b->err) {
		size_t result = fwrite(b->buf, 1, b->idx, b->fh);
		b->result += result;
		if (result != b->idx) // error, or wrote less than supplied
			b->err = 1;
	}
}

static void
putch(int ch, void *thunk)
{
	struct printbuf *b = (struct printbuf *) thunk;
	b->buf[b->idx++] = ch;
	if (b->idx == 256) {
		writebuf(b);
		b->idx = 0;
	}
}

int
vfprintf(FILE *fh, const char *fmt, va_list ap)
{
	struct printbuf b;

	b.fh = fh;
	b.idx = 0;
	b.result = 0;
	b.err = 0;
	vprintfmt(putch, &b, fmt, ap);
	if (b.idx > 0)
		writebuf(&b);

	return b.result;
}

int
fprintf(FILE *fh, const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vfprintf(fh, fmt, ap);
	va_end(ap);

	return cnt;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return cnt;
}

#endif /* LAB >= 4 */
