/* See COPYRIGHT for copyright information. */

///LAB2

#include <inc/stdarg.h>
#include <inc/types.h>
#include <kern/picirq.h>
#include <kern/console.h>
#include <kern/printf.h>

/*
 * Put a number(base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated(preceded?)
 * buffer.
 */
static char *
ksprintn(u_quad_t uq, int base, int *lenp)
{				/* A quad in binary, plus NULL. */
	static char buf[sizeof(u_quad_t) * 8 + 1];
	register char *p;

	p = buf;
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
 * Two additional formats:
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
 */

static u_quad_t
getint(va_list *ap, int lflag, int qflag)
{
	if (lflag)
		return va_arg(*ap, u_long);
	else if (qflag)
		return va_arg(*ap, u_quad_t);
	else
		return va_arg(*ap, u_int);
}

void
kprintf(const char *fmt, va_list ap)
{
	register char *p, *q;
	register int ch, n;
	u_quad_t uq;
	int base, lflag, qflag, tmp, width;
	char padc;

	for (;;) {
		padc = ' ';
		width = 0;
		while ((ch = *(u_char *) fmt++) != '%') {
			if (ch == '\0')
				return;
			cons_putc(ch);
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
			for (q = ksprintn(uq, *p++, NULL); (ch = *q--) != '\0';)
				cons_putc(ch);

			if (!uq)
				break;

			for (tmp = 0; (n = *p++) != '\0'; ) {
				if (uq & (1 << (n - 1))) {
					cons_putc(tmp ? ',' : '<');
					for (; (n = *p) > ' '; ++p)
						cons_putc(n);
					tmp = 1;
				}
				else
					for (; *p > ' '; ++p)
						continue;
			}
			if (tmp)
				cons_putc('>');
			break;
		case 'c':
			cons_putc(va_arg(ap, int));
			break;
		case 'r':
			p = va_arg(ap, char *);
			kprintf(p, va_arg(ap, va_list));
			break;
		case 's':
			if ((p = va_arg(ap, char *)) == NULL)
					p = "(null)";
			while ((ch = *p++) != '\0')
				cons_putc(ch);
			break;
		case 'd':
			uq = getint(&ap, lflag, qflag);
			if ((quad_t) uq < 0) {
				cons_putc('-');
				uq = -(quad_t) uq;
			}
			base = 10;
			goto number;
		case 'u':
			uq = getint(&ap, lflag, qflag);
			base = 10;
			goto number;
		case 'o':
///SOL2
			uq = getint(&ap, lflag, qflag);
			base = 8;
			goto number;
///ELSE
			/* Replace this with your code to print in octal. */
			cons_putc('X');
			cons_putc('X');
			cons_putc('X');
			break;
///END
		case 'p':
			cons_putc('0');
			cons_putc('x');
			uq = (u_long) va_arg(ap, void *);
			base = 16;
			goto number;
		case 'x':
			uq = getint(&ap, lflag, qflag);
			base = 16;
		number:
			p = ksprintn(uq, base, &tmp);
			if (width && (width -= tmp) > 0)
				while (width--)
					cons_putc(padc);
			while ((ch = *p--) != '\0')
				cons_putc(ch);
			break;
		default:
			cons_putc('%');
			if (lflag)
				cons_putc('l');
			/* FALLTHROUGH */
		case '%':
			cons_putc(ch);
		}
	}
}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
static const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters an infinite loop.
 * If executing on Bochs, drop into the debugger rather than chew CPU.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	printf("panic at %s:%d: ", file, line);
	kprintf(fmt, ap);
	printf("\n");
	va_end(ap);

dead:
	/* break into Bochs debugger */
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8AE0);

	for(;;);
}

/* like panic, but don't */
void
warn(const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

int
printf(const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf(fmt, ap);
	va_end(ap);
	return 0;
}
///END

