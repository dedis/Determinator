#if LAB >= 4
// Stripped-down printf.  Unlike kernel printf, formats to buffer
// and then sends buffer in one go with sys_cputs.

#include "lib.h"

/*
 * Put a number(base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
static char *
printnum(u_int uq, int base, char *buf, int *lenp)
{				/* A quad in binary, plus NULL. */
	register char *p;

	p = buf;
	*buf = 0;
	do {
		*++p = "0123456789abcdef"[uq % base];
	} while (uq /= base);
	if (lenp)
		*lenp = p - buf;
	return(p);
}

/*
 * Scaled down version of printf(3).
 *
 * Three additional formats:
 *
 * The format %b is supported to decode error registers.
 * Its usage is:
 *
 *      printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected(origin 1), and
 * the next characters(up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *      kprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *      reg=3<BITTWO,BITONE>
 *
 * The format %r passes an additional format string and argument list
 * recursively.  Its usage is:
 *
 * fn(char *fmt, ...)
 * {
 *      va_list ap;
 *      va_start(ap, fmt);
 *      printf("prefix: %r: suffix\n", fmt, ap);
 *      va_end(ap);
 * }
 *
 * Space or zero padding and a field width are supported for the numeric
 * formats only. 
 * 
 * The format %e takes an integer error code and prints a string describing the error.
 * The integer may be positive or negative, so that -E_NO_MEM and E_NO_MEM are
 * equivalent.
 */

static char *error_string[MAXERROR+1] =
{
	NULL,
	"unspecified error",
	"bad environment",
	"invalid parameter",
	"out of memory",
	"out of environments",
	"env is not recving",
#if LAB >= 5
	"no free space on disk",
	"too many files are open",
	"file or block not found",
	"invalid path",
	"file already exists",
	"file is not a valid executable",
#endif
};

static u_int
getint(va_list *ap, int lflag, int qflag)
{
	if (lflag)
		return va_arg(*ap, u_long);
	else /*if (qflag)
		return va_arg(*ap, u_quad_t);
	else*/
		return va_arg(*ap, u_int);
}

static void
buf_putc(char **pbuf, char *ebuf, int ch)
{
	char *buf;

	buf = *pbuf;
	if(buf >= ebuf){
		*pbuf = ebuf-1;
		ebuf[-1] = 0;
		return;
	}
	*buf++ = ch;
	*pbuf = buf;
}

int
vsnprintf(char *buf, int m, const char *fmt, va_list ap)
{
	char *obuf, *ebuf;
	register char *p, *q;
	register int ch, n;
	u_int uq;
	int base, lflag, qflag, tmp, width;
	char padc;
	char numbuf[sizeof(u_quad_t) * 8 + 1];

	obuf = buf;
	ebuf = buf+m;
	for (;;) {
		padc = ' ';
		width = 0;
		while ((ch = *(u_char *) fmt++) != '%') {
			buf_putc(&buf, ebuf, ch);
			if (ch == '\0')
				return buf-1-obuf;
		}
		lflag = 0;
		qflag = 0;
	reswitch:
		switch (ch = *(u_char *) fmt++) {
		case '0':
			padc = '0';
			goto reswitch;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			for (width = 0;; ++fmt) {
				width = width * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			goto reswitch;
		case 'l':
			lflag = 1;
			qflag = 0; /* this should be done better */
			goto reswitch;
		case 'q':
			qflag = 1;
			lflag = 0; /* this should be done better */
			goto reswitch;
		case 'b':
			uq = va_arg(ap, int);
			p = va_arg(ap, char *);
			for (q = printnum(uq, *p++, numbuf, NULL); (ch = *q--) != '\0';)
				buf_putc(&buf, ebuf, ch);

			if (!uq)
				break;

			for (tmp = 0; (n = *p++) != '\0'; ) {
				if (uq & (1 << (n - 1))) {
					buf_putc(&buf, ebuf, tmp ? ',' : '<');
					for (; (n = *p) > ' '; ++p)
						buf_putc(&buf, ebuf, n);
					tmp = 1;
				}
				else
					for (; *p > ' '; ++p)
						continue;
			}
			if (tmp)
				buf_putc(&buf, ebuf, '>');
			break;
		case 'c':
			buf_putc(&buf, ebuf, va_arg(ap, int));
			break;
		case 'e':
			n = va_arg(ap, int);
			if(n < 0)
				n = -n;
			if(n > MAXERROR || (p = error_string[n]) == NULL)
				buf += snprintf(buf, ebuf-buf, "error %d", n);
			else
				buf += snprintf(buf, ebuf-buf, "%s", p);
			break;
		case 'r':
			p = va_arg(ap, char *);
			buf += vsnprintf(buf, ebuf-buf, p, va_arg(ap, va_list));
			break;
		case 's':
			if ((p = va_arg(ap, char *)) == NULL)
					p = "(null)";
			while ((ch = *p++) != '\0')
				buf_putc(&buf, ebuf, ch);
			break;
		case 'd':
			uq = getint(&ap, lflag, qflag);
			/*if (qflag && (quad_t) uq < 0) {
				buf_putc(&buf, ebuf, '-');
				uq = -(quad_t) uq;
			}
			else*/ if ((long)uq < 0) {
				buf_putc(&buf, ebuf, '-');
				uq = -(long) uq;
			}
			base = 10;
			goto number;
		case 'u':
			uq = getint(&ap, lflag, qflag);
			base = 10;
			goto number;
		case 'o':
			uq = getint(&ap, lflag, qflag);
			base = 8;
			goto number;
		case 'p':
			buf_putc(&buf, ebuf, '0');
			buf_putc(&buf, ebuf, 'x');
			uq = (u_long) va_arg(ap, void *);
			base = 16;
			goto number;
		case 'x':
			uq = getint(&ap, lflag, qflag);
			base = 16;
		number:
			p = printnum(uq, base, numbuf, &tmp);
			if (width && (width -= tmp) > 0)
				while (width--)
					buf_putc(&buf, ebuf, padc);
			while ((ch = *p--) != '\0')
				buf_putc(&buf, ebuf, ch);
			break;
		default:
			buf_putc(&buf, ebuf, '%');
			if (lflag)
				buf_putc(&buf, ebuf, 'l');
			/* FALLTHROUGH */
		case '%':
			buf_putc(&buf, ebuf, ch);
		}
	}
}

int
snprintf(char *buf, int n, const char *fmt, ...)
{
	int m;
	va_list ap;

	va_start(ap, fmt);
	m = vsnprintf(buf, n, fmt, ap);
	va_end(ap);
	return m;
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters an infinite loop.
 * If executing on Bochs, drop into the debugger rather than chew CPU.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;
	char buf[256];
	int n;

	va_start(ap, fmt);
	n = snprintf(buf, sizeof buf, "user panic at %s:%d: ", file, line);
	n += vsnprintf(buf+n, sizeof buf-n, fmt, ap);
	n += snprintf(buf+n, sizeof buf-n, "\n");
	va_end(ap);
	sys_cputs(buf);

	for(;;);
}

/* like panic, but don't */
void
warn(const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	int n;

	va_start(ap, fmt);
	n = snprintf(buf, sizeof buf, "warning: ");
	n += vsnprintf(buf+n, sizeof buf-n, fmt, ap);
	n += snprintf(buf+n, sizeof buf-n, "\n");
	va_end(ap);
	sys_cputs(buf);
}

void
printf(const char *fmt, ...)
{
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	sys_cputs(buf);
}

#endif
