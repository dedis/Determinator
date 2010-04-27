//minimal mmap/sbrk

#include <inc/mman.h>

#include <inc/stdio.h>
#include <inc/mmu.h>
#include <inc/syscall.h>

static char * mem_brk;

void * sbrk(intptr_t increment) {
	extern char end[];
	char * m, * e;
	e = end;
	e = ROUNDUP(e, PAGESIZE);
	if (mem_brk < e)
		mem_brk = e;
	m = mem_brk;
	increment = ROUNDUP(increment, PAGESIZE);
	mem_brk += increment;
	// fprintf(stderr, "%p %p %d\n", m, mem_brk, increment);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, m, increment);
	return m;
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	return sbrk(length);
}

int munmap(void *addr, size_t length)
{
	cprintf("unmap addr $x, len %x\n", addr, length);
	return 0;
}
