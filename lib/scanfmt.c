#if LAB >= 9
// Simple printf-style formatting routines,
// used in common by scanf, sscanf, fscanf, etc.

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>

#define PEEK	1
#define NEXT	2

#define SCANDIGITS(base,sc,val,each) \
	while (1) { \
		if (sc >= '0' && sc <= '9') \
			val = val * base + sc - '0'; \
		else if (sc >= 'A' && sc <= 'A' + base - 10) \
			val = val * base + 10 + sc - 'A'; \
		else if (sc >= 'a' && sc <= 'a' + base - 10) \
			val = val * base + 10 + sc - 'a'; \
		else \
			break; \
		sc = lookch(NEXT|PEEK, dat); \
		each; \
	}

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

		int width = -1;
		int lflag = 0;
		int skipflag = 0;
		int base;
		reswitch:
		switch (fc = *f++) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (width < 0)
				width = 0;
			width = width * 10 + fc - '0';
			goto reswitch;
		case 'l':
			lflag++;
			goto reswitch;
		case '*':
			skipflag = 1;
			goto reswitch;
		case 'c': {			// character(s)
			if (width < 0)
				width = 1;
			char *buf = va_arg(ap, char *);
			while (width-- > 0) {
				if (sc < 0)
					goto err;	// incomplete field
				*buf++ = sc;
				sc = lookch(NEXT | (width > 0 ? PEEK : 0), dat);
			}
			nelt++;
			continue;
		    }
		case 's': {			// string
			if (width < 0)
				width = INT_MAX;
			while (isspace(sc))	// consume leading whitespace
				sc = lookch(NEXT|PEEK, dat);
			char *buf = va_arg(ap, char *);
			while (width-- > 0) {
				if (sc < 0)
					goto err;	// incomplete field
				if (isspace(sc))
					break;		// end of string
				*buf++ = sc;
				sc = lookch(NEXT | (width > 0 ? PEEK : 0), dat);
			}
			*buf = 0;		// null terminate string
			nelt++;
			continue;
		    }
		case 'i': {			// general integer
			base = 0;
		number:
			while (isspace(sc))	// consume leading whitespace
				sc = lookch(NEXT|PEEK, dat);
			int neg = (sc == '-');	// consume leading +/- signs
			if (neg || sc == '+')
				sc = lookch(NEXT|PEEK, dat);
			if (base == 0) {	// choose a base
				if (sc == '0') {
					sc = lookch(NEXT|PEEK, dat);
					if (sc == 'x' || sc == 'X') {
						sc = lookch(NEXT|PEEK, dat);
						base = 16;
					} else
						base = 8;
				} else
					base = 10;
			}
			uint64_t val = 0; int ndig = 0;
			SCANDIGITS(base,sc,val,ndig++)
			if (ndig == 0)
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
		    }
		case 'd':			// decimal integer
		case 'u':
			base = 10;
			goto number;
		case 'x': case 'X':		// hexadecimal integer
			base = 16;
			goto number;
		case 'o':			// octal integer
			base = 8;
			goto number;
		case 'a': case 'A':		// floating-point number
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G': {
			while (isspace(sc))	// consume input whitespace
				sc = lookch(NEXT|PEEK, dat);
			int neg = (sc == '-');	// consume leading +/- signs
			if (neg || sc == '+')
				sc = lookch(NEXT|PEEK, dat);
			// XXX NAN, INF/INFINITY
			uint64_t intpart = 0; int intdig = 0;
			SCANDIGITS(10,sc,intpart,intdig++)
			if (intdig == 0 && sc != '.')
				goto err;	// didn't find a valid number
			double val = intpart;
			if (sc == '.') {
				sc = lookch(NEXT|PEEK, dat);	// skip '.'
				uint64_t numer = 0, denom = 1;
				SCANDIGITS(10,sc,numer,denom *= 10);
				val += (double)numer / (double)denom;
			}
			if (neg)
				val = -val;
			if (skipflag)
				continue;
			if (lflag == 0)		// store as float
				*va_arg(ap, float *) = val;
			else 			// store as double
				*va_arg(ap, double *) = val;
			nelt++;
			continue;
			break;
		    }
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
	//cprintf("scanning str '%s' via fmt '%s'\n", str, fmt);
	return vscanfmt(sscanlook, &str, fmt, ap);
}

int
sscanf(const char *str, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int rc = vsscanf(str, fmt, ap);
	va_end(ap);
	return rc;
}

typedef struct fscanstate {
	FILE *f;
	int c;
} fscanstate;

static int fscanlook(int action, void *dat)
{
	fscanstate *st = dat;
	if (action & NEXT) {
		assert(st->c != EOF);	// should have a char by now
		st->c = EOF;		// toss it and move to next
	}
	if ((action & PEEK) && (st->c == EOF))
		st->c = fgetc(st->f);	// get a new char
	return st->c;
}

int
vfscanf(FILE *f, const char *fmt, va_list ap)
{
	fscanstate st = { .f = f, .c = EOF };
	int rc = vscanfmt(fscanlook, &st, fmt, ap);
	if (st.c != EOF)
		ungetc(st.c, f);	// unget the last char we peeked
	return rc;
}

int
fscanf(FILE *f, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int rc = vfscanf(f, fmt, ap);
	va_end(ap);
	return rc;
}

int
vscanf(const char *fmt, va_list arg)
{
	return vfscanf(stdin, fmt, arg);
}

int
scanf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int rc = vscanf(fmt, ap);
	va_end(ap);
	return rc;
}

#endif	// LAB >= 9
