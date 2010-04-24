/* See COPYRIGHT for copyright information. */

#ifndef PIOS_INC_ASSERT_H
#define PIOS_INC_ASSERT_H

#include <inc/stdio.h>
#include <inc/cdefs.h>

void debug_warn(const char*, int, const char*, ...);
void debug_panic(const char*, int, const char*, ...) gcc_noreturn;

#define warn(...)	debug_warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...)	debug_panic(__FILE__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG
#define assert(x)		\
	do { if (!(x)) panic("assertion failed: %s", #x); } while (0)
#else
#define assert(x)		// do nothing
#endif

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)	switch (x) case 0: case (x):

#endif /* !PIOS_INC_ASSERT_H */
