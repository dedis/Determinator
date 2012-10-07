/*
 * Simple setjmp and longjmp implementations
 * Bits and pieces from FreeBSD libc setjmp.S
 */
#include "asm.h"

ENTRY(setjmp)
#if 0
	movl	4(%esp),%ecx		// jmp_buf argument

	movl	(%esp),%edx		// save return pointer
	movl	%edx,(%ecx)
	movl	%ebx,4(%ecx)		// save callee-saved registers
	movl	%esi,8(%ecx)
	movl	%edi,12(%ecx)
	movl	%ebp,16(%ecx)
	movl	%esp,20(%ecx)		// save stack pointer
#endif
	xorq	%rax,%rax		// return 0
	ret
END(setjmp)

ENTRY(longjmp)
	movq	%rdi,%rdx
	/* Restore the mxcsr, but leave exception flags intact. */
	stmxcsr	-4(%rsp)
	movl	68(%rdx),%eax
	andl	$0xffffffc0,%eax
	movl	-4(%rsp),%edi
	andl	$0x3f,%edi
	xorl	%eax,%edi
	movl	%edi,-4(%rsp)
	ldmxcsr -4(%rsp)
	movq	%rsi,%rax		/* retval */
	movq	0(%rdx),%rcx
	movq	8(%rdx),%rbx
	movq	16(%rdx),%rsp
	movq	24(%rdx),%rbp
	movq	32(%rdx),%r12
	movq	40(%rdx),%r13
	movq	48(%rdx),%r14
	movq	56(%rdx),%r15
	fldcw	64(%rdx)
	testq	%rax,%rax
	jnz	1f
	incq	%rax
1:	movq	%rcx,0(%rsp)
	ret
END(longjmp)
