/*
 * Assertions and other debugging macros.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_ASSERT_H
#define PIOS_INC_ASSERT_H

#include <stdio.h>
#include <cdefs.h>

// The cputs() function is the basic debug-printing function in PIOS.
// Implemented in kern/console.c (kernel) or lib/cputs.c (user-level code).
void	cputs(const char *str);
#define CPUTS_MAX	256	// Max buffer length cputs() will accept

void debug_warn(const char*, int, const char*, ...);
void debug_panic(const char*, int, const char*, ...) gcc_noreturn;
void debug_dump(const char*, int, const void *ptr, int size);

#define warn(...)	debug_warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...)	debug_panic(__FILE__, __LINE__, __VA_ARGS__)
#define dump(...)	debug_dump(__FILE__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG
#define assert(x)		\
	do { if (!(x)) panic("assertion failed: %s", #x); } while (0)
#else
#define assert(x)		// do nothing
#endif

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)	switch (x) case 0: case (x):

#endif /* !PIOS_INC_ASSERT_H */
