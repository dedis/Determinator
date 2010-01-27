#if LAB >= 1
// Processor trap handling.
// See COPYRIGHT for copyright information.

#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/init.h>
#if LAB >= 2
#include <kern/proc.h>
#include <kern/syscall.h>

#include <dev/lapic.h>
#endif


// Interrupt descriptor table.  Must be built at run time because
// shifted function addresses can't be represented in relocation records.
static struct gatedesc idt[256];

// This "pseudo-descriptor" is needed only by the LIDT instruction,
// to specify both the size and address of th IDT at once.
static struct pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static void
trap_init_idt(void)
{
	extern segdesc gdt[];
#if SOL >= 1
	extern char
		Xdivide,Xdebug,Xnmi,Xbrkpt,Xoflow,Xbound,
		Xillop,Xdevice,Xdblflt,Xtss,Xsegnp,Xstack,
		Xgpflt,Xpgflt,Xfperr,Xalign,Xmchk,Xdefault,Xsyscall;
#if SOL >= 2
	extern char
		Xirq0,Xirq1,Xirq2,Xirq3,Xirq4,Xirq5,
		Xirq6,Xirq7,Xirq8,Xirq9,Xirq10,Xirq11,
		Xirq12,Xirq13,Xirq14,Xirq15;
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
		SETGATE(idt[i], 0, CPU_GDT_KCODE, &Xdefault, 0);

	SETGATE(idt[T_DIVIDE], 0, CPU_GDT_KCODE, &Xdivide, 0);
	SETGATE(idt[T_DEBUG],  0, CPU_GDT_KCODE, &Xdebug,  0);
	SETGATE(idt[T_NMI],    0, CPU_GDT_KCODE, &Xnmi,    0);
	SETGATE(idt[T_BRKPT],  0, CPU_GDT_KCODE, &Xbrkpt,  3);
	SETGATE(idt[T_OFLOW],  0, CPU_GDT_KCODE, &Xoflow,  3);
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

#if SOL >= 2
	// Use DPL=3 here because system calls are explicitly invoked
	// by the user process (with "int $T_SYSCALL").
	SETGATE(idt[T_SYSCALL], 0, CPU_GDT_KCODE, &Xsyscall, 3);

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

	// Load the IDT into this processor's IDT register.
	asm volatile("lidt %0" : : "m" (idt_pd));

	// Check for the correct IDT and trap handler operation.
	if (cpu_onboot())
		trap_check(0);
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

void gcc_noreturn
trap(trapframe *tf)
{
	// The user-level environment may have set the DF flag,
	// and some versions of GCC rely on DF being clear.
	asm volatile("cld" ::: "cc");

	// If this trap was anticipated, just use the designated handler.
	cpu *c = cpu_cur();
	if (c->recover)
		c->recover(tf, c->recoverdata);

#if SOL >= 2
	switch (tf->tf_trapno) {
	case T_SYSCALL:
		syscall(tf);
		break;
	case T_IRQ0 + IRQ_TIMER:
		lapic_eoi();
		proc_yield(tf);
		break;
	case T_IRQ0 + IRQ_SPURIOUS:
		cprintf("cpu%d: spurious interrupt at %x:%x\n",
			c->id, tf->tf_cs, tf->tf_eip);
		lapic_eoi();
		break;
	}

#endif	// LAB >= 2
	trap_print(tf);
	panic("unhandled trap");
}


// Helper function for trap_check_recover(), below:
// handles "anticipated" traps by simply resuming at a new EIP.
static void gcc_noreturn
trap_check_recover(trapframe *tf, void *recoverdata)
{
	tf->tf_eip = (uint32_t) recoverdata;
	trap_return(tf);
}

// Check the trap handling mechanism for correct operation.
// Called twice: first in kernel mode after trap_init() and trap_setup(),
// then later from user() in kern/init.c after we're in user mode.
// We assume the "current cpu" is always the boot cpu;
// this true for the call of this function from trap_init(),
// and is called from user mode only in lab 1,
// before any other CPUs have been started.
void
trap_check(int usermode)
{
	cpu *c = &cpu_boot;
	volatile int cookie = 0xfeedface;

	// First - are we actually in the correct mode?
	uint16_t cs;
	asm volatile("movw %%cs,%0" : "=r" (cs));
	if (usermode)
		assert((cs & 3) == 3);
	else
		assert((cs & 3) == 0);

	// Recover from the traps we're going to cause.
	if (c->recover != NULL) {
		c->recover = NULL;
		panic("trap_check(): oops, did we get called recursively!?");
	}
	c->recover = trap_check_recover;

	// Compute where the trap frame will get pushed when we cause a trap.
	trapframe *tf = usermode
		? (trapframe*)(c->kstackhi - trapframe_usize)
		: (trapframe*)(read_esp() - trapframe_ksize);

	// Try a divide by zero trap.
	// Be careful when using && to take the address of a label:
	// some versions of GCC (4.4.2 at least) will incorrectly try to
	// eliminate code it thinks is _only_ reachable via such a pointer.
	c->recoverdata = &&after_div0;
	int zero = 0;
	cookie = 1 / zero;
after_div0:
	assert(tf->tf_trapno == T_DIVIDE);

	// Make sure we got our correct stack back with us.
	// The asm ensures gcc uses ebp/esp to get the cookie.
	asm volatile("" : : : "eax","ebx","ecx","edx","esi","edi");
	assert(cookie == 0xfeedface);

	// Breakpoint trap
	c->recoverdata = &&after_breakpoint;
	asm volatile("int3");
after_breakpoint:
	assert(tf->tf_trapno == T_BRKPT);

	// Overflow trap
	c->recoverdata = &&after_overflow;
	asm volatile("addl %0,%0; into" : : "r" (0x70000000));
after_overflow:
	assert(tf->tf_trapno == T_OFLOW);

	// Bounds trap
	c->recoverdata = &&after_bound;
	int bounds[2] = { 1, 3 };
	asm volatile("boundl %0,%1" : : "r" (0), "m" (bounds[0]));
after_bound:
	assert(tf->tf_trapno == T_BOUND);

	// Illegal instruction trap
	c->recoverdata = &&after_illegal;
	asm volatile("ud2");	// guaranteed to be undefined
after_illegal:
	assert(tf->tf_trapno == T_ILLOP);

	// General protection fault due to invalid segment load
	c->recoverdata = &&after_gpfault;
	asm volatile("movl %0,%%fs" : : "r" (-1));
after_gpfault:
	assert(tf->tf_trapno == T_GPFLT);

	// General protection fault due to privilege violation
	if (usermode) {
		c->recoverdata = &&after_priv;
		asm volatile("lidt %0" : : "m" (idt_pd));
	after_priv:
		assert(tf->tf_trapno == T_GPFLT);
	}

	// Make sure our stack cookie is still with us
	assert(cookie == 0xfeedface);

	c->recover = NULL;	// No more mr. nice-guy; traps are real again

	cprintf("trap_check() succeeded from %s mode!\n",
		usermode ? "user" : "kernel");
}

#endif /* LAB >= 1 */
