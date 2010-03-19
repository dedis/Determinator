#if LAB >= 4
#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/unistd.h>
#include <inc/errno.h>

char buf[512];

void
wc(int fd, char *name)
{
	int i, n;
	int l, w, c, inword;

	l = w = c = 0;
	inword = 0;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		for (i=0; i<n; i++) {
			c++;
			if (buf[i] == '\n')
				l++;
			if (strchr(" \r\t\n\v", buf[i]))
				inword = 0;
			else if (!inword) {
				w++;
				inword = 1;
			}
		}
	}
	if (n < 0) {
		cprintf("wc: read error\n");
		exit(1);
	}
	printf("%d %d %d %s\n", l, w, c, name);
}

int
main(int argc, char *argv[])
{
	int fd, i;

	if (argc <= 1) {
		wc(0, "");
		return 0;
	}

	for (i = 1; i < argc; i++) {
		if ((fd = open(argv[i], O_RDONLY)) < 0) {
			cprintf("cat: cannot open %s: %s\n", argv[i],
				strerror(errno));
			exit(1);
		}
		wc(fd, argv[i]);
		close(fd);
	}
	return 0;
}

#endif	// LAB >= 4
