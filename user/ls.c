#if LAB >= 6
#include "lib.h"

int flag[256];

void
ls(char *path, char *prefix)
{
	int r;
	struct File f;

	if ((r=stat(path, &f)) < 0)
		panic("stat %s: %e", path, r);
	if (f.f_isdir && !flag['d'])
		lsdir(path, prefix);
	else
		ls1(0, 0, f.f_size, path);
}

void
lsdir(char *path, char *prefix)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, r);

	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		ls1(prefix, f.f_isdir, f.f_size, f.f_name);
	if (n > 0)
		panic("short read in directory %s", path);
	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}

void
ls1(char *prefix, uint isdir, uint size, char *name)
{
	if(flag['l'])
		printf("%11d ");
	if(prefix)
		printf("%s/", prefix);
	printf("%s", name);
	if(flag['F'] && isdir)
		printf("/");
	printf("\n");
}

void
usage(void)
{
	print("usage: ls [-dFl] [file...]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	ARGBEGIN{
	default:
		usage();
	case 'd':
	case 'F':
	case 'l':
		flag[ARGC()]++;
		break;
	}ARGEND

	if (argc == 0)
		ls(".", "");
	else {
		for (i=0; i<argc; i++)
			ls(argv[i], argv[i]);
	}
}

#endif /* LAB >= 6 */

