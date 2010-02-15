#if LAB >= 3

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/mmu.h>
#include <inc/vm.h>


#define STACKSIZE	PAGESIZE

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

extern uint8_t start[], etext[], edata[], end[];

static uint8_t gcc_aligned(STACKSIZE) stack[2][STACKSIZE];


static void fork(int cmd, uint8_t child, void (*func)(int arg), int arg,
		uint8_t stack[STACKSIZE])
{

	// Push the function argument and fake return address on the stack
	uint32_t *esp = (uint32_t*)&stack[STACKSIZE];
	*--esp = arg;
	*--esp = (uint32_t) sys_ret;

	// Set up the register state for the child
	struct cpustate cs;
	memset(&cs, 0, sizeof(cs));
	cs.tf.tf_eip = (uint32_t) func;
	cs.tf.tf_esp = (uint32_t) esp;

	// Fork the child, copying our entire user address space into it.
	sys_put(cmd | SYS_REGS | SYS_COPY, child, &cs, ALLVA, ALLVA, ALLSIZE);
}

static int join(int cmd, uint8_t child)
{
	// Wait for the child and retrieve its CPU state
	struct cpustate cs;
	sys_get(cmd | SYS_REGS, child, &cs, NULL, NULL, 0);

	// Treat the trap number as the child's "exit code"
	return cs.tf.tf_trapno;
}

static void trapchild(int arg)
{
	int bounds[2] = { 1, 3 };
	switch (arg) {
	case T_DIVIDE:
		asm volatile("divl %0,%0" : : "r" (0));
	case T_BRKPT:
		asm volatile("int3");
	case T_OFLOW:
		asm volatile("addl %0,%0; into" : : "r" (0x70000000));
	case T_BOUND:
		asm volatile("boundl %0,%1" : : "r" (0), "m" (bounds[0]));
	case T_ILLOP:
		asm volatile("ud2");	// guaranteed to be undefined
	case T_GPFLT:
		asm volatile("lidt %0" : : "m" (arg));
	}
}

static void trapcheck(int trapno)
{
	fork(SYS_START, 0, trapchild, trapno, stack[0]);
	assert(join(0, 0) == trapno);
}

static void readchild(int arg) {
	(void)(*(volatile int*)arg);
}
#define readfaulttest(va) \
	fork(SYS_START, 0, readchild, (int)va, stack[0]); \
	assert(join(0, 0) == T_PGFLT);

static void writefaultchild(int arg) {
	*(volatile int*)arg = 0xdeadbeef;
}
#define writefaulttest(va) \
	fork(SYS_START, 0, writefaultchild, (int)va, stack[0]); \
	assert(join(0, 0) == T_PGFLT);

static void cputsfaultchild(int arg) {
	sys_cputs((char*)arg);
}
#define cputsfaulttest(va) \
	fork(SYS_START, 0, cputsfaultchild, (int)va, stack[0]); \
	assert(join(0, 0) == T_PGFLT);

static void putfaultchild(int arg) {
	sys_put(SYS_REGS, 0, (cpustate*)arg, NULL, NULL, 0);
}
#define putfaulttest(va) \
	fork(SYS_START, 0, putfaultchild, (int)va, stack[0]); \
	assert(join(0, 0) == T_PGFLT);

static void getfaultchild(int arg) {
	sys_get(SYS_REGS, 0, (cpustate*)arg, NULL, NULL, 0);
}
#define getfaulttest(va) \
	fork(SYS_START, 0, getfaultchild, (int)va, stack[0]); \
	assert(join(0, 0) == T_PGFLT);

static void loadcheck()
{
	// Simple ELF loading test: make sure bss is mapped but cleared
	uint8_t *p;
	for (p = edata; p < end; p++)
		assert(*p == 0);

	cprintf("testvm: loadcheck passed\n");
}

