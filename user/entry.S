#if LAB >= 4
#include <inc/mmu.h>

.data
#if LAB >= 5
	// define page-aligned fsipcbuf for fsipc.c
	// ... and fdtab for file.c
	.p2align PGSHIFT
	.globl _fsipcbuf
_fsipcbuf:
	.space BY2PG
	.globl _fdtab
_fdtab:
	.space BY2PG
#endif
	.globl _envs
	.set _envs, UENVS
	.globl _pages
	.set _pages, UPAGES
	.globl _vpt
	.set _vpt, UVPT
	.globl _vpd
	.set _vpd, (UVPT+(UVPT>>12)*4)

.text
.globl .start
.start:
	// push empty argc, argv if none present
	cmpl $USTACKTOP, %esp
	jne args_exist
	pushl $0
	pushl $0

	// If no args, kernel set us up -- clear the bss
	// since the kernel did not.  This depends on the
	// bss not extending into a new page, since otherwise
	// we will page fault and die.
	movl $(_end), %eax
	subl $(_edata), %eax
	pushl %eax
	pushl $0
	pushl $(_edata)
	call _memset
	addl $12, %esp

args_exist:
	call _libmain
1:      jmp 1b


.data
.globl __pgfault_handler
__pgfault_handler:
	.long 0

// On entry there is a trap frame on the stack:
//	[ 5 spare words ]
//	trap-time eip
//	trap-time eflags
//	trap-time esp
//	trap-time errcode
//	trap-time va		<-- %esp

.text
.globl __asm_pgfault_handler
__asm_pgfault_handler:
	// save the caller-save registers
#if SOL >= 4
	movl %eax, 28(%esp)
	movl %ecx, 24(%esp)
	movl %edx, 20(%esp)
#else
	//	(your code here)
#endif

	// call the C page fault handler
	movl __pgfault_handler, %eax
	call *%eax

	// push trap-time eip, eflags onto trap-time stack
#if SOL >= 4
	movl 8(%esp), %eax
	subl $8, %eax
	movl %eax, 8(%esp)

	movl 16(%esp), %ecx	// eip
	movl %ecx, 4(%eax)
	movl 12(%esp), %ecx	// eflags
	movl %ecx, 0(%eax)
#else
	//	(your code here)
#endif

	// restore the caller-save registers
#if SOL >= 4
	movl 20(%esp), %edx
	movl 24(%esp), %ecx
	movl 28(%esp), %eax
#else
	//	(your code here)
#endif

	// switch to the trap-time stack 
#if SOL >= 4
	movl 8(%esp), %esp
#else
	//	(your code here)
#endif

	// restore eflags and eip from the stack
#if SOL >= 4
	popf
	ret
#else
	//	(your code here)
#endif
#endif // LAB >= 4
