#if LAB >= 4
#include <inc/mmu.h>
#include <inc/memlayout.h>


// Page fault upcall entrypoint.

// This is where we ask the kernel to redirect us to whenever we cause
// a page fault in user space (see the call to sys_set_pgfault_handler
// in pgfault.c).
//
// When a page fault actually occurs, the kernel switches our ESP to
// point to the user exception stack if we're not already on the user
// exception stack, and then it pushes a UTrapframe onto our user
// exception stack:
//
//	trap-time esp
//	trap-time eflags
//	trap-time eip
//	utf_regs.reg_eax
//	...
//	utf_regs.reg_esi
//	utf_regs.reg_edi
//	utf_err (error code)
//	utf_fault_va            <-- %esp
//
// If this is a recursive fault, the kernel will reserve for us a
// blank word above the trap-time esp for scratch work when we unwind
// the recursive call.
//
// We then have call up to the appropriate page fault handler in C
// code, pointed to by the global variable '_pgfault_handler' declared
// above.

.text
.globl _pgfault_upcall
_pgfault_upcall:
	// Call the C page fault handler.
	pushl %esp
	movl _pgfault_handler, %eax
	call *%eax

	// Push trap-time %eip onto the trap-time stack.
	//
	// Explanation:
	//   We must prepare the trap-time stack for our eventual return to
	//   re-execute the instruction that faulted.
	//   Unfortunately, we can't return directly from this stack
	//   (the exception stack).  Why not?
	//   We can't call 'jmp', since that requires that we load the address
	//   into a register, and all registers must have their trap-time
	//   values after the return.
	//   We can't call 'ret' from the exception stack either, since if we
	//   did, %esp would have the wrong value.
	//   So instead, we push the trap-time %eip onto the *trap-time* stack!
	//   Below we'll switch to that stack and call 'ret', which will
	//   restore %eip to its pre-fault value.
	//
	//   In the case of a recursive fault on the exception stack,
	//   note that the word we're pushing now will fit in the
	//   blank word that the kernel reserved for us.
	//
	// Hint:
	//   What registers are available for intermediate calculations?
	//
#if SOL >= 4
	movl 13*4(%esp), %eax   // trap-time esp in eax
	subl $4, %eax
	movl %eax, 13*4(%esp)	// reserve a return word

	movl 11*4(%esp), %ebx	// eip
	movl %ebx, 0(%eax)
#else
	// LAB 4: Your code here.
#endif

	// Restore the trap-time registers.
#if SOL >= 4
	addl $0x0c, %esp
	popal
#else
	// LAB 4: Your code here.
#endif

	// Restore eflags from the stack.
#if SOL >= 4
	addl $0x04, %esp
	popf
#else
	// LAB 4: Your code here.
#endif

	// Switch back to the adjusted trap-time stack.
#if SOL >= 4
	popl %esp
#else
	// LAB 4: Your code here.
#endif

	// Return to re-execute the instruction that faulted.
#if SOL >= 4
	ret
#else
	// LAB 4: Your code here.
#endif

#endif // LAB >= 4
