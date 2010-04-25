// Simple printf-style formatting routines,
// used in common by scanf, sscanf, fscanf, etc.

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define PEEK	1
#define NEXT	2

int
vscanfmt(int (*lookch)(int action, void *dat), void *dat,
	const char *fmt, va_list ap)
{
	const unsigned char *f = (const unsigned char*)fmt;
	int nelt = 0;
	int fc;
	while ((fc = *f++) != 0) {
		int sc = lookch(PEEK, dat);
		if (sc < 0) {
			err:
			return nelt > 0 ? nelt : EOF;
		}

		// Whitespace in the format string means consume whitespace
		if (isspace(fc)) {
			while (isspace(*f))	// eat all in fmt string
				f++;
			while (isspace(sc))	// eat all in input string
				sc = lookch(NEXT|PEEK, dat);
			continue;
		}

		// Match literal charcters in the format string
		if (fc != '%') {
			if (sc != fc)
				goto err;	// characters don't match
			lookch(NEXT, dat);
			continue;
		}

		int precision = -1;
		int lflag = 0;
		int skipflag = 0;
		int base;
		reswitch:
		switch (fc = *f++) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			precision = 0;
			do {
				precision = precision * 10 + fc - '0';
				fc = *f++;
			} while (fc >= '0' && fc <= '9');
			goto reswitch;
		case 'l':
			lflag++;
			goto reswitch;
		case '*':
			skipflag = 1;
			goto reswitch;
		case 'd':
		case 'u':
			base = 10;
			number:
			while (isspace(sc))	// consume input whitespace
				sc = lookch(NEXT|PEEK, dat);
			int neg = (sc == '-');	// consume leading +/- signs
			if (neg || sc == '+')
				sc = lookch(NEXT|PEEK, dat);
			uint64_t val = 0;
			bool anydig = 0;
			while (1) {		// scan digits
				if (sc >= '0' && sc <= '9')
					val = val * base + sc - '0';
				else if (sc >= 'A' && sc <= 'A' + base - 10)
					val = val * base + 10 + sc - 'A';
				else if (sc >= 'a' && sc <= 'a' + base - 10)
					val = val * base + 10 + sc - 'a';
				else
					break;
				sc = lookch(NEXT|PEEK, dat);
				anydig = 1;
			}
			if (!anydig)
				goto err;	// didn't find a valid number
			if (neg)
				val = -val;
			if (skipflag)
				continue;
			if (lflag == 0)		// store as int
				*va_arg(ap, int *) = val;
			else if (lflag == 1)	// store as long
				*va_arg(ap, long *) = val;
			else if (lflag >= 2)	// store as long long
				*va_arg(ap, long long *) = val;
			nelt++;
			continue;
		case 'x':
			base = 16;
			goto number;
		case 'o':
			base = 8;
			goto number;
		default:
			panic("vscanfmt: unknown conversion character '%c'\n",
				fc);
		}
	}
	return nelt;
}

static int sscanlook(int action, void *dat)
{
	const char **strp = dat;
	if (action & NEXT) {	// advance to next character
		assert(**strp != 0);
		(*strp)++;
	}
	return **strp;		// peek next char - always safe for strings
}

int
vsscanf(const char *str, const char *fmt, va_list ap)
{
	cprintf("scanning str '%s' via fmt '%s'\n", str, fmt);
	return vscanfmt(sscanlook, &str, fmt, ap);
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

