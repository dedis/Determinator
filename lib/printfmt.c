#if LAB >= 1
// Stripped-down primitive printf-style formatting routines,
// used in common by printf, sprintf, fprintf, etc.
// This code is also used by both the kernel and user programs.

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/ctype.h>
#include <inc/string.h>
#include <inc/stdarg.h>
#include <inc/assert.h>
#include <inc/ctype.h>
#include <inc/math.h>

typedef struct printstate {
	void (*putch)(int ch, void *putdat);	// character output function
	void *putdat;		// data for above function
	int padc;		// left pad character, ' ' or '0'
	int lwidth;		// field width for left-padding, -1=none
	int rwidth;		// field width for right-padding, -1=none
	int prec;		// numeric precision or string length, -1=none
	int signc;		// sign character: '+', '-', ' ', or -1=none
	int flags;		// flags below
	int base;		// base for numeric output
} printstate;

#define	F_L	0x01		// (at least) one 'l' specified
#define	F_LL	0x02		// (at least) two 'l's specified
#define F_ALT	0x04		// '#' alternate format flag specified
#define F_DOT	0x08		// '.' separating width from precision seen
#define F_RPAD	0x10		// '-' indiciating right padding seen

// Print padding characters, and an optional sign before a number.
static void
printpad(printstate *st, int width)
{
	if (st->signc >= 0)
		width--;		// account for sign character
	while (--st->width > 0)
		putch(st->padc, st->putdat);
	if (st->signc >= 0)
		putch(st->signc, st->putdat);
}

// Print a number (base <= 16) in reverse order,
// padding on the left to make output at least 'leftwidth' characters.
static void
printnum(printstate *st, unsigned long long num, int leftwidth)
{
	// first recursively print all preceding (more significant) digits
	if (num >= st->base)
		printnum(st, num / st->base, leftwidth - 1);
	else
		printpad(st, leftwidth);

	// then print this (the least significant) digit
	putch("0123456789abcdef"[num % st->base], st->putdat);
}

// Get an unsigned int of various possible sizes from a varargs list,
// depending on the lflag parameter.
static uintmax_t
getuint(printstate *st, va_list *ap)
{
	if (st->flags & F_LL)
		return va_arg(*ap, unsigned long long);
	else if (st->flags & F_L)
		return va_arg(*ap, unsigned long);
	else
		return va_arg(*ap, unsigned int);
}

// Same as getuint but signed - can't use getuint
// because of sign extension
static intmax_t
getint(printstate *st, va_list *ap)
{
	if (st->flags & F_LL)
		return va_arg(*ap, long long);
	else if (st->flags & F_L)
		return va_arg(*ap, long);
	else
		return va_arg(*ap, int);
}

#ifndef PIOS_KERNEL	// the kernel doesn't need or want floating-point
static void
printfloat(printstate *st, double num, int leftwidth)
{
	if (num >= 10.0)
		printfloat(st, num / 10.0, leftwidth - 1);
	else
		printpad(st, leftwidth);

	int dig = fmod(num, 10.0); assert(dig >= 0 && dig < 10);
	putch('0' + dig, st->putdat);
}

static void
printfrac(printstate *st, double num, int width)
{
	num -= floor(num);	// get the fractional part only
	while (width-- > 0) {
		num *= 10.0;
		int dig = (int)num; assert(dig >= 0 && dig < 10);
		putch('0' + dig, putdat);
		num -= dig;
	}
}
#endif	// ! PIOS_KERNEL

