#if LAB >= 5
#include "lib.h"

void
umain(void)
{
	int rfd, wfd;
	char buf[512];
	int n, r;

	if ((rfd = open("/newmotd", O_RDONLY)) < 0)
		panic("open /newmotd: %e", rfd);
	if ((wfd = open("/motd", O_RDWR)) < 0)
		panic("open /motd: %e", wfd);

	printf("OLD MOTD\n===\n");
	while ((n = read(wfd, buf, sizeof buf-1)) > 0) {
		buf[n] = 0;
		sys_cputs(buf);
	}
	printf("===\n");
	seek(wfd, 0);

	if ((r = ftruncate(wfd, 0)) < 0)
		panic("truncate /motd: %e", r);

	printf("NEW MOTD\n===\n");
	while ((n = read(rfd, buf, sizeof buf-1)) > 0) {
		buf[n] = 0;
		sys_cputs(buf);
		if ((r=write(wfd, buf, n)) != n)
			panic("write /motd: %e", r);
	}
	printf("===\n");

	if (n < 0)
		panic("read /newmotd: %e", n);

	close(rfd);
	close(wfd);
}
#endif
