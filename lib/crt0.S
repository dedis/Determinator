#if LAB >= 3
/*
 * Entrypoint code for PIOS user-space programs.
 * This code must always be the first object file linked into any program,
 * and must always be linked at the same memory address (see below).
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 * Adapted for 64-bit by Weiyi Wu at Yale University.
 */

#include <inc/syscall.h>
#include <inc/vm.h>


	.text

// Start entrypoint - this is where the kernel (or our parent process)
// starts us running when we are initially loaded into a new process.
	.globl start
start:
#if LAB >= 4
	// See if we were started with arguments on the stack.
	// If not, our esp will start on a nice big power-of-two boundary.
	test $0xffffffff, %esp
	jnz args_exist

	// If not, push dummy argc/argv arguments.
	// This happens when we are loaded by the kernel,
	// because the kernel does not know about passing arguments.
	pushq $0
	pushq $0

args_exist:
	// 64-bit ABI need to pass argument via registers
	movq	(%rsp),%rdi
	movq	0x08(%rsp),%rsi
#endif

/*FIXME:: Ishan - 01 Jun,2011
  -check the logic below.
 */ 
#if LAB >= 9		// run initialization functions
	movq	$__init_array_start,%rdx
1:	cmpq	$__init_array_end,%rdx
	je	2f
	movq	(%rdx),%rax
	addq	$8,%rdx
	call	*%rax //FIXME
	jmp	1b
2:
#endif

	call	main	// run the program
	movq	%rax,%rdi	// use with main's return value as exit status

#if LAB >= 9		// run finalization functions
	movq	$__fini_array_start,%rdx
1:	cmpq	$__fini_array_end,%rdx
	je	2f
	movq	(%rdx),%rax
	addq	$8,%rdx
	call	*%rax //FIXME
	jmp	1b
2:
#endif
#if LAB >= 4
	call	exit
#else
        movl	$SYS_RET, %eax
        int	$T_SYSCALL
#endif
1:	jmp 1b


#if LAB >= 4
// Start entrypoint for exec.  When our exec code replaces an existing process
// with a new one, it loads the new program image into child process 0,
// then calls this "function" with the new program's initial stack pointer
// as the only argument.
// Here we overwrite our entire user space memory state with that of child 0,
// clear child 0's address space, and start the new program.
// Since the old program's executable gets overwritten by the new one
// during the first system call below, this code will continue to work
// after that point ONLY if this particular code sequence is identical
// and at the same location in EVERY user program.
// We guarantee this by putting it in lib/entry.S, which is always the same
// and linked at the beginning of every user program.
	.globl exec_start
exec_start:
	mov	%rdi,%rsp	// Load new executable's initial stack pointer
	xor	%rbp,%rbp	// New stack will be at its first stack frame

	mov	$SYS_GET|SYS_COPY,%rax	// Copy child 0's memory onto our own.
	xor	%rdx,%rdx		// rdx[0-7] = child 0
	mov	$VM_USERLO,%rsi
	mov	$VM_USERLO,%rdi
	mov	$VM_USERHI-VM_USERLO,%rcx
	int	$T_SYSCALL

	mov	$SYS_PUT|SYS_ZERO,%rax	// Zero out child 0's state
	int	$T_SYSCALL

	jmp	start

#endif	// LAB >= 4

#if LAB >= 9
// Thread-local storage definitions.
	.section ".tdata"
	.globl	tdata_start
tdata_start:
	.section ".tbss"
	.globl	tbss_start
tbss_start:
#endif	// LAB >= 9
#endif	// LAB >= 3
