/* See COPYRIGHT for copyright information. */

#ifndef _PRINTF_H_
#define _PRINTF_H_

void warn(const char *, ...)
	__attribute__((format(printf, 1, 2)));
void _panic(const char *, int, const char *, ...) 
	__attribute__((noreturn))
	__attribute__((format(printf, 3, 4)));
int printf(const char *, ...)
	__attribute__((format(printf, 1, 2)));

#define assert(x)	\
	do {	if (!(x)) panic("assertion failed: %s", #x); } while (0)

#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

#endif
