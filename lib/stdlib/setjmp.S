
	.text
	.p2align 4
	.globl	setjmp
setjmp:
	movl	4(%esp),%ecx		// jmp_buf argument

	movl	(%esp),%edx		// save return pointer
	movl	%edx,(%ecx)
	movl	%ebx,4(%ecx)		// save callee-saved registers
	movl	%esi,8(%ecx)
	movl	%edi,12(%ecx)
	movl	%ebp,16(%ecx)
	movl	%esp,20(%ecx)		// save stack pointer

	xorl	%eax,%eax		// return 0
	ret

	.p2align 4
	.globl	longjmp
longjmp:
	movl	4(%esp),%ecx		// jmp_buf argument
	movl	8(%esp),%eax		// return value argument

	movl	(%ecx),%edx		// get return address
	movl	%edx,(%esp)
	movl	4(%ecx),%ebx		// restore callee-saved registers
	movl	8(%ecx),%esi
	movl	12(%ecx),%edi
	movl	16(%ecx),%ebp
	movl	20(%ecx),%esp		// restore stack pointer

	testl	%eax,%eax		// always return nonzero
	jnz	1f
	incl	%eax
1:	ret

