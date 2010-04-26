
#include <stdio.h>

int
puts(const char *str)
{
	return printf("%s\n", str);
}

int
fputs(const char *str, FILE *f)
{
	return printf("%s", str);
}

