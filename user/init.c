#if LAB >= 5
#include "lib.h"

struct {
	char msg1[5000];
	char msg2[1000];
} data = {
	"this is initialized data",
	"so is this"
};

char bss[6000];

int
sum(char *s, int n)
{
	int i, tot;

	tot = 0;
	for(i=0; i<n; i++)
		tot ^= i*s[i];
	return tot;
}
		
void
umain(int argc, char **argv)
{
	int i, x, want;

	printf("/init: running\n");

	want = 0x40795;
	if ((x=sum((char*)&data, sizeof data)) != want)
		printf("/init: data is not initialized: got sum %08x wanted %08x\n",
			x, want);
	else
		printf("/init: data seems okay\n");
	if ((x=sum(bss, sizeof bss)) != 0)
		printf("bss is not initialized: wanted sum 0 got %08x\n", x);
	else
		printf("/init: bss seems okay\n");

	printf("/init: args:");
	for (i=0; i<argc; i++)
		printf(" '%s'", argv[i]);
	printf("\n");

	printf("/init: exiting\n");
}
#endif
