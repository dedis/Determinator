#if LAB >= 4

#include <inc/lib.h>

char *argv0;

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

	n = 0;
	if (argv0)
		n += snprintf(buf+n, sizeof buf-n, "%s: ", argv0);
	n += snprintf(buf+n, sizeof buf-n, "user panic at %s:%d: ", file, line);
	n += vsnprintf(buf+n, sizeof buf-n, fmt, ap);
	n += snprintf(buf+n, sizeof buf-n, "\n");
	va_end(ap);
	sys_cputs(buf);

	for(;;);
}

#endif // LAB >= 4
