#ifndef PIOS_INC_STRING_H
#define PIOS_INC_STRING_H

#include <types.h>

int	strlen(const char *s);
char *	strcpy(char *dst, const char *src);
char *	strncpy(char *dst, const char *src, size_t size);
size_t	strlcpy(char *dst, const char *src, size_t size);
char *	strcat(char *s, const char *append);
int	strcmp(const char *s1, const char *s2);
int	strncmp(const char *s1, const char *s2, size_t size);
char *	strchr(const char *s, char c);
char *	strdup(const char *s);
char *	strstr(const char *s1, const char *s2);
char *	strpbrk(const char *s1, const char *s2);
char *	strtok(char *s, const char *sep);
char *	strtok_r(char *s, const char *sep, char **lasts);
int	strcasecmp(const char *s1, const char *s2);
int	strncasecmp(const char *s1, const char *s2, size_t n);

void *	memset(void *dst, int c, size_t len);
void *	memcpy(void *dst, const void *src, size_t len);
void *	memmove(void *dst, const void *src, size_t len);
int	memcmp(const void *s1, const void *s2, size_t len);
void *	memchr(const void *str, int c, size_t len);

long	strtol(const char *s, char **endptr, int base);

char *	strerror(int err);

#endif /* not PIOS_INC_STRING_H */
