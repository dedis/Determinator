#if LAB >= 9
#if 0	// XXX need to decide on how to use the right malloc.  -baf
#include <inc/malloc.h>
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

void * malloc(size_t size) {
	return sbrk(size);
}


// Not yet supported.
void free(void * ptr) {

}
#endif
#endif	// LAB >= 9
