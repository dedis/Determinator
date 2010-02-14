#if LAB >= 3

#include <inc/stdio.h>

void piosmain()
{
	cprintf("testvm: in piosmain()\n");

	// check basic trap reflection again

	// check that kernel area is inaccessible to user code

	// check that ELF image was loaded correctly - proper page perms etc.
	// check that BSS was cleared

	// check copyin/copyout range checking and trap handling

	// check copy-on-write

	while (1);
}

#endif
