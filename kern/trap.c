#if LAB >= 3

#include <inc/mmu.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/sched.h>
#include <kern/console.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
#if LAB >= 4
#include <kern/syscall.h>
#endif

u_int page_fault_mode = PFM_NONE;
static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { {0}, };
struct Pseudodesc idt_pd =
{
	0, sizeof(idt) - 1, (unsigned long) idt,
};


void
idt_init(void)
{
	extern struct Segdesc gdt[];
#if SOL >= 3
	extern void
		Xdivide,Xdebug,Xnmi,Xbrkpt,Xoflow,Xbound,
		Xillop,Xdevice,Xdblflt,Xtss,Xsegnp,Xstack,
		Xgpflt,Xpgflt,Xfperr,Xalign,Xmchk,Xdefault,
		Xirq0,Xirq1,Xirq2,Xirq3,Xirq4,Xirq5,
		Xirq6,Xirq7,Xirq8,Xirq9,Xirq10,Xirq11,
		Xirq12,Xirq13,Xirq14,Xirq15,Xsyscall;

	int i;

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

#if SOL >= 4
	// Use DPL=3 because system calls are explicitly invoked
	// by the user process (with "int $T_SYSCALL").
	SETGATE(idt[T_SYSCALL], 0, GD_KT, &Xsyscall, 3);
#endif
#endif /* SOL >= 3 */

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Love to put this code in the initialization of gdt, but
	// the compiler generates an error incorrectly.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (u_long) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

	// Load the IDT
	asm volatile("lidt idt_pd+2");
}


static void
print_trapframe(struct Trapframe *tf)
{
	printf("TRAP frame at %p\n", tf);
	printf("	edi  0x%x\n", tf->tf_edi);
	printf("	esi  0x%x\n", tf->tf_esi);
	printf("	ebp  0x%x\n", tf->tf_ebp);
	printf("	oesp 0x%x\n", tf->tf_oesp);
	printf("	ebx  0x%x\n", tf->tf_ebx);
	printf("	edx  0x%x\n", tf->tf_edx);
	printf("	ecx  0x%x\n", tf->tf_ecx);
	printf("	eax  0x%x\n", tf->tf_eax);
	printf("	es   0x%x\n", tf->tf_es);
	printf("	ds   0x%x\n", tf->tf_ds);
	printf("	trap 0x%x\n", tf->tf_trapno);
	printf("	err  0x%x\n", tf->tf_err);
	printf("	eip  0x%x\n", tf->tf_eip);
	printf("	cs   0x%x\n", tf->tf_cs);
	printf("	flag 0x%x\n", tf->tf_eflags);
	printf("	esp  0x%x\n", tf->tf_esp);
	printf("	ss   0x%x\n", tf->tf_ss);
}

void
trap(struct Trapframe *tf)
{
	// print_trapframe(tf);
#if LAB >= 4
	if (tf->tf_trapno == T_PGFLT) {
		page_fault_handler(tf);
		return;
	}
#endif
#if SOL >= 4
	if (tf->tf_trapno == T_SYSCALL) {
		// handle system call
		tf->tf_eax = syscall(
			tf->tf_eax,
			tf->tf_edx,
			tf->tf_ecx,
			tf->tf_ebx,
			tf->tf_edi,
			tf->tf_esi
		);
		return;
	}
#endif
	if (tf->tf_trapno == IRQ_OFFSET+0) {
		// irq 0 -- clock interrupt
#if SOL >= 3
		sched_yield();
#else
		panic("clock interrupt");
#endif
	}
#if SOL >= 6
	if (tf->tf_trapno == IRQ_OFFSET+1) {
		kbd_intr();
		return;
	}
#endif
	if (tf->tf_trapno == IRQ_OFFSET+4) {
		serial_intr();
		return;
	}
	if (IRQ_OFFSET <= tf->tf_trapno 
			&& tf->tf_trapno < IRQ_OFFSET+MAX_IRQS) {
		// just ingore spurious interrupts
		printf("spurious interrupt on irq %d\n",
			tf->tf_trapno - IRQ_OFFSET);
		print_trapframe(tf);
		return;
	}

	// the user process or the kernel has a bug.
	print_trapframe(tf);
#if SOL >= 3
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
#else
	panic("unhandled trap");
#endif
}


#if LAB >= 4
void
page_fault_handler(struct Trapframe *tf)
{
	u_int fault_va;
	u_int *tos, d;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

#if SOL >= 4
	if ((tf->tf_cs & 3) == 0) {
		switch (page_fault_mode) {
		default:
			panic("page_fault_mode %d", page_fault_mode);
		case PFM_NONE:
			print_trapframe(tf);
			panic("page fault");
	
		case PFM_KILL:
			printf("[%08x] PFM_KILL va %08x ip %08x\n", 
				curenv->env_id, fault_va, tf->tf_eip);
			env_destroy(curenv);
		}
	}

	if (curenv->env_pgfault_handler == 0) {
		printf("[%08x] user fault va %08x ip %08x eax %08x\n",
			curenv->env_id, fault_va, tf->tf_eip, tf->tf_eax);
		env_destroy(curenv);
	}

	d = curenv->env_xstacktop - tf->tf_esp;
	if (d < BY2PG)
		tos = (u_int*)tf->tf_esp;
	else
		tos = (u_int*)curenv->env_xstacktop;

	page_fault_mode = PFM_KILL;

	// first spare word special -- need to TRUP pointer to check it
	--tos;
	tos = TRUP(tos);
	*tos = 0;

	// rest of frame
	*--tos = 0;
	*--tos = 0;
	*--tos = 0;
	*--tos = 0;
	*--tos = tf->tf_eip;
	*--tos = tf->tf_eflags;
	*--tos = tf->tf_esp;
	*--tos = tf->tf_err;
	*--tos = fault_va;

	// set user registers so that env_run switches to fault handler
	tf->tf_esp = (u_int)tos;
	tf->tf_eip = curenv->env_pgfault_handler;
	page_fault_mode = PFM_NONE;

	env_run(curenv);
#else
	// Fill this in
	print_trapframe(tf);
	panic("page fault");
#endif
}
#endif /* LAB >= 4 */

#endif /* LAB >= 3 */
