#if LAB >= 6
#include "lib.h"

#define VA (0xA0000000)
char *msg = "hello, world\n";
char *msg2 = "goodbye, world\n";

void childofspawn(void);

void
umain(int argc, char **argv)
{
	int r;
	char *va;

	if (argc != 0)
		childofspawn();

	if ((r = sys_mem_alloc(0, VA, PTE_P|PTE_W|PTE_U|PTE_LIBRARY)) < 0)
		panic("sys_mem_alloc: %e", r);
	va = (char*)VA;

	// check fork
	if ((r = fork()) < 0)
		panic("fork: %e", r);
	if (r == 0) {
		strcpy(va, msg);
		exit();
	}
	wait(r);
	printf("fork handles PTE_LIBRARY %s\n", strcmp(va, msg) == 0 ? "right" : "wrong");

	// check spawn
	if ((r = spawnl("/testptelibrary", "testptelibrary", "arg", 0)) < 0)
		panic("spawn: %e", r);
	wait(r);
	printf("spawn handles PTE_LIBRARY %s\n", strcmp(va, msg2) == 0 ? "right" : "wrong");
}

void
childofspawn(void)
{
	strcpy((char*)VA, msg2);
	exit();
}
#endif
