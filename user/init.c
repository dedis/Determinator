#if LAB >= 5

#include <inc/lib.h>

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
	int i, r, x, want;

	printf("init: running\n");

	want = 0xf989e;
	if ((x=sum((char*)&data, sizeof data)) != want)
		printf("init: data is not initialized: got sum %08x wanted %08x\n",
			x, want);
	else
		printf("init: data seems okay\n");
	if ((x=sum(bss, sizeof bss)) != 0)
		printf("bss is not initialized: wanted sum 0 got %08x\n", x);
	else
		printf("init: bss seems okay\n");

	printf("init: args:");
	for (i=0; i<argc; i++)
		printf(" '%s'", argv[i]);
	printf("\n");

	printf("init: running sh\n");

	// being run directly from kernel, so no file descriptors open yet
	close(0);
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);
	for (;;) {
		printf("init: starting sh\n");
		r = spawnl("sh", "sh", (char*)0);
		if (r < 0) {
			printf("init: spawn sh: %e\n", r);
			continue;
		}
		wait(r);
	}
}
#endif
