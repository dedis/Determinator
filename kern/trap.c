#if LAB >= 3
#include <inc/mmu.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/sched.h>
#include <kern/console.h>
#include <kern/printf.h>
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
	SETGATE(idt[T_BRKPT],  0, GD_KT, &Xbrkpt,  0);
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

	// dpl=3 since it is explicitly invoked with "int $T_SYSCALL"
	// by the user process(at priv level 3)
	SETGATE(idt[T_SYSCALL], 0, GD_KT, &Xsyscall, 3);

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
	asm volatile("lidt _idt_pd+2");
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
#if 0
	if (tf->tf_trapno == 32)
		printf(".");
	else if (tf->tf_trapno == 33)
		printf("*");
#endif

#if SOL >= 3
	if (tf->tf_trapno == IRQ_OFFSET) {
		// irq 0 -- clock interrupt
		sched_yield();
	}
#if LAB >= 4
	if (tf->tf_trapno == T_PGFLT) {
		page_fault_handler(tf);
	}
	else if (tf->tf_trapno == T_SYSCALL) {
		/* tf_eax - # of system call
		 * tf_edx - 1st argument(if any)
		 * tf_ecx - 2nd argument(if any)
		 * tf_ebx - 3rd argument(if any)
		 * tf_esi - 4th argument(if any)
		 */
		tf->tf_eax = dispatch_syscall(
				tf->tf_eax, tf->tf_edx, tf->tf_ecx,
				tf->tf_ebx, tf->tf_esi);
	}
#endif /* not LAB >= 4 */
#if LAB >= 6
	else if (tf->tf_trapno == IRQ_OFFSET + 1) {
		kbd_intr();
	}
#endif
	else if (tf->tf_trapno >= IRQ_OFFSET && 
		 tf->tf_trapno < IRQ_OFFSET + MAX_IRQS) {
		// just ignore spurious interrupts
		u_int irq = tf->tf_trapno - IRQ_OFFSET;
		printf("ignoring unexpected IRQ %d:", irq);
		printf(" eip 0x%x;", tf->tf_eip);
		printf(" cs = 0x%x;", tf->tf_cs & 0xffff);
		printf(" eflags = 0x%x\n", tf->tf_eflags);
	} else {
		// the user process or the kernel has a bug..
#if LAB >= 4
		if (tf->tf_cs == GD_KT)
			panic("unhandled trap in kernel");
		else
			env_destroy(curenv);
#else
		print_trapframe(tf);
		panic("unhandled trap");
#endif
	}
#else /* not SOL >= 3 */
	print_trapframe(tf);
	panic("unhandled trap");
#endif /* not SOL >= 3 */
}


#if LAB >= 4
void
page_fault_handler(struct Trapframe *tf)
{
	u_int va = rcr2 ();
#if 0
	u_int env_id = curenv ? curenv->env_id : -1;
	printf("%%%% [0x%x] Page fault(code 0x%x) for VA 0x%x(env 0x%x)\n"
		"   eip = 0x%x, cs = 0x%x, eflags = 0x%x(pte 0x%x)\n",
		tf->tf_trapno, 0xffff & tf->tf_err, va, env_id,
		tf->tf_eip, 0xffff & tf->tf_cs, tf->tf_eflags, 
		vpd[PDX(va)] & PG_P ? vpt[PGNO(va)] : -1);

	/* Only traps from user mode push %ss:%esp */
	if (tf->tf_err & FEC_U)
		printf("   esp = 0x%x, ss = 0x%x\n", tf->tf_esp, 0xffff & tf->tf_ss);
#endif
#if SOL >= 4

	if (tf->tf_err & FEC_U) {
		if (curenv->env_id == 0)
			panic("Unexpected page fault in idle environment");

		if (curenv->env_pgfault_handler) {
			// switch to the exception stack(if we are not already on it).
#if 0
			u_int *xsp = (u_int *)(tf->tf_esp <= USTACKTOP ? UXSTACKTOP : tf->tf_esp);
#else
			//printf("%x %x\n", tf->tf_esp, curenv->env_xstacktop);

			u_int *xsp = (u_int *)curenv->env_xstacktop;
			if ((tf->tf_esp < (u_int)xsp) && (tf->tf_esp > (u_int)xsp - NBPG)) {
				xsp = (u_int *)tf->tf_esp;
				//printf("ON XSTACK [%x]: ", (u_int)xsp);
			}
#endif
			// XXX what if xsp is 0?
			//     Then trup works, but xsp -= 5 wraps around
			//     and the code overwrites kernel memory. 

			// XXX explain..
			xsp -= 5;
			if ((u_int)xsp < ULIM) { 	// XXX and check for write-ability
				// "push" the trap frame on the user exception stack
				*--xsp = tf->tf_eip;
				*--xsp = tf->tf_eflags;
				*--xsp = tf->tf_esp;
				*--xsp = tf->tf_err;
				*--xsp = va;
				tf->tf_esp = (u_int)xsp;
				// XXX if the trap handler is bogus, will the fault be charged
				// to the kernel or the user?
				tf->tf_eip = curenv->env_pgfault_handler;
				//printf("popping...eip %x\n", tf->tf_eip);
				//printf("popping...esp %x\n", tf->tf_esp);
				env_pop_tf(tf);
			}
		}
		printf("Killing faulting user environment\n");
		env_destroy(curenv);
	} else {
		panic("Unexpected kernel page fault");
	}
#else /* not SOL >= 4 */
	// Fill this in
#endif /* not SOL >= 4 */
}
#endif /* LAB >= 4 */

// 4491726


#endif /* LAB >= 3 */