// Check forking of simple child processes and trap redirection (once more)
static void forkcheck()
{
	// Our first copy-on-write test: fork and execute a simple child.
	fork(SYS_START, 0, trapchild, T_SYSCALL, stack[0]);
	assert(join(0, 0) == T_SYSCALL);

	// Re-check trap handling and reflection from child processes
	trapcheck(T_DIVIDE);
	trapcheck(T_BRKPT);
	trapcheck(T_OFLOW);
	trapcheck(T_BOUND);
	trapcheck(T_ILLOP);
	trapcheck(T_GPFLT);

	// Make sure we can run several children using the same stack area
	// (since each child should get a separate logical copy)
	fork(SYS_START, 0, trapchild, T_SYSCALL, stack[0]);
	fork(SYS_START, 1, trapchild, T_DIVIDE, stack[0]);
	fork(SYS_START, 2, trapchild, T_BRKPT, stack[0]);
	fork(SYS_START, 3, trapchild, T_OFLOW, stack[0]);
	fork(SYS_START, 4, trapchild, T_BOUND, stack[0]);
	fork(SYS_START, 5, trapchild, T_ILLOP, stack[0]);
	fork(SYS_START, 6, trapchild, T_GPFLT, stack[0]);
	assert(join(0, 0) == T_SYSCALL);
	assert(join(0, 1) == T_DIVIDE);
	assert(join(0, 2) == T_BRKPT);
	assert(join(0, 3) == T_OFLOW);
	assert(join(0, 4) == T_BOUND);
	assert(join(0, 5) == T_ILLOP);
	assert(join(0, 6) == T_GPFLT);

	// Check that kernel address space is inaccessible to user code
	readfaulttest(0);
	readfaulttest(VM_USERLO-4);
	readfaulttest(VM_USERHI);
	readfaulttest(0-4);

	cprintf("testvm: forkcheck passed\n");
}

// Check for proper virtual memory protection
static void protcheck()
{
	// Copyin/copyout protection:
	// make sure we can't use cputs/put/get data in kernel space
	cputsfaulttest(0);
	cputsfaulttest(VM_USERLO-1);
	cputsfaulttest(VM_USERHI);
	cputsfaulttest(~0);
	putfaulttest(0);
	putfaulttest(VM_USERLO-1);
	putfaulttest(VM_USERHI);
	putfaulttest(~0);
	getfaulttest(0);
	getfaulttest(VM_USERLO-1);
	getfaulttest(VM_USERHI);
	getfaulttest(~0);

	// Check that unused parts of user space are also inaccessible
	readfaulttest(VM_USERLO+PTSIZE);
	readfaulttest(VM_USERHI-PTSIZE);
	readfaulttest(VM_USERHI-PTSIZE*2);
	cputsfaulttest(VM_USERLO+PTSIZE);
	cputsfaulttest(VM_USERHI-PTSIZE);
	cputsfaulttest(VM_USERHI-PTSIZE*2);
	putfaulttest(VM_USERLO+PTSIZE);
	putfaulttest(VM_USERHI-PTSIZE);
	putfaulttest(VM_USERHI-PTSIZE*2);
	getfaulttest(VM_USERLO+PTSIZE);
	getfaulttest(VM_USERHI-PTSIZE);
	getfaulttest(VM_USERHI-PTSIZE*2);

	// Check that our text segment is mapped read-only
	writefaulttest((int)start);
	writefaulttest((int)etext-4);
	getfaulttest((int)start);
	getfaulttest((int)etext-4);

	cprintf("testvm: protcheck passed\n");
}

