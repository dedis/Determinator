// "Real function" versions of getc and getchar for apps that need them.

#include <stdio.h>

#undef getc
#undef getchar

int
getc(FILE *f)
{
	return fgetc(f);
}

int
getchar(void)
{
	return fgetc(stdin);
}

