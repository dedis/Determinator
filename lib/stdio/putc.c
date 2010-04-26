// "Real function" versions of putc and putchar for apps that need them.

#include <stdio.h>

#undef putc
#undef putchar

int
putc(int c, FILE *f)
{
	return fputc(c, f);
}

int
putchar(int c)
{
	return fputc(c, stdout);
}

