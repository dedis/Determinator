#ifndef JOS_INC_STRING_H
#define JOS_INC_STRING_H

#include <inc/types.h>

int		strlen(const char *s);
char *		strcpy(char *dest, const char *src);
int		strcmp(const char *s1, const char *s2);
char *	strchr(const char *s, char c);
long		strtol(const char *s, char **endptr, int base);

void *		memset(void *dest, int, size_t len);
void *		memcpy(void *dest, const void *src, size_t len);

#endif /* not JOS_INC_STRING_H */
