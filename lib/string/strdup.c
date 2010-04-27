#if LAB >= 1

#include <string.h>
#include <stdlib.h>

char *	strdup(const char *s)
{
	int len = strlen(s);
	char *ns = malloc(len+1);
	if (!ns)
		return NULL;
	memcpy(ns, s, len+1);
	return ns;
}

#endif	// LAB >= 1
