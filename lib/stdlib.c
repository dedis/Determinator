#if LAB >= 4
/*
 * Basic C standard library functions.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#include <inc/cdefs.h>
#include <inc/file.h>
#include <inc/stdlib.h>
#include <inc/syscall.h>
#include <inc/assert.h>
#include <inc/string.h>

void gcc_noreturn
exit(int status)
{
	// To exit a PIOS user process, by convention,
	// we just set our exit status in our filestate area
	// and return to our parent process.
	files->status = status;
	files->exited = 1;
	sys_ret();
	panic("exit: sys_ret shouldn't have returned");
}

void gcc_noreturn
abort(void)
{
	exit(EXIT_FAILURE);
}


#endif	// LAB >= 4
