// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/pmap.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#if LAB >= 2
#include <kern/trap.h>
#endif

#define CMDBUF_SIZE	80	// enough for one VGA text line

#if SOL >= 2
static int mon_exit(int argc, char** argv, struct Trapframe* tf);
#endif

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
#if SOL >= 1
	{ "backtrace", "Display a stack backtrace", mon_backtrace },
#endif
#if SOL >= 2
	{ "exit", "Exit the kernel monitor", mon_exit },
#endif
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))



/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char** argv, struct Trapframe* tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		printf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char** argv, struct Trapframe* tf)
{
	extern char _start[], etext[], edata[], end[];

	printf("Special kernel symbols:\n");
	printf("  _start %08x (virt)  %08x (phys)\n", _start, _start - KERNBASE);
	printf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	printf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	printf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	printf("Kernel executable memory footprint: %dKB\n",
		(end-_start+1023)/1024);
	return 0;
}

int
mon_backtrace(int argc, char** argv, struct Trapframe* tf)
{
#if SOL >= 1
#if SOL >= 2
	const uint32_t* ebp = (tf ? (const uint32_t*) tf->tf_ebp : (const uint32_t*) read_ebp());
#else
	const uint32_t* ebp = (const uint32_t*) read_ebp();
#endif
	int i;

	printf("Stack backtrace:\n");
	while (ebp) {

		// print this stack frame
		printf("  ebp %08x  eip %08x  args", ebp, ebp[1]);
		for (i = 0; i < 5; i++)
			printf(" %08x", ebp[2+i]);
		printf("\n");

		// move to next lower stack frame
		ebp = (const uint32_t*) ebp[0];
	}
#else
	// Your code here.
#endif // SOL >= 1
	return 0;
}

#if SOL >= 2
int
mon_exit(int argc, char** argv, struct Trapframe* tf)
{
	return -1;
}
#endif


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char* buf, struct Trapframe* tf)
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
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	printf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe* tf)
{
	char* buf;

	printf("Welcome to the JOS kernel monitor!\n");
	printf("Type 'help' for a list of commands.\n");

#if LAB >= 2
	if (tf != NULL)
		print_trapframe(tf);
#endif	// LAB >= 2

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

