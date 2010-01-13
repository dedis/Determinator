#if LAB >= 1
// Processor trap handling.
// See COPYRIGHT for copyright information.

#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#if LAB >= 3
#include <kern/env.h>
#include <kern/syscall.h>
#endif
#if LAB >= 4
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#endif
#if LAB >= 6
#include <kern/time.h>
#include <kern/e100.h>
#endif


// Interrupt descriptor table.  Must be built at run time because
// shifted function addresses can't be represented in relocation records.
static struct gatedesc idt[256];

// This "pseudo-descriptor" is needed only by the LIDT instruction,
// to specify both the size and address of th IDT at once.
static struct pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


void
trap_init(void)
{
	extern segdesc gdt[];
#if SOL >= 3
	extern char
		Xdivide,Xdebug,Xnmi,Xbrkpt,Xoflow,Xbound,
		Xillop,Xdevice,Xdblflt,Xtss,Xsegnp,Xstack,
		Xgpflt,Xpgflt,Xfperr,Xalign,Xmchk,Xdefault,Xsyscall;
#if SOL >= 4
	extern char
		Xirq0,Xirq1,Xirq2,Xirq3,Xirq4,Xirq5,
		Xirq6,Xirq7,Xirq8,Xirq9,Xirq10,Xirq11,
		Xirq12,Xirq13,Xirq14,Xirq15;
#endif
	int i;

	// check that the SIZEOF_STRUCT_TRAPFRAME symbol is defined correctly
	static_assert(sizeof(trapframe) == SIZEOF_STRUCT_TRAPFRAME);
#if SOL >= 4
	// check that T_IRQ0 is a multiple of 8
	static_assert((T_IRQ0 & 7) == 0);
#endif

	// install a default handler
	for (i = 0; i < sizeof(idt)/sizeof(idt[0]); i++)
		SETGATE(idt[i], 0, CPU_GDT_KCODE, &Xdefault, 0);

	SETGATE(idt[T_DIVIDE], 0, CPU_GDT_KCODE, &Xdivide, 0);
	SETGATE(idt[T_DEBUG],  0, CPU_GDT_KCODE, &Xdebug,  0);
	SETGATE(idt[T_NMI],    0, CPU_GDT_KCODE, &Xnmi,    0);
	SETGATE(idt[T_BRKPT],  0, CPU_GDT_KCODE, &Xbrkpt,  3);
	SETGATE(idt[T_OFLOW],  0, CPU_GDT_KCODE, &Xoflow,  0);
	SETGATE(idt[T_BOUND],  0, CPU_GDT_KCODE, &Xbound,  0);
	SETGATE(idt[T_ILLOP],  0, CPU_GDT_KCODE, &Xillop,  0);
	SETGATE(idt[T_DEVICE], 0, CPU_GDT_KCODE, &Xdevice, 0);
	SETGATE(idt[T_DBLFLT], 0, CPU_GDT_KCODE, &Xdblflt, 0);
	SETGATE(idt[T_TSS],    0, CPU_GDT_KCODE, &Xtss,    0);
	SETGATE(idt[T_SEGNP],  0, CPU_GDT_KCODE, &Xsegnp,  0);
	SETGATE(idt[T_STACK],  0, CPU_GDT_KCODE, &Xstack,  0);
	SETGATE(idt[T_GPFLT],  0, CPU_GDT_KCODE, &Xgpflt,  0);
	SETGATE(idt[T_PGFLT],  0, CPU_GDT_KCODE, &Xpgflt,  0);
	SETGATE(idt[T_FPERR],  0, CPU_GDT_KCODE, &Xfperr,  0);
	SETGATE(idt[T_ALIGN],  0, CPU_GDT_KCODE, &Xalign,  0);
	SETGATE(idt[T_MCHK],   0, CPU_GDT_KCODE, &Xmchk,   0);

#if SOL >= 4
	SETGATE(idt[T_IRQ0 + 0], 0, CPU_GDT_KCODE, &Xirq0, 0);
	SETGATE(idt[T_IRQ0 + 1], 0, CPU_GDT_KCODE, &Xirq1, 0);
	SETGATE(idt[T_IRQ0 + 2], 0, CPU_GDT_KCODE, &Xirq2, 0);
	SETGATE(idt[T_IRQ0 + 3], 0, CPU_GDT_KCODE, &Xirq3, 0);
	SETGATE(idt[T_IRQ0 + 4], 0, CPU_GDT_KCODE, &Xirq4, 0);
	SETGATE(idt[T_IRQ0 + 5], 0, CPU_GDT_KCODE, &Xirq5, 0);
	SETGATE(idt[T_IRQ0 + 6], 0, CPU_GDT_KCODE, &Xirq6, 0);
	SETGATE(idt[T_IRQ0 + 7], 0, CPU_GDT_KCODE, &Xirq7, 0);
	SETGATE(idt[T_IRQ0 + 8], 0, CPU_GDT_KCODE, &Xirq8, 0);
	SETGATE(idt[T_IRQ0 + 9], 0, CPU_GDT_KCODE, &Xirq9, 0);
	SETGATE(idt[T_IRQ0 + 10], 0, CPU_GDT_KCODE, &Xirq10, 0);
	SETGATE(idt[T_IRQ0 + 11], 0, CPU_GDT_KCODE, &Xirq11, 0);
	SETGATE(idt[T_IRQ0 + 12], 0, CPU_GDT_KCODE, &Xirq12, 0);
	SETGATE(idt[T_IRQ0 + 13], 0, CPU_GDT_KCODE, &Xirq13, 0);
	SETGATE(idt[T_IRQ0 + 14], 0, CPU_GDT_KCODE, &Xirq14, 0);
	SETGATE(idt[T_IRQ0 + 15], 0, CPU_GDT_KCODE, &Xirq15, 0);
#endif	// SOL >= 4

	// Use DPL=3 here because system calls are explicitly invoked
	// by the user process (with "int $T_SYSCALL").
	SETGATE(idt[T_SYSCALL], 0, CPU_GDT_KCODE, &Xsyscall, 3);
#else	// not SOL >= 3
	
	// LAB 3: Your code here.
#endif	// SOL >= 3
}

