#if LAB >= 3

#include <inc/lib.h>

char *argv0;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: <message>", then causes a breakpoint exception,
 * which causes JOS to enter the JOS kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);

	// Print the panic message
	if (argv0)
		printf("%s: ", argv0);
	printf("user panic in %s at %s:%d: ", binaryname, file, line);
	vprintf(fmt, ap);
	printf("\n");

	// Cause a breakpoint exception
	for (;;)
		asm volatile("int3");
}

#endif // LAB >= 3
