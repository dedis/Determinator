#if LAB >= 4
// User-level page fault handler support.
// We use an assembly language wrapper around a C function.
// The assembly wrapper is in pfentry.S.

#include <inc/lib.h>


// Assembly language pgfault entrypoint defined in lib/pgfaultentry.S.
extern void _pgfault_entry(void);

// Pointer to currently installed C-language pgfault handler.
void (*_pgfault_handler)(u_int, u_int);

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
		if ((r=sys_page_alloc(0, UXSTACKTOP-PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
			panic("allocating exception stack: %e", r);

		// register assembly pgfault entrypoint with JOS kernel
		sys_env_set_pgfault_upcall(0, (u_int)_pgfault_entry);
#else
		// Your code here:
		// map one page of exception stack with top at UXSTACKTOP
		// register assembly pgfault entrypoint with JOS kernel
		panic("set_pgfault_handler not implemented");
#endif
	}

	// Save handler pointer for assembly to call.
	_pgfault_handler = fn;
}

#endif	// LAB >= 4
