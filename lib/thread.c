#if LAB >= 5
/*
 * Simple "thread" fork/join functions for PIOS.
 * Since the PIOS doesn't actually allow multiple threads
 * to run in one process and share memory directly,
 * these functions use the kernel's SNAP and MERGE features
 * to emulate threads in a deterministic consistency model.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/string.h>
#include <inc/unistd.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/vm.h>
#include <inc/file.h>
#include <inc/errno.h>
#if LAB >= 9
#include <inc/pthread.h>
#endif

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

#define SHAREVA		((void*) VM_SHARELO)
#define SHARESIZE	(VM_SHAREHI - VM_SHARELO)


#define EXIT_BARRIER 0x80000000 // Highest bit set to indicate sys_ret at barrier.

static int thread_id;


// Fork a child process/thread, returning 0 in the child and 1 in the parent.
int
tfork(uint16_t child)
{
	// Set up the register state for the child
	struct procstate ps;
	memset(&ps, 0, sizeof(ps));

	// Use some assembly magic to propagate registers to child
	// and generate an appropriate starting eip
	int isparent;
	asm volatile(
		"	movq	%%rsi,%0;"
		"	movq	%%rdi,%1;"
		"	movq	%%rbp,%2;"
		"	movq	%%rsp,%3;"
		"	movq	$1f,%4;"
		"	movl	$1,%5;"
		"1:	"
		: "=m" (ps.tf.rsi),
		  "=m" (ps.tf.rdi),
		  "=m" (ps.tf.rbp),
		  "=m" (ps.tf.rsp),
		  "=m" (ps.tf.rip),
		  "=a" (isparent)
		:
		: "rbx", "rcx", "rdx");
	if (!isparent) {
#if LAB >= 9
		files->thself = child;
#endif
		return 0;	// in the child
	}

	// Fork the child, copying our entire user address space into it.
	ps.tf.rax = 0;	// isparent == 0 in the child
	sys_put(SYS_REGS | SYS_COPY | SYS_SNAP | SYS_START, child,
 		&ps, ALLVA, ALLVA, ALLSIZE);

	return 1;
}

void
tjoin(uint16_t child)
{
	// Wait for the child and retrieve its CPU state.
	// If merging, leave the highest 4MB containing the stack unmerged,
	// so that the stack acts as a "thread-private" memory area.
	struct procstate ps;
	sys_get(SYS_MERGE | SYS_REGS, child, &ps, SHAREVA, SHAREVA, SHARESIZE);

	// Make sure the child exited with the expected trap number
	if (ps.tf.trapno != T_SYSCALL) {
		cprintf("  rip  0x%08x\n", ps.tf.rip);
		cprintf("  rsp  0x%08x\n", ps.tf.rsp);
		panic("tjoin: unexpected trap %d, expecting %d\n",
			ps.tf.trapno, T_SYSCALL);
	}
}

#if LAB >= 9
void
tresume(uint16_t child)
{
	// Restart a child after it has stopped.
	// Refresh child's memory state to match parent's.
	sys_put( SYS_COPY | SYS_SNAP | SYS_START, child,
		 NULL, SHAREVA, SHAREVA, SHARESIZE);
}

typedef struct internal_args_t {
	int num_children;
	void * start_routine;
	void * args;
	int * status_array;
} internal_args_t;


void *
tparallel_internal(void * args_ptr)
{

	internal_args_t * args = (internal_args_t *)args_ptr;
	pthread_t threads[PROC_CHILDREN];
	int cn, ret;
	assert(args->num_children > 0);
	assert(args->num_children < PROC_CHILDREN);
	for (cn = 1; cn < PROC_CHILDREN; cn++)
		assert(files->child[cn].state == PROC_FREE);
	for (cn = 0; cn < args->num_children; cn++) {
		ret = pthread_create(&threads[cn], NULL, args->start_routine, args->args);
		if (ret != 0) {
			warn("tparallel_internal: thread creation failure");
			return (void *)EXIT_FAILURE;
		}
	}
	for (cn = 0; cn < args->num_children; cn++) {
		ret = pthread_join(threads[cn], (void **)&args->status_array[cn]);
		if (ret != 0) {
			warn("tparallel_internal: thread joining failure");
			return (void *)EXIT_FAILURE;
		}

	}
	return (void *)EXIT_SUCCESS;
}


void
tparallel_begin(int * master, int num_children, void * (* start_routine)(void *), void * args,
		int status_array[])
{

	internal_args_t internal_args;
	int ret;
	internal_args.num_children = num_children;
	internal_args.start_routine = start_routine;
	internal_args.args = args;
	internal_args.status_array = status_array;
	ret = pthread_create((pthread_t *)master, NULL, &tparallel_internal, &internal_args);
	if (ret != 0) {
		fprintf(stderr, "tparallel_begin: thread creation failure: %d\n", errno);
		exit(EXIT_FAILURE);
	}
}

void
tparallel_end(int master)
{
	int status;
	int ret = pthread_join((pthread_t)master, (void **)&status);
	if (ret != 0) {
		fprintf(stderr, "tparallel_end: thread joining failure: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	if (status != EXIT_SUCCESS) {
		fprintf(stderr, "tparallel_end: exit status %d\n", status);
		exit(EXIT_FAILURE);
	}
}

#endif // LAB >= 9
#endif // LAB >= 5
