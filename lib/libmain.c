#if LAB >= 3
// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, vpd, and vpt.

#include <inc/lib.h>

extern void umain(int, char**);

struct Env *env;
char *binaryname = "(PROGRAM NAME UNKNOWN)";

void
libmain(int argc, char **argv)
{
	// set env to point at our env structure in envs[].
#if SOL >= 3
	env = &envs[ENVX(sys_getenvid())];
#else
	env = 0;	// Your code here.
#endif

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

#endif	// LAB >= 3
