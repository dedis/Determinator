#if LAB >= 4
#include <inc/stdio.h>
#include <inc/string.h>

int
main(int argc, char **argv)
{
	int i, nflag;
	int (*pr)(const char *fmt, ...) = printf;

	nflag = 0;
	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == 'n')
			nflag = 1;
		else if (argv[1][1] == 'c')
			pr = cprintf;
		else
			break;
		argc--;
		argv++;
	}

	for (i = 1; i < argc; i++) {
		if (i > 1)
			pr(" ");
		pr("%s", argv[i]);
	}
	if (!nflag)
		pr("\n");

	return 0;
}
#endif
