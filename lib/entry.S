#if LAB >= 3
#include <inc/mmu.h>
#include <inc/pmap.h>

.data
#if LAB >= 5
	// define page-aligned fsipcbuf for fsipc.c
	// ... and fdtab for file.c
	.p2align PGSHIFT
	.globl fsipcbuf
fsipcbuf:
	.space BY2PG
	.globl fdtab
fdtab:
	.space BY2PG
#endif


// Define the global symbols 'envs', 'pages', 'vpt', and 'vpd'
// so that they can be used in C as if they were ordinary global arrays.
	.globl envs
	.set envs, UENVS
	.globl pages
	.set pages, UPAGES
	.globl vpt
	.set vpt, UVPT
	.globl vpd
	.set vpd, (UVPT+(UVPT>>12)*4)


// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start
_start:
	// push empty argc, argv if none present
	cmpl $USTACKTOP, %esp
	jne args_exist
	pushl $0
	pushl $0

	// If no args, kernel set us up -- clear the bss
	// since the kernel did not.  This depends on the
	// bss not extending into a new page, since otherwise
	// we will page fault and die.
	movl $(end), %eax
	subl $(edata), %eax
	pushl %eax
	pushl $0
	pushl $(edata)
	call memset
	addl $12, %esp

args_exist:
	call libmain
1:      jmp 1b


.data
.globl _pgfault_handler
_pgfault_handler:
	.long 0


// Page fault handler entrypoint -
// this is where we ask the kernel
// (by calling sys_set_pgfault_handler)
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
.globl _asm_pgfault_handler
_asm_pgfault_handler:
	// save the caller-save registers
#if SOL >= 3
	movl %eax, 28(%esp)
	movl %ecx, 24(%esp)
	movl %edx, 20(%esp)
#else
	//	(your code here)
#endif

	// call the C page fault handler
	movl _pgfault_handler, %eax
	call *%eax

	// push trap-time eip, eflags onto trap-time stack
#if SOL >= 3
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
#if SOL >= 3
	movl 20(%esp), %edx
	movl 24(%esp), %ecx
	movl 28(%esp), %eax
#else
	//	(your code here)
#endif

	// switch to the trap-time stack 
#if SOL >= 3
	movl 8(%esp), %esp
#else
	//	(your code here)
#endif

	// restore eflags and eip from the stack
#if SOL >= 3
	popf
	ret
#else
	//	(your code here)
#endif
#endif // LAB >= 3
