#if LAB >= 4
#include <inc/stat.h>
#include <inc/errno.h>
#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/string.h>
#include <inc/dirent.h>
#include <inc/assert.h>
#include <inc/args.h>

int flag[256];

void lsdir(const char *path, const char *realpath);
void lsfile(const char *path, const char *realpath);

void
ls(const char *path)
{
	int r;
	struct stat st;

	const char *realpath = path[0] ? path : ".";
	if (stat(realpath, &st) < 0)
		panic("stat %s: %s", realpath, strerror(errno));
	if (S_ISDIR(st.st_mode) && !flag['d'])
		lsdir(path, realpath);
	else
		lsfile(path, realpath);
}

void
lsdir(const char *path, const char *realpath)
{
	DIR *d;
	struct dirent *de;

	if ((d = opendir(realpath)) == NULL)
		panic("opendir %s: %s", realpath, strerror(errno));
	while ((de = readdir(d)) != NULL) {
		char depath[PATH_MAX];
		snprintf(depath, PATH_MAX, "%s%s%s", path,
			(path[0] && path[strlen(path)-1] != '/') ? "/" : "",
			de->d_name);
		lsfile(depath, depath);
	}
	closedir(d);
}

void
lsfile(const char *path, const char *realpath)
{
	char *sep;

	// Get information about the file
	struct stat st;
	if (stat(realpath, &st) < 0) {
		warn("error reading %s: %s", realpath, strerror(errno));
		return;
	}
	bool isdir = S_ISDIR(st.st_mode);

	if(flag['l'])
		printf("%11d %c ", st.st_size, isdir ? 'd' : '-');
	printf("%s", path);
	if(flag['F'] && isdir)
		printf("/");
	printf("\n");
}

void
usage(void)
{
	fprintf(stderr, "usage: ls [-dFl] [file...]\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int i;

	ARGBEGIN{
	default:
		usage();
	case 'd':
	case 'F':
	case 'l':
		flag[(uint8_t)ARGC()] = 1;
		break;
	}ARGEND

	if (argc > 0) {
		for (i=0; i<argc; i++)
			ls(argv[i]);
	} else
		ls("");

	return 0;
}

#endif /* LAB >= 4 */
