#if LAB >= 3

	.data

// Entrypoint - this is where the kernel (or our parent process)
// starts us running when we are initially loaded into a new process.
	.text
	.globl start
start:
#if LAB >= 99
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
	call	piosmain
	call	sys_ret
1:	jmp 1b

#endif // LAB >= 3
