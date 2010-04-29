#if LAB >= 3

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
	testl $0x0fffffff, %esp
	jnz args_exist

	// If not, push dummy argc/argv arguments.
	// This happens when we are loaded by the kernel,
	// because the kernel does not know about passing arguments.
	pushl $0
	pushl $0

args_exist:
#endif
#if LAB >= 9		// run initialization functions
	movl	$__init_array_start,%esi
1:	cmpl	$__init_array_end,%esi
	je	2f
	movl	(%esi),%eax
	addl	$4,%esi
	call	*%eax
	jmp	1b
2:
#endif

	call	main	// run the program
	pushl	%eax	// use with main's return value as exit status
#if LAB >= 9		// run finalization functions
	movl	$__fini_array_start,%esi
1:	cmpl	$__fini_array_end,%esi
	je	2f
	movl	(%esi),%eax
	addl	$4,%esi
	call	*%eax
	jmp	1b
2:
#endif
	call	exit
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
	movl	4(%esp),%esp	// Load new executable's initial stack pointer
	xorl	%ebp,%ebp	// New stack will be at its first stack frame

	movl	$SYS_GET|SYS_COPY,%eax	// Copy child 0's memory onto our own.
	xorl	%edx,%edx		// edx[0-7] = child 0
	movl	$VM_USERLO,%esi
	movl	$VM_USERLO,%edi
	movl	$VM_USERHI-VM_USERLO,%ecx
	int	$T_SYSCALL

	movl	$SYS_PUT|SYS_ZERO,%eax	// Zero out child 0's state
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
