#if LAB >= 3

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/assert.h>
#include <inc/ctype.h>

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

// Dump a block of memory as 32-bit words and ASCII bytes
void
debug_dump(const char *file, int line, const void *ptr, int size)
{
	cprintf("user dump at %s:%d of memory %08x-%08x:\n",
		file, line, ptr, ptr + size);
	for (size = (size+15) & ~15; size > 0; size -= 16, ptr += 16) {
		char buf[100];

		// Hex words
		const uint32_t *v = ptr;
		sprintf(buf, "%08x: %08x %08x %08x %08x ",
			ptr, v[0], v[1], v[2], v[3]);

		// ASCII bytes
		int i; 
		const uint8_t *c = ptr;
		for (i = 0; i < 16; i++)
			buf[10+4*9+i] = isprint(c[i]) ? c[i] : '.';
		buf[10+4*9+16] = 0;

		// Print each line atomically to avoid async mixing
		cprintf("%s\n", buf);
	}
}

#endif // LAB >= 3