// Test explicit memory management operations
static void memopcheck(void)
{
	// Test page permission changes
	void *va = (void*)VM_USERLO+PTSIZE+PAGESIZE;
	readfaulttest(va);
	sys_get(SYS_PERM | SYS_READ, 0, NULL, NULL, va, PAGESIZE);
	assert(*(volatile int*)va == 0);	// should be readable now
	writefaulttest(va);			// but not writable
	sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL, va, PAGESIZE);
	*(volatile int*)va = 0xdeadbeef;	// should be writable now
	sys_get(SYS_PERM, 0, NULL, NULL, va, PAGESIZE);	// revoke all perms
	readfaulttest(va);
	sys_get(SYS_PERM | SYS_READ, 0, NULL, NULL, va, PAGESIZE);
	assert(*(volatile int*)va == 0xdeadbeef);	// readable again
	writefaulttest(va);				// but not writable
	sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL, va, PAGESIZE);

	// Test SYS_ZERO with SYS_GET
	va = (void*)VM_USERLO+PTSIZE;	// 4MB-aligned
	sys_get(SYS_ZERO, 0, NULL, NULL, va, PTSIZE);
	readfaulttest(va);		// should be inaccessible again
	sys_get(SYS_PERM | SYS_READ, 0, NULL, NULL, va, PAGESIZE);
	assert(*(volatile int*)va == 0);	// and zeroed
	writefaulttest(va);			// but not writable
	sys_get(SYS_ZERO, 0, NULL, NULL, va, PTSIZE);
	readfaulttest(va);			// gone again
	sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL, va, PAGESIZE);
	*(volatile int*)va = 0xdeadbeef;	// writable now
	sys_get(SYS_ZERO, 0, NULL, NULL, va, PTSIZE);
	readfaulttest(va);			// gone again
	sys_get(SYS_PERM | SYS_READ, 0, NULL, NULL, va, PAGESIZE);
	assert(*(volatile int*)va == 0);	// and zeroed

	// Test SYS_COPY with SYS_GET - pull residual stuff out of child 0
	void *sva = (void*)VM_USERLO;
	void *dva = (void*)VM_USERLO+PTSIZE;
	sys_get(SYS_COPY, 0, NULL, sva, dva, PTSIZE);
	assert(memcmp(sva, dva, end - start) == 0);
	writefaulttest(dva);
	readfaulttest(dva + PTSIZE-4);

	// Test SYS_ZERO with SYS_PUT
	void *dva2 = (void*)VM_USERLO+PTSIZE*2;
	sys_put(SYS_ZERO, 0, NULL, NULL, dva, PTSIZE);
	sys_get(SYS_COPY, 0, NULL, dva, dva2, PTSIZE);
	readfaulttest(dva2);
	readfaulttest(dva2 + PTSIZE-4);
	sys_get(SYS_PERM | SYS_READ, 0, NULL, NULL, dva2, PTSIZE);
	assert(*(volatile int*)dva2 == 0);
	assert(*(volatile int*)(dva2+PTSIZE-4) == 0);

	// Test SYS_COPY with SYS_PUT
	sys_put(SYS_COPY, 0, NULL, sva, dva, PTSIZE);
	sys_get(SYS_COPY, 0, NULL, dva, dva2, PTSIZE);
	assert(memcmp(sva, dva2, end - start) == 0);
	writefaulttest(dva2);
	readfaulttest(dva2 + PTSIZE-4);

	// Hide an easter egg and make sure it survives the two copies
	sva = (void*)VM_USERLO; dva = sva+PTSIZE; dva2 = dva+PTSIZE;
	uint32_t ofs = PTSIZE-PAGESIZE;
	sys_get(SYS_PERM|SYS_READ|SYS_WRITE, 0, NULL, NULL, sva+ofs, PAGESIZE);
	*(volatile int*)(sva+ofs) = 0xdeadbeef;	// should be writable now
	sys_get(SYS_PERM, 0, NULL, NULL, sva+ofs, PAGESIZE);
	readfaulttest(sva+ofs);			// hide it
	sys_put(SYS_COPY, 0, NULL, sva, dva, PTSIZE);
	sys_get(SYS_COPY, 0, NULL, dva, dva2, PTSIZE);
	readfaulttest(dva2+ofs);		// stayed hidden?
	sys_get(SYS_PERM|SYS_READ, 0, NULL, NULL, dva2+ofs, PAGESIZE);
	assert(*(volatile int*)(dva2+ofs) == 0xdeadbeef);	// survived?

	cprintf("testvm: memopcheck passed\n");
}

void piosmain()
{
	cprintf("testvm: in piosmain()\n");

	loadcheck();
	forkcheck();
	protcheck();
	memopcheck();

	cprintf("testvm: completed successfully!\n");
}

#endif