// Main function to format and print a string.
void
vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap)
{
	register int ch, err;

	printstate st = { .putch = putch, .putdat = putdat };
	while (1) {
		while ((ch = *(unsigned char *) fmt++) != '%') {
			if (ch == '\0')
				return;
			putch(ch, putdat);
		}

		// Process a %-escape sequence
		st.padc = ' ';
		st.signc = -1;
		st.flags = 0;
		st.lwidth = -1;
		st.rwidth = -1;
		st.prec = -1;
		uintmax_t num;
	reswitch:
		switch (ch = *(unsigned char *) fmt++) {

		// modifier flags
		case '-': // pad on the right instead of the left
			st.flags |= F_RPAD;
			goto reswitch;

		case '+': // prefix positive numeric values with a '+' sign
			st.signc = '+';
			goto reswitch;

		case ' ': // prefix signless numeric values with a space
			if (st.signc < 0)
				signc = ' ';
			goto reswitch;

		// width or precision field
		case '0':
			if (!(st.flags & F_DOT))
				padc = '0'; // pad with 0's instead of spaces
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (st.prec = 0; ; ++fmt) {
				st.prec = st.prec * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			goto gotprec;

		case '*':
			st.prec = va_arg(ap, int);
		gotprec:
			if (!(st.flags & F_DOT)) {
				if (st.flags & F_RPAD)
					st.rwidth = st.prec;
				else
					st.lwidth = st.prec;
				st.prec = -1;
			}
			goto reswitch;

		case '.':
			st.flags |= F_DOT;
			goto reswitch;

		case '#':
			st.flags |= F_ALT;
			goto reswitch;

		// long flag (doubled for long long)
		case 'l':
			st.flags |= (st.flags & F_L) ? F_LL : F_L;
			goto reswitch;

		// character
		case 'c':
			putch(va_arg(ap, int), putdat);
			break;

		// string
		case 's': {
			const char *s;
			if ((s = va_arg(ap, char *)) == NULL)
				s = "(null)";
			int plen = (st.prec >= 0) ? strnlen(s, st.prec)
						   : strlen(s);
			printpad(st, st.lwidth - len);
			while ((ch = *p++) != '\0' && (--len >= 0))
				if (altflag && !isprint(ch))
					putch('?', putdat);
				else
					putch(ch, putdat);
			printpad(st, st.rwidth - len);
			break;
		    }

		// (signed) decimal
		case 'd':
			num = getint(&st, &ap);
			if ((intmax_t) num < 0) {
				num = -(intmax_t) num;
				st.signc = '-';
			}
			st.base = 10;
			goto number;

		// unsigned decimal
		case 'u':
			num = getuint(&st, &ap);
			st.base = 10;
			goto number;

		// (unsigned) octal
		case 'o':
#if SOL >= 1
			num = getuint(&st, &ap);
			st.base = 8;
			goto number;
#else
			// Replace this with your code.
			putch('X', putdat);
			putch('X', putdat);
			putch('X', putdat);
			break;
#endif

		// pointer
		case 'p':
			putch('0', putdat);
			putch('x', putdat);
			num = (unsigned long long)
				(uintptr_t) va_arg(ap, void *);
			st.base = 16;
			goto number;

		// (unsigned) hexadecimal
		case 'x':
			num = getuint(&st, &ap);
			base = 16;
		number: {
			int lwidth = (st.flags & F_RPAD) ? -1 : width;
			int rwidth = (st.flags & F_RPAD) ? width : -1;
			printnum(&st, num, lwidth);
			break;
		    }

#ifndef PIOS_KERNEL
		// floating-point
		case 'f': case 'F':
		case 'e': case 'E':	// XXX should be different from %f
		case 'g': case 'G': {	// XXX should be different from %f
			double val = va_arg(ap, double);
			if (val < 0) {
				val = -val;
				signc = '-';
			}

			// Handle infinities and NaNs
			const char *str = NULL;
			if (isinf(val))
				str = isupper(ch) ? "INF" : "inf";
			else if (isnan(val))
				str = isupper(ch) ? "NAN" : "nan";
			if (str != NULL) {
				putch(str[0], putdat);
				putch(str[1], putdat);
				putch(str[2], putdat);
				break;
			}

			if (st.prec < 0)	// width of fraction part
				st.prec = 6;
			lwidth -= st.prec;	// width only of int part
			if (st.prec > 0 || altflag)
				lwidth--;	// account for '.'
			printfloat(putch, putdat, val, lwidth, padc, signc);
			if (precision > 0 || altflag)
				putch('.', putdat);
			printfrac(&st, val, precision);
			for (; rwidth > 0; rwidth--)
				putch(' ', putdat);
			break;
		    }
#endif	// ! PIOS_KERNEL

		// escaped '%' character
		case '%':
			putch(ch, putdat);
			break;

		// unrecognized escape sequence - just print it literally
		default:
			putch('%', putdat);
			for (fmt--; fmt[-1] != '%'; fmt--)
				/* do nothing */;
			break;
		}
	}
}

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
