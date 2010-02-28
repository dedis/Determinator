#if LAB >= 3

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/assert.h>

char *argv0;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: <message>", then causes a breakpoint exception.
 */
void
debug_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;
	va_start(ap, fmt);

	// Print the panic message
	if (argv0)
		cprintf("%s: ", argv0);
	cprintf("user panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");

	abort();
}

/* like panic, but don't */
void
debug_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("user warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}

#endif // LAB >= 3
