#if LAB >= 1
// Basic string routines.  Not hardware optimized, but not shabby.

#include <inc/string.h>

int
strlen(const char *s)
{
	int n;

	for (n=0; *s; s++)
		n++;
	return n;
}

char*
strcpy(char *dst, const char *src)
{
	char *ret;

	ret = dst;
	while ((*dst++ = *src++) != 0)
		;
	return ret;
}

int
strcmp(const char *p, const char *q)
{
	while (*p && *p == *q)
		p++, q++;
	if ((u_int)*p < (u_int)*q)
		return -1;
	if ((u_int)*p > (u_int)*q)
		return 1;
	return 0;
}

char*
strchr(const char *s, char c)
{
	for(; *s; s++)
		if(*s == c)
			return (char*)s;
	return 0;
}

long
strtol(const char *s, char **endptr, int base)
{
	int neg = 0;
	long val = 0;

	// gobble initial whitespace
	while (*s == ' ' || *s == '\t')
		s++;

	// plus/minus sign
	if (*s == '+')
		s++;
	else if (*s == '-')
		s++, neg = 1;

	// hex or octal base prefix
	if ((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x'))
		s += 2, base = 16;
	else if (base == 0 && s[0] == '0')
		s++, base = 8;
	else if (base == 0)
		base = 10;

	// digits
	while (1) {
		int dig;

		if (*s >= '0' && *s <= '9')
			dig = *s - '0';
		else if (*s >= 'a' && *s <= 'z')
			dig = *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'Z')
			dig = *s - 'A' + 10;
		else
			break;
		if (dig >= base)
			break;
		s++, val = (val * base) + dig;
		// we don't properly detect overflow!
	}

	*endptr = (char*)s;
	return val;
}

void *
memset(void *v, int c, size_t n)
{
	char *p;
	int m;

	p = v;
	m = n;
	while (--m >= 0)
		*p++ = c;

	return v;
}

void *
memcpy(void *dst, const void *src, size_t n)
{
	const char *s;
	char *d;
	int m;

	s = src;
	d = dst;
	m = n;
	while (--m >= 0)
		*d++ = *s++;

	return dst;
}

#endif
