// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/pmap.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	void (*func)(int argc, char **argv);
};

static struct Command commands[] = {
	{"help",	"Display this list of commands", mon_help},
	{"kerninfo",	"Display information about the kernel", mon_kerninfo},
#if SOL >= 1
	{"backtrace", "display a stack backtrace", mon_backtrace},
#endif
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))



/***** Implementations of basic kernel monitor commands *****/

void mon_help(int argc, char **argv)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		printf("%s - %s\n", commands[i].name, commands[i].desc);
}

void mon_kerninfo(int argc, char **argv)
{
	extern char _start[], etext[], edata[], end[];

	printf("Special kernel symbols:\n");
	printf("  _start %08x (virt)  %08x (phys)\n", _start, _start-KERNBASE);
	printf("  etext  %08x (virt)  %08x (phys)\n", etext, etext-KERNBASE);
	printf("  edata  %08x (virt)  %08x (phys)\n", edata, edata-KERNBASE);
	printf("  end    %08x (virt)  %08x (phys)\n", end, end-KERNBASE);
	printf("Kernel executable memory footprint: %dKB\n",
		(end-_start+1023)/1024);
}

void mon_backtrace(int argc, char **argv)
{
#if SOL >= 1
	u_int *ebp = (u_int*)read_ebp();
	int i;

	printf("Stack backtrace:\n");
	while (ebp) {

		// print this stack frame
		printf("  ebp %08x  eip %08x  args", ebp, ebp[1]);
		for (i = 0; i < 5; i++)
			printf(" %08x", ebp[2+i]);
		printf("\n");

		// move to next lower stack frame
		ebp = (u_int*)ebp[0];
	}
#else
	// Your code here.
#endif // SOL >= 1
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n"
#define MAXARGS 16

static void
runcmd(char *buf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			printf("Too many arguments (max %d)\n", MAXARGS);
			return;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0) {
			commands[i].func(argc, argv);
			return;
		}
	}
	printf("Unknown command '%s'\n", argv[0]);
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	printf("Welcome to the 6.828 kernel monitor!\n");
	printf("Type 'help' for a list of commands.\n");

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			runcmd(buf);
	}
}

