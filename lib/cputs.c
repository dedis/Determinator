/*
 * User-space implementation of cputs() for console output,
 * which just feeds the string to the sys_cputs() system call.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 */

#include <inc/stdio.h>
#include <inc/syscall.h>

void cputs(const char *str)
{
	sys_cputs(str);
}

