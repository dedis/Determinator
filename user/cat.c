#if LAB >= 4
#include <inc/string.h>
#include <inc/unistd.h>
#include <inc/assert.h>
#include <inc/errno.h>

char buf[8192];

void
cat(int f, char *s)
{
	long n;
	int r;

	while ((n = read(f, buf, sizeof(buf))) > 0)
		if (write(1, buf, n) != n)
			panic("write error copying %s: %s", s, strerror(errno));
	if (n < 0)
		panic("error reading %s: %s", s, strerror(errno));
}

int
main(int argc, char **argv)
{
	int f, i;

	if (argc == 1)
		cat(0, "<stdin>");
	else
		for (i = 1; i < argc; i++) {
			f = open(argv[i], O_RDONLY);
			if (f < 0)
				panic("can't open %s: %s", argv[i],
					strerror(errno));
			else {
				cat(f, argv[i]);
				close(f);
			}
		}

	return 0;
}

#endif	// LAB >= 4
