// Simple printf-style formatting routines,
// used in common by scanf, sscanf, fscanf, etc.

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

int
vscanfmt(int (*lookch)(void *), void (*nextch)(void*), void *scandat,
	const char *fmt, va_list ap)
{
	panic("vscanfmt not implemented");
#if 0
	int nelt = 0;
	while (*fmt != 0) {
		int ch = lookch(scandat);
		if (ch < 0)
			return nelt > 0 ? nelt : EOF;
		...
	}
	return nelt;
#endif
}

int
vsscanf(const char *str, const char *fmt, va_list arg)
{
	panic("vsscanf not implemented");
}

int
sscanf(const char *str, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vsscanf(str, fmt, ap);
	va_end(ap);

	return rc;
}

int
vfscanf(FILE *f, const char *fmt, va_list arg)
{
	panic("vfscanf not implemented");
}

int
fscanf(FILE *f, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vfscanf(f, fmt, ap);
	va_end(ap);

	return rc;
}

int
vscanf(const char *fmt, va_list arg)
{
	panic("vscanf not implemented");
}

int
scanf(const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vscanf(fmt, ap);
	va_end(ap);

	return rc;
}

