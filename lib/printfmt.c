#if LAB >= 1
// Stripped-down primitive printf-style formatting routines,
// used in common by printf, sprintf, fprintf, etc.
// This code is also used by both the kernel and user programs.

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/error.h>

/*
 * Space or zero padding and a field width are supported for the numeric
 * formats only. 
 * 
 * The special format %e takes an integer error code
 * and prints a string describing the error.
 * The integer may be positive or negative,
 * so that -E_NO_MEM and E_NO_MEM are equivalent.
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
	"unexpected end of file",
#if LAB >= 5
	"no free space on disk",
	"too many files are open",
	"file or block not found",
	"invalid path",
	"file already exists",
	"file is not a valid executable",
#endif
};

/*
 * Print a number (base <= 16) in reverse order,
 * using specified putch function and associated pointer putdat.
 */
static void
printnum(void (*putch)(int, void*), void *putdat,
	 unsigned long long num, unsigned base, int width, int padc)
{
	// first recursively print all preceding (more significant) digits
	if (num >= base) {
		printnum(putch, putdat, num / base, base, width-1, padc);
	} else {
		// print any needed pad characters before first digit
		while (--width > 0)
			putch(padc, putdat);
	}

	// then print this (the least significant) digit
	putch("0123456789abcdef"[num % base], putdat);
}

// Get an int of various possible sizes from a varargs list,
// depending on the lflag parameter.
static unsigned long long
getint(va_list *ap, int lflag)
{
	if (lflag >= 2)
		return va_arg(*ap, unsigned long long);
	else if (lflag)
		return va_arg(*ap, unsigned long);
	else
		return va_arg(*ap, unsigned int);
}


// Main function to format and print a string.
void
printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);

void
vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap)
{
	register char *p;
	register int ch, err;
	unsigned long long num;
	int base, lflag, width;
	char padc;

	for (;;) {
		while ((ch = *(u_char *) fmt++) != '%') {
			if (ch == '\0')
				return;
			putch(ch, putdat);
		}

		// Process a %-escape sequence
		padc = ' ';
		width = 0;
		lflag = 0;
	reswitch:
		switch (ch = *(u_char *) fmt++) {

		// flag to pad with 0's instead of spaces
		case '0':
			padc = '0';
			goto reswitch;

		// width field
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

		// long flag (doubled for long long)
		case 'l':
			lflag++;
			goto reswitch;

		// character
		case 'c':
			putch(va_arg(ap, int), putdat);
			break;

		// error message
		case 'e':
			err = va_arg(ap, int);
			if(err < 0)
				err = -err;
			if(err > MAXERROR || (p = error_string[err]) == NULL)
				printfmt(putch, putdat, "error %d", err);
			else
				printfmt(putch, putdat, "%s", p);
			break;

		// string
		case 's':
			if ((p = va_arg(ap, char *)) == NULL)
					p = "(null)";
			while ((ch = *p++) != '\0')
				putch(ch, putdat);
			break;

		// (signed) decimal
		case 'd':
			num = getint(&ap, lflag);
			if ((long long)num < 0) {
				putch('-', putdat);
				num = -(long long) num;
			}
			base = 10;
			goto number;

		// unsigned decimal
		case 'u':
			num = getint(&ap, lflag);
			base = 10;
			goto number;

		// (unsigned) octal
		case 'o':
#if SOL >= 1
			num = getint(&ap, lflag);
			base = 8;
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
				(u_long)va_arg(ap, void *);
			base = 16;
			goto number;

		// (unsigned) hexadecimal
		case 'x':
			num = getint(&ap, lflag);
			base = 16;
		number:
			printnum(putch, putdat, num, base, width, padc);
			break;

		// unrecognized escape sequence - just print it literally
		default:
			putch('%', putdat);
			while (lflag-- > 0)
				putch('l', putdat);
			/* FALLTHROUGH */

		// escaped '%' character
		case '%':
			putch(ch, putdat);
		}
	}
}

void
printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintfmt(putch, putdat, fmt, ap);
	va_end(ap);
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
vsnprintf(char *buf, int n, const char *fmt, va_list ap)
{
	struct sprintbuf b = {buf, buf+n-1, 0};

	if (buf == NULL || n < 1)
		return -E_INVAL;

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
