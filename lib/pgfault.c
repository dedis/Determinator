#if LAB >= 4
// User-level page fault handler.  We use an assembly wrapper around a C function.
// The assembly wrapper is in entry.S.

#include <inc/lib.h>

extern void (*_pgfault_handler)(u_int, u_int);
extern void _asm_pgfault_handler(void);

//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to 
// allocate an exception stack and tell the kernel to
// call _asm_pgfault_handler on it.
//
void
set_pgfault_handler(void (*fn)(u_int va, u_int err))
{
	int r;

	if (_pgfault_handler == 0) {
#if SOL >= 4
		// map exception stack
		if ((r=sys_mem_alloc(0, UXSTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
			panic("allocating exception stack: %e", r);
		// install assembly handler with operating system
		sys_set_pgfault_handler(0, (u_int)_asm_pgfault_handler, UXSTACKTOP);
#else
		// Your code here:
		// map one page of exception stack with top at UXSTACKTOP
		// register assembly handler and stack with operating system
		panic("set_pgfault_handler not implemented");
#endif
	}

	// Save handler pointer for assembly to call.
	_pgfault_handler = fn;
}

#endif
