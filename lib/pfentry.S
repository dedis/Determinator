#if LAB >= 4
#include <inc/mmu.h>
#include <inc/pmap.h>


// Page fault handler entrypoint -
// this is where we ask the kernel
// (by calling sys_set_pgfault_handler in pgfault.c)
// to redirect us to whenever we cause a page fault in user space.
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
//	trap-time errcode
//	trap-time va		<-- %esp
//
// We then have to save additional callee-saved registers
// and call up to the appropriate page fault handler in C code,
// pointed to by the global variable '_pgfault_handler' declared above.

.text
.globl _pgfault_entry
_pgfault_entry:
	// Save the caller-save registers
#if SOL >= 4
	movl %eax, 28(%esp)
	movl %ecx, 24(%esp)
	movl %edx, 20(%esp)
#else
	//	(your code here)
#endif

	// Call the C page fault handler
	movl _pgfault_handler, %eax
	call *%eax

	// Push trap-time eip and eflags onto trap-time stack.
	// In the case of a recursive fault on the exception stack,
	// note that the two words we're pushing now
	// overlap with the exception frame we're currently using!
#if SOL >= 4
	movl 8(%esp), %eax	// trap-time esp in eax
	subl $8, %eax
	movl %eax, 8(%esp)

	movl 16(%esp), %ecx	// eip
	movl %ecx, 4(%eax)
	movl 12(%esp), %ecx	// eflags
	movl %ecx, 0(%eax)
#else
	//	(your code here)
#endif

	// Restore the caller-save registers.
#if SOL >= 4
	movl 20(%esp), %edx
	movl 24(%esp), %ecx
	movl 28(%esp), %eax
#else
	//	(your code here)
#endif

	// Switch back to the trap-time stack.
#if SOL >= 4
	movl 8(%esp), %esp
#else
	//	(your code here)
#endif

	// Restore eflags and eip from the stack.
#if SOL >= 4
	popf
	ret
#else
	//	(your code here)
#endif

#endif // LAB >= 4
