#if LAB >= 4
#include <inc/mmu.h>
#include <inc/pmap.h>


// Page fault upcall entrypoint.
// This is where we ask the kernel
// to redirect us to whenever we cause a page fault in user space
// (see the call to sys_set_pgfault_handler in pgfault.c).
//
// When a page fault actually occurs, 
// the kernel switches our ESP to point to the user exception stack
// if we're not already on the user exception stack,
// and then it pushes the following minimal trap frame
// onto our user exception stack:
//
//	[ 5 spare words ]
//	trap-time eip
//	trap-time eflags
//	trap-time esp
//	tf_err (error code)
//	fault_va		<-- %esp
//
// We then have to save additional caller-saved registers
// and call up to the appropriate page fault handler in C code,
// pointed to by the global variable '_pgfault_handler' declared above.

.text
.globl _pgfault_upcall
_pgfault_upcall:
	// Save the caller-saved registers.
#if SOL >= 4
	movl %eax, 28(%esp)
	movl %ecx, 24(%esp)
	movl %edx, 20(%esp)
#else
	// LAB 4: Your code here.
#endif

	// Call the C page fault handler.
	movl _pgfault_handler, %eax
	call *%eax

	// Push trap-time %eip and %eflags onto the trap-time stack.
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
	//   restore %esp to its pre-fault value.
	//   We'll also restore %eflags from the trap-time stack, in case an
	//   intervening instruction changes the flags.  ('ret' does not.)
	//
	//   In the case of a recursive fault on the exception stack,
	//   note that the two words we're pushing now will overlap with
	//   the current exception frame!
	//
	// Hint:
	//   What registers are available for intermediate calculations?
	//
#if SOL >= 4
	movl 8(%esp), %eax	// trap-time esp in eax
	subl $8, %eax
	movl %eax, 8(%esp)

	movl 16(%esp), %ecx	// eip
	movl %ecx, 4(%eax)
	movl 12(%esp), %ecx	// eflags
	movl %ecx, 0(%eax)
#else
	// LAB 4: Your code here.
#endif

	// Restore the caller-saved registers.
#if SOL >= 4
	movl 20(%esp), %edx
	movl 24(%esp), %ecx
	movl 28(%esp), %eax
#else
	// LAB 4: Your code here.
#endif

	// Switch back to the adjusted trap-time stack.
#if SOL >= 4
	movl 8(%esp), %esp
#else
	// LAB 4: Your code here.
#endif

	// Restore eflags from the stack.
#if SOL >= 4
	popf
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
