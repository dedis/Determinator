/* See COPYRIGHT for copyright information. */

#ifndef _INC_ASSERT_H_
#define _INC_ASSERT_H_

#include <inc/stdio.h>

void _warn(const char *, int, const char *, ...);
void _panic(const char *, int, const char *, ...) 
	__attribute__((noreturn));

#define warn(...) _warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)	\
	do {	if (!(x)) panic("assertion failed: %s", #x); } while (0)

#endif
