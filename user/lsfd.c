#if LAB >= 5

#include <inc/lib.h>

void
usage(void)
{
	printf("usage: lsfd [-1]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i, usefprint;
	struct Stat st;

	usefprint = 0;
	ARGBEGIN{
	default:
		usage();
	case '1':
		usefprint = 1;
		break;
	}ARGEND

	for (i=0; i<32; i++)
		if (fstat(i, &st) >= 0) {
			if (usefprint)
				fprintf(1, "fd %d: name %s isdir %d size %d dev %s\n",
					i, st.st_name, st.st_isdir,
					st.st_size, st.st_dev->dev_name);	
			else
				printf("fd %d: name %s isdir %d size %d dev %s\n",
					i, st.st_name, st.st_isdir,
					st.st_size, st.st_dev->dev_name);
		}
}
#endif
