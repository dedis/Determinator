#if LAB >= 4
// idle loop

#include "lib.h"

void
umain(void)
{
#if LAB >= 5
	sync();
	sys_panic("idle loop");
#else
	for(;;);
#endif
}

#endif
