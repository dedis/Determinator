// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/mmu.h>
#include <inc/assert.h>

#include <kern/console.h>
#include <kern/monitor.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	void (*func)(int argc, char **argv);
};

static struct Command commands[] = {
	{"help", "display this list of commands", mon_help},
	{"kerninfo", "display information about the kernel", mon_kerninfo},
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
	extern char start[], etext[], edata[], end[];

	printf("Special kernel symbols:\n");
	printf("  start  %08x (virt)  %08x (phys)\n", start, start-KERNBASE);
	printf("  etext  %08x (virt)  %08x (phys)\n", etext, etext-KERNBASE);
	printf("  edata  %08x (virt)  %08x (phys)\n", edata, edata-KERNBASE);
	printf("  end    %08x (virt)  %08x (phys)\n", end, end-KERNBASE);
	printf("Kernel executable memory footprint: %dKB\n",
		(end-start+1023)/1024);
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

