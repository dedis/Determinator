#if LAB >= 1
/*
 * Processor trap handling.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>
#include <inc/trap.h>
#if LAB >= 9
#include <inc/fenv.h>
#endif

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/cons.h>
#include <kern/init.h>
#if LAB >= 2
#include <kern/proc.h>
#include <kern/syscall.h>
#if LAB >= 3
#include <kern/pmap.h>
#endif
#if LAB >= 5
#include <kern/net.h>
#endif

#if LAB >= 9
#include <dev/timer.h>
#include <dev/pmc.h>
#endif
#include <dev/lapic.h>
#if LAB >= 4
#include <dev/kbd.h>
#include <dev/serial.h>
#endif
#if LAB >= 5
#include <dev/e100.h>
#endif
#endif // LAB >= 2


// Interrupt descriptor table.  Must be built at run time because
// shifted function addresses can't be represented in relocation records.
static struct gatedesc idt[256] gcc_aligned(16);

// This "pseudo-descriptor" is needed only by the LIDT instruction,
// to specify both the size and address of th IDT at once.
static struct pseudodesc idt_pd = {
	sizeof(idt) - 1, (uintptr_t) idt
};

static void
trap_init_idt(void)
{
	extern segdesc gdt[];
#if SOL >= 1
	extern char
		Xdivide,Xdebug,Xnmi,Xbrkpt,Xoflow,Xbound,
		Xillop,Xdevice,Xdblflt,Xtss,Xsegnp,Xstack,
		Xgpflt,Xpgflt,Xfperr,Xalign,Xmchk,Xsimd,Xdefault;
#if SOL >= 2
	extern char
		Xirq0,Xirq1,Xirq2,Xirq3,Xirq4,Xirq5,
		Xirq6,Xirq7,Xirq8,Xirq9,Xirq10,Xirq11,
		Xirq12,Xirq13,Xirq14,Xirq15,
		Xsyscall,Xltimer,Xlerror,Xperfctr;
#endif	// SOL >= 2
	int i;

	// check that the SIZEOF_STRUCT_TRAPFRAME symbol is defined correctly
	static_assert(sizeof(trapframe) == SIZEOF_STRUCT_TRAPFRAME);
#if SOL >= 2
	// check that T_IRQ0 is a multiple of 8
	static_assert((T_IRQ0 & 7) == 0);
#endif

	// install a default handler
	for (i = 0; i < sizeof(idt)/sizeof(idt[0]); i++)
		SETGATE(idt[i], 0, SEG_KERN_CS_64, &Xdefault, 0,0);

	SETGATE(idt[T_DIVIDE], 0, SEG_KERN_CS_64, &Xdivide, 0,0);
	SETGATE(idt[T_DEBUG],  0, SEG_KERN_CS_64, &Xdebug,  0,0);
	SETGATE(idt[T_NMI],    0, SEG_KERN_CS_64, &Xnmi,    0,0);
	SETGATE(idt[T_BRKPT],  0, SEG_KERN_CS_64, &Xbrkpt,  3,0);
	SETGATE(idt[T_OFLOW],  0, SEG_KERN_CS_64, &Xoflow,  3,0);
	SETGATE(idt[T_BOUND],  0, SEG_KERN_CS_64, &Xbound,  0,0);
	SETGATE(idt[T_ILLOP],  0, SEG_KERN_CS_64, &Xillop,  0,0);
	SETGATE(idt[T_DEVICE], 0, SEG_KERN_CS_64, &Xdevice, 0,0);
	SETGATE(idt[T_DBLFLT], 0, SEG_KERN_CS_64, &Xdblflt, 0,1);
	SETGATE(idt[T_TSS],    0, SEG_KERN_CS_64, &Xtss,    0,0);
	SETGATE(idt[T_SEGNP],  0, SEG_KERN_CS_64, &Xsegnp,  0,0);
	SETGATE(idt[T_STACK],  0, SEG_KERN_CS_64, &Xstack,  0,0);
	SETGATE(idt[T_GPFLT],  0, SEG_KERN_CS_64, &Xgpflt,  0,0);
	SETGATE(idt[T_PGFLT],  0, SEG_KERN_CS_64, &Xpgflt,  0,0);
	SETGATE(idt[T_FPERR],  0, SEG_KERN_CS_64, &Xfperr,  0,0);
	SETGATE(idt[T_ALIGN],  0, SEG_KERN_CS_64, &Xalign,  0,0);
	SETGATE(idt[T_MCHK],   0, SEG_KERN_CS_64, &Xmchk,   0,1);
	SETGATE(idt[T_SIMD],   0, SEG_KERN_CS_64, &Xsimd,   0,0);
	// WWY
	SETGATE(idt[T_BRKPT], 0, SEG_KERN_CS_64, &Xbrkpt, 3,0);

#if SOL >= 2
	SETGATE(idt[T_IRQ0 + 0], 0, SEG_KERN_CS_64, &Xirq0, 0,0);
	SETGATE(idt[T_IRQ0 + 1], 0, SEG_KERN_CS_64, &Xirq1, 0,0);
	SETGATE(idt[T_IRQ0 + 2], 0, SEG_KERN_CS_64, &Xirq2, 0,0);
	SETGATE(idt[T_IRQ0 + 3], 0, SEG_KERN_CS_64, &Xirq3, 0,0);
	SETGATE(idt[T_IRQ0 + 4], 0, SEG_KERN_CS_64, &Xirq4, 0,0);
	SETGATE(idt[T_IRQ0 + 5], 0, SEG_KERN_CS_64, &Xirq5, 0,0);
	SETGATE(idt[T_IRQ0 + 6], 0, SEG_KERN_CS_64, &Xirq6, 0,0);
	SETGATE(idt[T_IRQ0 + 7], 0, SEG_KERN_CS_64, &Xirq7, 0,0);
	SETGATE(idt[T_IRQ0 + 8], 0, SEG_KERN_CS_64, &Xirq8, 0,0);
	SETGATE(idt[T_IRQ0 + 9], 0, SEG_KERN_CS_64, &Xirq9, 0,0);
	SETGATE(idt[T_IRQ0 + 10], 0, SEG_KERN_CS_64, &Xirq10, 0,0);
	SETGATE(idt[T_IRQ0 + 11], 0, SEG_KERN_CS_64, &Xirq11, 0,0);
	SETGATE(idt[T_IRQ0 + 12], 0, SEG_KERN_CS_64, &Xirq12, 0,0);
	SETGATE(idt[T_IRQ0 + 13], 0, SEG_KERN_CS_64, &Xirq13, 0,0);
	SETGATE(idt[T_IRQ0 + 14], 0, SEG_KERN_CS_64, &Xirq14, 0,0);
	SETGATE(idt[T_IRQ0 + 15], 0, SEG_KERN_CS_64, &Xirq15, 0,0);

	// Use DPL=3 here because system calls are explicitly invoked
	// by the user process (with "int $T_SYSCALL").
	SETGATE(idt[T_SYSCALL], 0, SEG_KERN_CS_64, &Xsyscall, 3,0);

	// Vectors we use for local APIC interrupts
	SETGATE(idt[T_LTIMER], 0, SEG_KERN_CS_64, &Xltimer, 0,0);
	SETGATE(idt[T_LERROR], 0, SEG_KERN_CS_64, &Xlerror, 0,0);
#if LAB >= 9
	SETGATE(idt[T_PERFCTR], 0, SEG_KERN_CS_64, &Xperfctr, 0,0);
#endif

#endif	// SOL >= 2
#else	// not SOL >= 1

	panic("trap_init() not implemented.");
#endif	// SOL >= 1
}

void
trap_init(void)
{
	// The first time we get called on the bootstrap processor,
	// initialize the IDT.  Other CPUs will share the same IDT.
	if (cpu_onboot())
		trap_init_idt();
	init_cprintf("idt init\n");

	// Load the IDT into this processor's IDT register.
	asm volatile("lidt %0" : : "m" (idt_pd));
	init_cprintf("idt reg init\n");

	// Check for the correct IDT and trap handler operation.
	if (cpu_onboot())
		trap_check_kernel();
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
#if LAB >= 2
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= T_IRQ0 && trapno < T_IRQ0 + 16)
		return "Hardware Interrupt";
#endif
	return "(unknown trap)";
}

void
trap_print_regs(trapframe *tf)
{
	cprintf("  r15  0x%016llx\n", tf->r15);
	cprintf("  r14  0x%016llx\n", tf->r14);
	cprintf("  r13  0x%016llx\n", tf->r13);
	cprintf("  r12  0x%016llx\n", tf->r12);
	cprintf("  r11  0x%016llx\n", tf->r11);
	cprintf("  r10  0x%016llx\n", tf->r10);
	cprintf("  r9   0x%016llx\n", tf->r9);
	cprintf("  r8   0x%016llx\n", tf->r8);
	cprintf("  rbp  0x%016llx\n", tf->rbp);
	cprintf("  rdi  0x%016llx\n", tf->rdi);
	cprintf("  rsi  0x%016llx\n", tf->rsi);
	cprintf("  rdx  0x%016llx\n", tf->rdx);
	cprintf("  rcx  0x%016llx\n", tf->rcx);
	cprintf("  rbx  0x%016llx\n", tf->rbx);
	cprintf("  rax  0x%016llx\n", tf->rax);
}

void
trap_print(trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	trap_print_regs(tf);
#if LAB >= 9
	cprintf("  gs   0x------------%04x\n", tf->gs);
	cprintf("  fs   0x------------%04x\n", tf->fs);
#endif
	cprintf("  es   0x------------%04x\n", tf->es);
	cprintf("  ds   0x------------%04x\n", tf->ds);
	cprintf("  trap 0x%016llx %s\n", tf->trapno, trap_name(tf->trapno));
	cprintf("  err  0x--------%08lx\n", tf->err);
	cprintf("  rip  0x%016llx\n", tf->rip);
	cprintf("  cs   0x------------%04x\n", tf->cs);
	cprintf("  flag 0x%016llx\n", tf->rflags);
	cprintf("  rsp  0x%016llx\n", tf->rsp);
	cprintf("  ss   0x------------%04x\n", tf->ss);
}

void gcc_noreturn
trap(trapframe *tf)
{
	// The user-level environment may have set the DF flag,
	// and some versions of GCC rely on DF being clear.
	asm volatile("cld" ::: "cc");
//	cprintf("trap no is %x\n", tf->trapno);

#if SOL >= 3
	// If this is a page fault, first handle lazy copying automatically.
	// If that works, this call just calls trap_return() itself -
	// otherwise, it returns normally to blame the fault on the user.
	if (tf->trapno == T_PGFLT)
		pmap_pagefault(tf);

#endif
	// If this trap was anticipated, just use the designated handler.
	cpu *c = cpu_cur();
	if (c->recover)
		c->recover(tf, c->recoverdata);

#if LAB >= 2
#if SOL >= 2
	proc *p = proc_cur();
	switch (tf->trapno) {
	case T_SYSCALL:
		assert(tf->cs & 3);	// syscalls only come from user space
		syscall(tf);
		break;

	case T_BRKPT:	// other traps entered via explicit INT instructions
	case T_OFLOW:
		assert(tf->cs & 3);	// only allowed from user space
		proc_ret(tf, 1);	// reflect trap to parent process

#if SOL >= 2
	case T_DEVICE:	// attempted to access FPU while TS flag set
		//cprintf("trap: enabling FPU\n");
		p->sv.pff |= PFF_USEFPU;
		assert(sizeof(p->sv.fx) == 512);
		lcr0(rcr0() & ~CR0_TS);			// enable FPU
		asm volatile("fxrstor %0" : : "m" (p->sv.fx));
		trap_return(tf);

#endif // SOL >= 2
	case T_LTIMER: ;
#if SOL >= 5
		net_tick();
#endif
		lapic_eoi();
#if LAB >= 9	// Determinator
		uint64_t t = timer_read(); // update PIT count high bits
		//cprintf("LTIMER on %d: %lld\n", c->id, (long long)t);
#if LAB >= 99
		{	static uint64_t lastt;
			static int cnt;
			if (cpu_onboot())
				cnt++;
			if (cpu_onboot() && t > lastt) {
				lastt += TIMER_FREQ;
				cprintf("tick - after %d\n", cnt);
				cnt = 0;
			}
		}
#endif
#else		// PIOS
		if (tf->cs & 3)	// If in user mode, context switch
			proc_yield(tf);
#endif
		trap_return(tf);	// Otherwise, stay in idle loop
	case T_LERROR:
		lapic_errintr();
		trap_return(tf);
#if SOL >= 4
	case T_IRQ0 + IRQ_KBD:
		//cprintf("CPU%d: KBD\n", c->id);
		kbd_intr();
		lapic_eoi();
		trap_return(tf);
	case T_IRQ0 + IRQ_SERIAL:
		//cprintf("CPU%d: SER\n", c->id);
		lapic_eoi();
		serial_intr();
		trap_return(tf);
#endif // SOL >= 4
#if LAB >= 99
	case T_IRQ0 + IRQ_IDE:
		ide_intr();
		lapic_eoi();
		trap_return(tf);
#endif
	case T_IRQ0 + IRQ_SPURIOUS:
		cprintf("cpu%d: spurious interrupt at %x:%x\n",
			c->id, tf->cs, tf->rip);
		trap_return(tf); // Note: no EOI (see Local APIC manual)
		break;
#if LAB >= 9
	case T_DEBUG:	// count instructions by single-stepping
		// XXX user code can set the trace flag itself;
		// need to manage a virtual trace flag on behalf of it
		// instead of just panicking if we see a debug trap
		// that we didn't cause.
		assert(tf->cs & 3);
		assert(tf->rflags & FL_TF);
		assert(p->sv.pff & PFF_ICNT);
		assert(!pmc_get || (p->sv.imax - p->sv.icnt) <= pmc_safety);
		//cprintf("T_DEBUG eip %x\n", tf->eip);
		if (++p->sv.icnt < p->sv.imax)
			trap_return(tf);	// keep stepping
		tf->trapno = T_ICNT;
		proc_ret(tf, -1);	// can't run any more insns!

	case T_PERFCTR:	// count insns via performance monitoring counters
		lapic_eoi();	// first acknowledge the PMC interrupt
		// Since performance counter interrupts are asynchronous,
		// one can arrive after the process that triggered it
		// has already taken a trap for some other reason and
		// the CPU has switched to another process or the idle loop.
		// If we're in the idle loop when this happens, cs == 0.
		if (!(tf->cs & 3) || (p->pmcmax == 0)) {
			warn("spurious performance counter interrupt\n");
			trap_return(tf);
		}
		assert(p->sv.pff & PFF_ICNT);
		assert(pmc_get != NULL);
		int32_t ninsn = pmc_get(p->pmcmax);
		int32_t overshoot = ninsn - p->pmcmax;
		if (overshoot > pmc_overshoot)
			pmc_overshoot = overshoot;
		//cprintf("T_PERFCTR: after %d tgt %d ovr %d max %d\n",
		//	ninsn, p->pmcmax, overshoot, pmc_overshoot);
		p->sv.icnt += ninsn;
		p->pmcmax = 0;
		if (p->sv.icnt > p->sv.imax)
			panic("oops, perf ctr overshoot by %d insns\n",
				p->sv.icnt - p->sv.imax);
		if (p->sv.icnt < p->sv.imax) {
			tf->rflags |= FL_TF;	// single-step the rest
			trap_return(tf);
		}
		tf->trapno = T_ICNT;
		proc_ret(tf, -1);	// can't run any more insns!

	case T_SIMD:
		__stmxcsr(&tf->err);		// use MXCSR as error code
		break;				// defer to user code
#endif	// LAB >= 9
	}
#if SOL >= 5
	if (tf->trapno == T_IRQ0 + e100_irq) {
		e100_intr();
		lapic_eoi();
		trap_return(tf);
	}
#endif // ! SOL >= 5
	if (tf->cs & 3) {		// Unhandled trap from user mode
		cprintf("trap in proc %x, reflecting to proc %x\n",
			proc_cur(), proc_cur()->parent);
		trap_print(tf);
		proc_ret(tf, -1);	// Reflect trap to parent process
	}
#else	// SOL < 2
	// Lab 2: your trap handling code here!
#endif	// SOL < 2

	// If we panic while holding the console lock,
	// release it so we don't get into a recursive panic that way.
	if (spinlock_holding(&cons_lock))
		spinlock_release(&cons_lock);
#endif	// LAB >= 2
	trap_print(tf);
	panic("unhandled trap");
}


// Helper function for trap_check_recover(), below:
// handles "anticipated" traps by simply resuming at a new RIP.
static void gcc_noreturn
trap_check_recover(trapframe *tf, void *recoverdata)
{
	trap_check_args *args = recoverdata;
	tf->rip = (uint64_t) args->rrip;	// Use recovery RIP on return
	args->trapno = tf->trapno;		// Return trap number
	trap_return(tf);
}

// Check for correct handling of traps from kernel mode.
// Called on the boot CPU after trap_init() and trap_setup().
void
trap_check_kernel(void)
{
	assert((read_cs() & 3) == 0);	// better be in kernel mode!

	cpu *c = cpu_cur();
	c->recover = trap_check_recover;
	trap_check(&c->recoverdata);
	c->recover = NULL;	// No more mr. nice-guy; traps are real again

#if LAB >= 9
#else
	cprintf("trap_check_kernel() succeeded!\n");
#endif
}

// Check for correct handling of traps from user mode.
// Called from user() in kern/init.c, only in lab 1.
// We assume the "current cpu" is always the boot cpu;
// this true only because lab 1 doesn't start any other CPUs.
void
trap_check_user(void)
{
	assert((read_cs() & 3) == 3);	// better be in user mode!

	cpu *c = &cpu_boot;	// cpu_cur doesn't work from user mode!
	c->recover = trap_check_recover;
	trap_check(&c->recoverdata);
	c->recover = NULL;	// No more mr. nice-guy; traps are real again

#if LAB >= 9
#else
	cprintf("trap_check_user() succeeded!\n");
#endif
}

void after_div0();
void after_breakpoint();
void after_overflow();
void after_bound();
void after_illegal();
void after_gpfault();
void after_priv();

// Multi-purpose trap checking function.
void
trap_check(void **argsp)
{
	volatile int cookie = 0xfeedface;
	volatile trap_check_args args;
	*argsp = (void*)&args;	// provide args needed for trap recovery

	// Try a divide by zero trap.
	// Be careful when using && to take the address of a label:
	// some versions of GCC (4.4.2 at least) will incorrectly try to
	// eliminate code it thinks is _only_ reachable via such a pointer.
	args.rrip = after_div0;
	asm volatile("div %0,%0; after_div0:" : : "r" (0));
	assert(args.trapno == T_DIVIDE);
	// Make sure we got our correct stack back with us.
	// The asm ensures gcc uses ebp/esp to get the cookie.
	asm volatile("" : : : "rax","rbx","rcx","rdx","rsi","rdi");
	assert(cookie == 0xfeedface);

	// Breakpoint trap
	args.rrip = after_breakpoint;
	asm volatile("int3; after_breakpoint:");
	assert(args.trapno == T_BRKPT);

	// Overflow trap
	args.rrip = after_overflow;

	/*
	// RAJAT's solution. not being used due to a strange "symbol 'after_overflow' already defined" error
	uint64_t rflag_temp = 0;
	asm volatile("addl %1,%1; pushfq; popq %%rax;" : "=a" (rflag_temp): "r" (0x70000000));
	rflag_temp &= FL_OF;
	if (rflag_temp) {
		asm volatile("int %0; after_overflow:"::"i" (T_OFLOW));
	} else {
		asm volatile("after_overflow:");
	}
	*/

	asm volatile("addq %0,%0; jo overflow; jno after_overflow; overflow: int %1; after_overflow:" : :
			"r" (0x7000000000000000),"i" (T_OFLOW));
	assert(args.trapno == T_OFLOW);

	/*
	// Bounds trap
	args.rrip = after_bound;
	int bounds[2] = { 1, 3 };
	asm volatile("boundl %0,%1; after_bound:" : : "r" (0), "m" (bounds[0]));
	assert(args.trapno == T_BOUND);
	*/

	// Illegal instruction trap
	args.rrip = after_illegal;
	asm volatile("ud2; after_illegal:");	// guaranteed to be undefined
	assert(args.trapno == T_ILLOP);

	// General protection fault due to invalid segment load
	args.rrip = after_gpfault;
	asm volatile("movl %0,%%fs; after_gpfault:" : : "r" (-1));
	assert(args.trapno == T_GPFLT);

	// General protection fault due to privilege violation
	if (read_cs() & 3) {
		args.rrip = after_priv;
		asm volatile("lidt %0; after_priv:" : : "m" (idt_pd));
		assert(args.trapno == T_GPFLT);
	}

	// Make sure our stack cookie is still with us
	assert(cookie == 0xfeedface);

	*argsp = NULL;	// recovery mechanism not needed anymore
}

#endif /* LAB >= 1 */
