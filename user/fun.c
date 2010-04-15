#include <inc/stdlib.h>
#include <inc/stdio.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/file.h>
#include <inc/errno.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/mmu.h>
#include <inc/vm.h>

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)


pid_t fork_opt(int opt_cmd)
{
	int i;

	// Find a free child process slot.
	// We just use child process slot numbers as Unix PIDs,
	// even though child slots are process-local in PIOS
	// whereas PIDs are global in Unix.
	// This means that commands like 'ps' and 'kill'
	// have to be shell-builtin commands under PIOS.
	pid_t pid;
	for (pid = 1; pid < 256; pid++)
		if (files->child[pid].state == PROC_FREE)
			break;
	if (pid == 256) {
		warn("fork: no child process available");
		errno = EAGAIN;
		return -1;
	}

	// Set up the register state for the child
	struct cpustate cs;
	memset(&cs, 0, sizeof(cs));

	// Use some assembly magic to propagate registers to child
	// and generate an appropriate starting eip
	int isparent;
	asm volatile(
		"	movl	%%esi,%0;"
		"	movl	%%edi,%1;"
		"	movl	%%ebp,%2;"
		"	movl	%%esp,%3;"
		"	movl	$1f,%4;"
		"	movl	$1,%5;"
		"1:	"
		: "=m" (cs.tf.tf_regs.reg_esi),
		  "=m" (cs.tf.tf_regs.reg_edi),
		  "=m" (cs.tf.tf_regs.reg_ebp),
		  "=m" (cs.tf.tf_esp),
		  "=m" (cs.tf.tf_eip),
		  "=a" (isparent)
		:
		: "ebx", "ecx", "edx");
	if (!isparent) {
		// Clear our child state array, since we have no children yet.
		memset(&files->child, 0, sizeof(files->child));
		files->child[0].state = PROC_RESERVED;
		for (i = 1; i < FILE_INODES; i++)
			if (fileino_alloced(i)) {
				files->fi[i].rino = i;	// 1-to-1 mapping
				files->fi[i].rver = files->fi[i].ver;
				files->fi[i].rlen = files->fi[i].size;
			}

		return 0;	// indicate that we're the child.
	}

	// Copy our entire user address space into the child and start it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(opt_cmd | SYS_REGS | SYS_COPY | SYS_START, pid, &cs,
		ALLVA, ALLVA, ALLSIZE);

	// Record the inode generation numbers of all inodes at fork time,
	// so that we can reconcile them later when we synchronize with it.
	memset(&files->child[pid], 0, sizeof(files->child[pid]));
	files->child[pid].state = PROC_FORKED;

	return pid;
}



int main(int argc, char ** argv) {

	int fd, written;
	char * filename;
	char * text = "exegi monumentum aere perennius\n";

	printf("This is a test.\n");
	if (argc > 1)
		filename = argv[1];
	else filename = "boo.txt";
	fd = open(filename, O_CREAT | O_WRONLY);
	if (fd == -1)
		panic("open: %s", strerror(errno));
	written = write(fd, text, strlen(text) + 1);
	if (written != strlen(text) + 1) 
		printf("written: %d  text length: %d\n", written, strlen(text));
	close(fd);


	int r;
	if ((r = fork_opt(0)) < 0)
	  panic("fork: %e", r);
	if (r == 0) {
	  printf("Child\n");
	}
	else {
	  printf("Parent\n");
	  waitpid(r, NULL, 0);
	}


	return EXIT_SUCCESS;
}
