#if LAB >= 4
// idle loop

#include <inc/x86.h>
#include <inc/lib.h>

void
umain(void)
{
#if LAB >= 5
	// Since we're idling, there's no point in continuing on.
	// Do some illegal I/O, which should trap back into Bochs.
	outw(0x8A00, 0x8A00);
	sys_panic("idle loop can do I/O");
#else
	for(;;);
#endif
}

#endif