void
trap_startup(void)
{
	// Load the IDT into this processor's IDT register.
	asm volatile("lidt %0" : : "m" (idt_pd));
}

const char *trap_name(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
#if LAB >= 4
	if (trapno >= T_IRQ0 && trapno < T_IRQ0 + 16)
		return "Hardware Interrupt";
#endif
	return "(unknown trap)";
}

void
trap_print_regs(pushregs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
//	cprintf("  oesp 0x%08x\n", regs->reg_oesp);	don't print - useless
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

void
trap_print(trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	trap_print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trap_name(tf->tf_trapno));
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

static void
trap_dispatch(trapframe *tf)
{
	// Handle processor traps.
#if SOL >= 3
	if (tf->tf_trapno == T_PGFLT) {
		page_fault_handler(tf);
		return;
	}
	if (tf->tf_trapno == T_SYSCALL) {
		// handle system call
		tf->tf_regs.reg_eax =
			syscall(tf->tf_regs.reg_eax,
				tf->tf_regs.reg_edx,
				tf->tf_regs.reg_ecx,
				tf->tf_regs.reg_ebx,
				tf->tf_regs.reg_edi,
				tf->tf_regs.reg_esi);
		return;
	}
	if (tf->tf_trapno == T_BRKPT) {
		// Invoke the kernel monitor.
		monitor(tf);
		return;
	}
#else	// SOL >= 3
	// LAB 3: Your code here.
#endif	// SOL >= 3

#if LAB >= 4
#if SOL >= 4
	// New in Lab 4: Handle external interrupts
	if (tf->tf_trapno == T_IRQ0 + IRQ_TIMER) {
		// irq 0 -- clock interrupt
#if SOL >= 6
		time_tick();
#endif
		sched_yield();
	}
#if SOL >= 7
	if (tf->tf_trapno == T_IRQ0 + IRQ_KBD) {
		kbd_intr();
		return;
	}
#endif	// SOL >= 7
	if (tf->tf_trapno == T_IRQ0 + IRQ_SERIAL) {
		serial_intr();
		return;
	}
#if SOL >= 6
	if (tf->tf_trapno == T_IRQ0 + e100_irq) {
		e100_intr();
		pic_eoi();
		return;
	}
#endif  // SOL >= 6
#else	// SOL >= 4
	// Handle clock interrupts.
	// LAB 4: Your code here.

#if LAB >= 6
	// Add time tick increment to clock interrupts.
	// LAB 6: Your code here.

#endif
	// Handle spurious interupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == T_IRQ0 + IRQ_SPURIOUS) {
		cprintf("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}

#if LAB >= 7

	// Handle keyboard interrupts.
	// LAB 7: Your code here.
#endif	// LAB >= 7
#endif	// SOL >= 4
#endif	// LAB >= 4

	// Unexpected trap: The user process or the kernel has a bug.
	trap_print(tf);
	if (tf->tf_cs == CPU_GDT_KCODE)
		panic("unhandled trap in kernel");
	else {
		panic("XXX handle general user traps correctly");
		return;
	}
}

void
trap(trapframe *tf)
{
	// The user-level environment may have set the DF flag,
	// and some versions of GCC rely on DF being clear.
	asm volatile("cld" ::: "cc");

#if LAB == 3
	cprintf("Incoming trap frame at %p\n", tf);

#endif
#if LAB >= 3
	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		assert(curenv);
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}
	
#endif
	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

#if LAB >= 4
	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNABLE)
		env_run(curenv);
	else
		sched_yield();
#else
	// Return to the interrupted process.
	trap_return(tf);
#endif
}


