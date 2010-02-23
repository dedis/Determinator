#if LAB >= 4

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/vm.h>


void piosmain()
{
	cprintf("fs: in piosmain()\n");
}

#endif // LAB >= 4
