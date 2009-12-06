#if LAB >= 3
#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
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

static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
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
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
#endif
	return "(unknown trap)";
}


void
idt_init(void)
{
	extern struct Segdesc gdt[];
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
	static_assert(sizeof(struct Trapframe) == SIZEOF_STRUCT_TRAPFRAME);
#if SOL >= 4
	// check that IRQ_OFFSET is a multiple of 8
	static_assert((IRQ_OFFSET & 7) == 0);
#endif

	// install a default handler
	for (i = 0; i < sizeof(idt)/sizeof(idt[0]); i++)
		SETGATE(idt[i], 0, GD_KT, &Xdefault, 0);

	SETGATE(idt[T_DIVIDE], 0, GD_KT, &Xdivide, 0);
	SETGATE(idt[T_DEBUG],  0, GD_KT, &Xdebug,  0);
	SETGATE(idt[T_NMI],    0, GD_KT, &Xnmi,    0);
	SETGATE(idt[T_BRKPT],  0, GD_KT, &Xbrkpt,  3);
	SETGATE(idt[T_OFLOW],  0, GD_KT, &Xoflow,  0);
	SETGATE(idt[T_BOUND],  0, GD_KT, &Xbound,  0);
	SETGATE(idt[T_ILLOP],  0, GD_KT, &Xillop,  0);
	SETGATE(idt[T_DEVICE], 0, GD_KT, &Xdevice, 0);
	SETGATE(idt[T_DBLFLT], 0, GD_KT, &Xdblflt, 0);
	SETGATE(idt[T_TSS],    0, GD_KT, &Xtss,    0);
	SETGATE(idt[T_SEGNP],  0, GD_KT, &Xsegnp,  0);
	SETGATE(idt[T_STACK],  0, GD_KT, &Xstack,  0);
	SETGATE(idt[T_GPFLT],  0, GD_KT, &Xgpflt,  0);
	SETGATE(idt[T_PGFLT],  0, GD_KT, &Xpgflt,  0);
	SETGATE(idt[T_FPERR],  0, GD_KT, &Xfperr,  0);
	SETGATE(idt[T_ALIGN],  0, GD_KT, &Xalign,  0);
	SETGATE(idt[T_MCHK],   0, GD_KT, &Xmchk,   0);

#if SOL >= 4
	SETGATE(idt[IRQ_OFFSET + 0], 0, GD_KT, &Xirq0, 0);
	SETGATE(idt[IRQ_OFFSET + 1], 0, GD_KT, &Xirq1, 0);
	SETGATE(idt[IRQ_OFFSET + 2], 0, GD_KT, &Xirq2, 0);
	SETGATE(idt[IRQ_OFFSET + 3], 0, GD_KT, &Xirq3, 0);
	SETGATE(idt[IRQ_OFFSET + 4], 0, GD_KT, &Xirq4, 0);
	SETGATE(idt[IRQ_OFFSET + 5], 0, GD_KT, &Xirq5, 0);
	SETGATE(idt[IRQ_OFFSET + 6], 0, GD_KT, &Xirq6, 0);
	SETGATE(idt[IRQ_OFFSET + 7], 0, GD_KT, &Xirq7, 0);
	SETGATE(idt[IRQ_OFFSET + 8], 0, GD_KT, &Xirq8, 0);
	SETGATE(idt[IRQ_OFFSET + 9], 0, GD_KT, &Xirq9, 0);
	SETGATE(idt[IRQ_OFFSET + 10], 0, GD_KT, &Xirq10, 0);
	SETGATE(idt[IRQ_OFFSET + 11], 0, GD_KT, &Xirq11, 0);
	SETGATE(idt[IRQ_OFFSET + 12], 0, GD_KT, &Xirq12, 0);
	SETGATE(idt[IRQ_OFFSET + 13], 0, GD_KT, &Xirq13, 0);
	SETGATE(idt[IRQ_OFFSET + 14], 0, GD_KT, &Xirq14, 0);
	SETGATE(idt[IRQ_OFFSET + 15], 0, GD_KT, &Xirq15, 0);
#endif	// SOL >= 4

	// Use DPL=3 here because system calls are explicitly invoked
	// by the user process (with "int $T_SYSCALL").
	SETGATE(idt[T_SYSCALL], 0, GD_KT, &Xsyscall, 3);
#else	// not SOL >= 3
	
	// LAB 3: Your code here.
#endif	// SOL >= 3

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

	// Load the IDT
	asm volatile("lidt idt_pd");
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
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
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
		// irq 0 -- clock interrupt
#if SOL >= 6
		time_tick();
#endif
		sched_yield();
	}
#if SOL >= 7
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_KBD) {
		kbd_intr();
		return;
	}
#endif	// SOL >= 7
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SERIAL) {
		serial_intr();
		return;
	}
#if SOL >= 6
	if (tf->tf_trapno == IRQ_OFFSET + e100_irq) {
		e100_intr();
		irq_eoi();
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
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
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
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

#if LAB == 3
	cprintf("Incoming TRAP frame at %p\n", tf);

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
	// Return to the current environment, which should be runnable.
	assert(curenv && curenv->env_status == ENV_RUNNABLE);
	env_run(curenv);
#endif
}


void
page_fault_handler(struct Trapframe *tf)
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
	if (tf->tf_esp >= UXSTACKTOP - PGSIZE && tf->tf_esp < UXSTACKTOP) {
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

#endif /* LAB >= 3 */
