// buggy program - faults

#include "lib.h"

void
umain(void)
{
	*(u_int*)0 = 0;
}

