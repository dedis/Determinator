#if LAB >= 4

#include <inc/file.h>
#include <inc/syscall.h>
#include <inc/assert.h>

void
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

#endif	// LAB >= 4
