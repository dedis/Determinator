#if LAB >= 6
#include "lib.h"

void
umain(void)
{
	int i;
	struct Stat st;

	for (i=0; i<32; i++)
		if (fstat(i, &st) >= 0)
			printf("fd %d: name %s isdir %d size %d dev %s\n",
				i, st.st_name, st.st_isdir, st.st_size, st.st_dev->dev_name);
}
#endif
