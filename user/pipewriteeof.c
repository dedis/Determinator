#if LAB >= 6
#include "lib.h"

void
umain(void)
{
	int i, p[2];

	if ((i=pipe(p)) < 0)
		panic("pipe: %e", i);

	if ((i=fork()) < 0)
		panic("fork: %e", i);

	if (i == 0) {
		close(p[0]);
		for(;;){
			printf(".");
			if(write(p[1], "x", 1) != 1)
				break;
		}
		printf("\npipe write closed\n");
	}
}
#endif
