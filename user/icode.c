#if LAB >= 5

#include <inc/lib.h>

void
umain(void)
{
	int fd, n, r;
	char buf[512+1];

	sys_cputs("icode startup\n");

	// need to do some of the initialization here, 
	//because we are not called from init or the shell.
	binaryname = "icode";
	opencons();

	printf("icode: open /motd\n");
	if ((fd = open("/motd", O_RDONLY)) < 0)
		panic("icode: open /motd: %e", fd);

	printf("icode: read /motd\n");
	while ((n = read(fd, buf, sizeof buf-1)) > 0){
		buf[n] = 0;
		sys_cputs(buf);
	}

	printf("icode: close /motd\n");
	close(fd);

	printf("icode: spawn /init\n");
	if ((r = spawnl("/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
		panic("icode: spawn /init: %e", r);

	printf("icode: exiting\n");
}
#endif
