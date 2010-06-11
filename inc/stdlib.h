/*
 * Generic C standard library definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_STDLIB_H
#define PIOS_INC_STDLIB_H

#include <inc/gcc.h>
#include <inc/types.h>
#include <inc/stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#define EXIT_SUCCESS	0	// Success status for exit()
#define EXIT_FAILURE	1	// Failure status for exit()


// lib/stdlib.c
void	exit(int status) gcc_noreturn;
void	abort(void) gcc_noreturn;

#endif /* !PIOS_INC_STDLIB_H */
