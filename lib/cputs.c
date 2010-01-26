// User-space implementation of cputs() for console output,
// which just feeds the string to the sys_cputs() system call.

#include <inc/stdio.h>
#include <inc/syscall.h>

void cputs(const char *str)
{
	sys_cputs(str);
}

