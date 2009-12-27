#if LAB >= 2
/*
 * Low-level thread stack initialization and context switching.
 * This module serves as an assembly language helper to thread.c.
 * See COPYRIGHT for copyright information.
 */


	.text

//
// thread_setup_stack(thread, start_func, start_data)
//
// Set up the new thread's stack so that the first time it gets run,
// it will call thread_start() below (on the thread's new stack),
// with three arguments: the thread being switched from on that CPU,
// and the start_func and start_data arguments passed by the caller,
// representing whatever higher-level task the thread should perfom.
//
	.globl	thread_stack_init
	.type thread_stack_init,@function
	.align 2
thread_stack_init:

	// Get our arguments
	movl	4(%esp),%eax		// thread being initialized
	movl	8(%esp),%ecx		// start_func
	movl	12(%esp),%edx		// start_data

	// First "push" onto the new thread's stack
	// a fictional function call stack frame for the start_func:
	// namely the argument to the start_func
	// and a fake (NULL) return pointer to its (nonexistent) caller.
	movl	%edx,4096-4(%eax)	// Argument to start_func
	movl	$0,4096-8(%eax)		// Fake start_func return pointer.

	// Now "push" the start_func pointer itself,
	// as if it was the return address from a call
	// from the start_func to thread_switch() below:
	// that way thread_switch() will "return" to the start_func
	// the first time we switch to the new thread.
	movl	%ecx,4096-12(%eax)	// start_func is fake return addr

	ret

//
// thread_stack_switch(thread *old_thread, thread *new_thread);
//
// Switch from old_thread to new_thread,
// by saving old_thread's callee-saved registers including stack pointer,
// and loading those of the new thread.
// Just before resuming the new thread,
// call spinlock_release() on the old thread's lock,
// thereby allowing the same or other CPUs to run the old thread
// only once we're completely done with it and off its stack.
//
	.globl	thread_stack_switch
	.type thread_stack_switch,@function
	.align 2
thread_stack_switch:

	// Read our C function arguments off the stack
	movl	4(%esp),%eax		// old thread
	movl	8(%esp),%ecx		// new thread

	// Save the old thread's callee-saved registers, including stack.
	movl	%ebx,THREAD_EBX(%eax)
	movl	%esi,THREAD_ESI(%eax)
	movl	%edi,THREAD_EDI(%eax)
	movl	%ebp,THREAD_EBP(%eax)
	movl	%esp,THREAD_ESP(%eax)	// Old thread's stack pointer

	// Load the new thread's callee-saved registers, including stack.
	movl	THREAD_EBX(%ecx),%ebx
	movl	THREAD_ESI(%ecx),%esi
	movl	THREAD_EDI(%ecx),%edi
	movl	THREAD_EBP(%ecx),%ebp
	movl	THREAD_ESP(%ecx),%esp	// Switch to new thread's stack!

	// Now we need to unlock the old thread,
	// which we couldn't do until we were totally off its stack.
	// (As soon as we unlock it, another CPU might start running it).
	lea	THREAD_LOCK(%eax),%edx	// argument to spinlock_release()
	movl	%edx,4(%esp)		// place in our first argument position

	// Now we just tail-call spinlock_release():
	// as soon as it finishes, it'll return directly to our caller,
	// rather than to thread_switch() itself.
	jmp	spinlock_release


#endif // LAB >= 2
