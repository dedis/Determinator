#if LAB >= 4
// Process management.
// See COPYRIGHT for copyright information.

#include <inc/x86.h>
#include <inc/stat.h>
#include <inc/fcntl.h>
#include <inc/string.h>
#include <inc/syscall.h>
#include <inc/dirent.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/file.h>
#include <kern/init.h>
#include <kern/console.h>


// Build a table of files to include in the initial file system.
#define INITFILE(name)	\
	extern char _binary_obj_user_##name##_start[]; \
	extern char _binary_obj_user_##name##_end[];
#include <obj/kern/initfiles.h>
#undef INITFILE

#define INITFILE(name)	\
	{ #name, _binary_obj_user_##name##_start, \
		_binary_obj_user_##name##_end },
char *initfiles[][3] = {
	#include <obj/kern/initfiles.h>
};
#undef INITFILE


static spinlock file_lock;	// Lock to protect file I/O state
static size_t file_consout;	// Bytes written to console so far



void
file_init(void)
{
	spinlock_init(&file_lock);
}

void
file_initroot(proc *root)
{
	// Only one root process may perform external I/O directly -
	// all other processes do I/O indirectly via the process hierarchy.
	assert(root == proc_root);

	// Make sure the root process's page directory is active
	cpu_cur()->proc = root;
	lcr3(mem_phys(root->pdir));

	// Enable read/write access on the file metadata area
	pmap_setperm(root->pdir, FILESVA, ROUNDUP(sizeof(filestate), PAGESIZE),
				SYS_READ | SYS_WRITE);
	memset(files, 0, sizeof(*files));

	// Set up the standard I/O descriptors for console I/O
	files->fd[0].ino = FILEINO_CONSIN;
	files->fd[0].flags = O_RDONLY;
	files->fd[1].ino = FILEINO_CONSOUT;
	files->fd[1].flags = O_WRONLY | O_APPEND;
	files->fd[2].ino = FILEINO_CONSOUT;
	files->fd[2].flags = O_WRONLY | O_APPEND;

	// Setup the inodes for the console I/O files and root directory
	strcpy(files->fi[FILEINO_CONSIN].de.d_name, "consin");
	strcpy(files->fi[FILEINO_CONSOUT].de.d_name, "consout");
	strcpy(files->fi[FILEINO_ROOTDIR].de.d_name, "/");
	files->fi[FILEINO_CONSIN].dino = FILEINO_ROOTDIR;
	files->fi[FILEINO_CONSOUT].dino = FILEINO_ROOTDIR;
	files->fi[FILEINO_ROOTDIR].dino = FILEINO_ROOTDIR;
	files->fi[FILEINO_CONSIN].mode = S_IFREG | S_IFPART;
	files->fi[FILEINO_CONSOUT].mode = S_IFREG;
	files->fi[FILEINO_ROOTDIR].mode = S_IFDIR;

	// Set up the initial files in the root process's file system
	int ninitfiles = sizeof(initfiles)/sizeof(initfiles[0]);
	int i;
	for (i = 0; i < ninitfiles; i++) {
		int ino = FILEINO_GENERAL+i;
		int filesize = initfiles[i][2] - initfiles[i][1];
		strcpy(files->fi[ino].de.d_name, initfiles[i][0]);
		files->fi[ino].mode = S_IFREG;
		files->fi[ino].size = filesize;
		pmap_setperm(root->pdir, (uintptr_t)FILEDATA(ino),
					ROUNDUP(filesize, PAGESIZE),
					SYS_READ | SYS_WRITE);
		memcpy(FILEDATA(ino), initfiles[i][1], filesize);
	}

	// Set root process's current working directory
	files->cwd = FILEINO_ROOTDIR;

	// Child process state - reserve PID 0 as a "scratch" child process.
	files->child[0].state = PROC_RESERVED;
#if LAB >= 99

	// Increase the root process's stack size to 64KB.
	// XXX should just do this while loading the program.
	pmap_setperm(root->pdir, VM_STACKHI - 65536, 65536,
			SYS_READ | SYS_WRITE);
#endif
}

// Called from proc_ret() when the root process "returns" -
// this function performs any new output the root process requested,
// or if it didn't request output, puts the root process to sleep
// waiting for input to arrive from some I/O device.
void
file_io(trapframe *tf)
{
	proc *cp = proc_cur();
	assert(cp == proc_root);	// only root process should do this!

	// Note that we're not going to bother protecting ourselves
	// against memory access traps while accessing user memory here,
	// because we consider the root process a special, "trusted" process:
	// the whole system goes down anyway if the root process goes haywire.
	// This is very different from handling system calls
	// on behalf of arbitrary processes that might be buggy or evil.

	// Perform I/O with whatever devices we have access to.
	bool iodone = 0;
	iodone |= cons_io();
#if LAB >= 99
	iodone |= net_io();
#endif

	// Has the root process exited?
	if (files->exited) {
		cprintf("root process exited with status %d\n", files->status);
		done();
	}

	// We successfully did some I/O, let the root process run again.
	if (iodone)
		trap_return(tf);

	// No I/O ready - put the root process to sleep waiting for I/O.
	spinlock_acquire(&file_lock);
	cp->state = PROC_STOP;		// we're becoming stopped
	cp->runcpu = NULL;		// no longer running
	cp->tf = *tf;			// save our register state
	spinlock_release(&file_lock);

	proc_sched();			// go do something else
}

// Check to see if any input is available for the root process
// and if the root process is waiting for it, and if so, wake the process.
void
file_wakeroot(void)
{
	spinlock_acquire(&file_lock);
	if (proc_root && proc_root->state == PROC_STOP)
		proc_ready(proc_root);
	spinlock_release(&file_lock);
}

#endif // LAB >= 4
