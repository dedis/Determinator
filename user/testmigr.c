#if LAB >= 5

#include <inc/stdio.h>
#include <inc/syscall.h>

void migrate(int node)
{
	cprintf("testmigr: migrating to node %d...\n", node);
	sys_get(0, (node << 8) | 0, NULL, NULL, NULL, 0);
	cprintf("testmigr: now on node %d.\n", node);
}

int
main()
{
	// Basic migration test
	migrate(1);
	migrate(2);
	migrate(1);
	migrate(2);

	printf("testmigr done\n");
	return 0;
}

#endif	// LAB >= 5
