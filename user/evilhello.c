#if LAB >= 4
// evil hello world -- kernel pointer passed to kernel
// kernel should destroy user environment in response

#include "lib.h"

void
umain(void)
{
	// try to print the kernel entry point as a string!  mua ha ha!
	sys_cputs((char*)0xf0100020);
}

#endif