//
// Restore the CPU state from a given trapframe struct
// and return from the trap using the processor's 'iret' instruction.
// This function does not return to the caller,
// since the new CPU state this function loads
// replaces the caller's stack pointer and other registers.
//
void
trap_return(trapframe *tf)
{
	// Check to make sure the ring 0 stack hasn't overflowed
	// onto the current cpu struct.
	assert(cpu_cur()->magic == CPU_MAGIC);

	__asm __volatile(
		"movl %0,%%esp;"
		"popal;"
		"popl %%es;"
		"popl %%ds;"
		"addl $0x8,%%esp;" /* skip tf_trapno and tf_errcode */
		"iret"
		: : "g" (tf) : "memory");

	panic("iret failed");  /* mostly to placate the compiler */
}


#if LAB >= 3
void
page_fault_handler(trapframe *tf)
{
	uint32_t fault_va;
#if SOL >= 4
	struct UTrapframe *utf;
#endif

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

#if SOL >= 3
	if ((tf->tf_cs & 3) == 0) {
		print_trapframe(tf);
		panic("page fault");
	}
#else
	// Handle kernel-mode page faults.
	
	// LAB 3: Your code here.
#endif	// SOL >= 3

#if SOL >= 4
	// See if the environment has installed a user page fault handler.
	if (curenv->env_pgfault_upcall == 0) {
		cprintf("[%08x] user fault va %08x ip %08x\n",
			curenv->env_id, fault_va, tf->tf_eip);
		print_trapframe(tf);
		env_destroy(curenv);
	}

	// Decide where to push our exception stack frame.
	if (tf->tf_esp >= UXSTACKTOP - PAGESIZE && tf->tf_esp < UXSTACKTOP) {
		// The user's ESP is ALREADY in the user exception stack area,
		// so push the new frame on the exception stack,
		// preserving the existing exception stack contents.
		utf = (struct UTrapframe*)(tf->tf_esp
					   - sizeof(struct UTrapframe)
					   // Save a spare word for return
					   - 4);
	} else {
		// The user's ESP is NOT in the user exception stack area,
		// so it's presumably pointing to a normal user stack
		// and the user exception stack is not in use.
		// Therefore, switch the user's ESP onto the exception stack
		// and push the new frame at the top of the exception stack.
		utf = (struct UTrapframe*)(UXSTACKTOP
					   - sizeof(struct UTrapframe));
	}

	// If we can't write to the exception stack,
	// it means the user environment is seriously screwed up,
	// so just terminate it.
	user_mem_assert(curenv, utf, sizeof(struct UTrapframe), PTE_U | PTE_W);

	// fill utf
	utf->utf_fault_va = fault_va;
	utf->utf_err = tf->tf_err;
	utf->utf_regs = tf->tf_regs;
	utf->utf_eip = tf->tf_eip;
	utf->utf_eflags = tf->tf_eflags;
	utf->utf_esp = tf->tf_esp;
 
 	// set user registers so that env_run switches to fault handler
	tf->tf_esp = (uintptr_t) utf;
 	tf->tf_eip = (uintptr_t) curenv->env_pgfault_upcall;

	env_run(curenv);
#else	// not SOL >= 4
	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

#if LAB >= 4
	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// The trap handler needs one word of scratch space at the top of the
	// trap-time stack in order to return.  In the non-recursive case, we
	// don't have to worry about this because the top of the regular user
	// stack is free.  In the recursive case, this means we have to leave
	// an extra word between the current top of the exception stack and
	// the new stack frame because the exception stack _is_ the trap-time
	// stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.

#endif
	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
#endif	// not SOL >= 4
}
#endif	// LAB >= 3

#endif /* LAB >= 1 */
