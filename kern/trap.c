///LAB3
#include <inc/mmu.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>

u_int page_fault_mode = PFM_NONE;
static struct Ts ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct gate_desc idt[256] = { {0}, };
struct pseudo_desc idt_pd =
{
  0, sizeof (idt) - 1, (unsigned long) idt,
};


void
idt_init ()
{
  extern struct seg_desc gdt[];
///LAB4
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
    setgate (idt[i], 0, GD_KT, &Xdefault, 0);

  setgate (idt[T_DIVIDE], 0, GD_KT, &Xdivide, 0);
  setgate (idt[T_DEBUG],  0, GD_KT, &Xdebug,  0);
  setgate (idt[T_NMI],    0, GD_KT, &Xnmi,    0);
  setgate (idt[T_BRKPT],  0, GD_KT, &Xbrkpt,  0);
  setgate (idt[T_OFLOW],  0, GD_KT, &Xoflow,  0);
  setgate (idt[T_BOUND],  0, GD_KT, &Xbound,  0);
  setgate (idt[T_ILLOP],  0, GD_KT, &Xillop,  0);
  setgate (idt[T_DEVICE], 0, GD_KT, &Xdevice, 0);
  setgate (idt[T_DBLFLT], 0, GD_KT, &Xdblflt, 0);
  setgate (idt[T_TSS],    0, GD_KT, &Xtss,    0);
  setgate (idt[T_SEGNP],  0, GD_KT, &Xsegnp,  0);
  setgate (idt[T_STACK],  0, GD_KT, &Xstack,  0);
  setgate (idt[T_GPFLT],  0, GD_KT, &Xgpflt,  0);
  setgate (idt[T_PGFLT],  0, GD_KT, &Xpgflt,  0);
  setgate (idt[T_FPERR],  0, GD_KT, &Xfperr,  0);
  setgate (idt[T_ALIGN],  0, GD_KT, &Xalign,  0);
  setgate (idt[T_MCHK],   0, GD_KT, &Xmchk,   0);

  setgate (idt[IRQ_OFFSET + 0], 0, GD_KT, &Xirq0, 0);
  setgate (idt[IRQ_OFFSET + 1], 0, GD_KT, &Xirq1, 0);
  setgate (idt[IRQ_OFFSET + 2], 0, GD_KT, &Xirq2, 0);
  setgate (idt[IRQ_OFFSET + 3], 0, GD_KT, &Xirq3, 0);
  setgate (idt[IRQ_OFFSET + 4], 0, GD_KT, &Xirq4, 0);
  setgate (idt[IRQ_OFFSET + 5], 0, GD_KT, &Xirq5, 0);
  setgate (idt[IRQ_OFFSET + 6], 0, GD_KT, &Xirq6, 0);
  setgate (idt[IRQ_OFFSET + 7], 0, GD_KT, &Xirq7, 0);
  setgate (idt[IRQ_OFFSET + 8], 0, GD_KT, &Xirq8, 0);
  setgate (idt[IRQ_OFFSET + 9], 0, GD_KT, &Xirq9, 0);
  setgate (idt[IRQ_OFFSET + 10], 0, GD_KT, &Xirq10, 0);
  setgate (idt[IRQ_OFFSET + 11], 0, GD_KT, &Xirq11, 0);
  setgate (idt[IRQ_OFFSET + 12], 0, GD_KT, &Xirq12, 0);
  setgate (idt[IRQ_OFFSET + 13], 0, GD_KT, &Xirq13, 0);
  setgate (idt[IRQ_OFFSET + 14], 0, GD_KT, &Xirq14, 0);
  setgate (idt[IRQ_OFFSET + 15], 0, GD_KT, &Xirq15, 0);

  // dpl=3 since it is explicitly invoked with "int $T_SYSCALL"
  // by the user process (at priv level 3)
  setgate (idt[T_SYSCALL], 0, GD_KT, &Xsyscall, 3);

///END

  // Setup a TSS so that we get the right stack when we trap to the kernel.
  ts.ts_esp0 = KSTACKTOP;
  ts.ts_ss0 = GD_KD;
///LAB200	
  ts.ts_cr3 = p0cr3_boot; // XXX delete these?
  ts.ts_iomb = 0xdfff;
///END	

  // Love to put this code in the initialization of gdt, but
  // the compiler generates an error incorrectly.
  gdt[GD_TSS >> 3] = seg (STS_T32A, (u_long) (&ts), sizeof (struct Ts), 0);
  gdt[GD_TSS >> 3].sd_s = 0;

  ltr (GD_TSS);
  asm volatile ("lidt _idt_pd+2");
}


static void
print_trapframe (struct Trapframe *tf)
{
  printf ("%%%% Trap 0x%x (errcode 0x%x)\n", tf->tf_trapno, 0xffff & tf->tf_err);
  printf ("   cs:eip 0x%x:0x%x; ", 0xffff & tf->tf_cs, tf->tf_eip);
  printf ("ss:esp 0x%x:0x%x; ", 0xffff & tf->tf_ss, tf->tf_esp);
  printf ("ebp 0x%x\n", tf->tf_ebp);
  printf ("   eax 0x%x; ", tf->tf_eax);
  printf ("ebx 0x%x; ", tf->tf_ebx);
  printf ("ecx 0x%x; ", tf->tf_ecx);
  printf ("edx 0x%x;\n", tf->tf_edx);
  printf ("   eflags 0x%x; ", tf->tf_eflags);
  printf ("edi 0x%x; ", tf->tf_edi);
  printf ("esi 0x%x; ", tf->tf_esi);
  printf ("ds 0x%x; ", 0xffff & tf->tf_ds);
  printf ("es 0x%x;\n", 0xffff & tf->tf_es);
}


void
trap (struct Trapframe *tf)
{
#if 0
  if (tf->tf_trapno == 32)
    printf (".");
  else if (tf->tf_trapno == 33)
    printf ("*");
#endif

///LAB4
#if 0
///END
  print_trapframe (tf);
///LAB4
#endif
///END

  if (tf->tf_trapno == T_PGFLT) {
    page_fault_handler (tf);
  }
  else if (tf->tf_trapno == T_SYSCALL) {
///LAB5
    /* tf_eax - # of system call
     * tf_edx - 1st argument (if any)
     * tf_ecx - 2nd argument (if any)
     * tf_ebx - 3rd argument (if any)
     * tf_esi - 4th argument (if any)
     */
    tf->tf_eax = dispatch_syscall (tf->tf_eax, tf->tf_edx, tf->tf_ecx, 
				   tf->tf_ebx, tf->tf_esi);
///END
  }
  else if (tf->tf_trapno == IRQ_OFFSET) {
    // irq 0 -- clock interrupt
///LAB4
    yield ();
///END
  }
///LAB6
  else if (tf->tf_trapno == IRQ_OFFSET + 1) {
    kbd_intr ();
  }
///END
  else if (tf->tf_trapno >= IRQ_OFFSET && 
	   tf->tf_trapno < IRQ_OFFSET + MAX_IRQS) {
    // just ignore spurious interrupts
    u_int irq = tf->tf_trapno - IRQ_OFFSET;
    printf ("ignoring unexpected IRQ %d:", irq);
    printf (" eip 0x%x;", tf->tf_eip);
    printf (" cs = 0x%x;", tf->tf_cs & 0xffff);
    printf (" eflags = 0x%x\n", tf->tf_eflags);
  } else {
    // the user process or the kernel has a bug..
    print_trapframe (tf);
    panic ("That does it (Unhandled trap).");
///LAB4
    if (tf->tf_cs == GD_KT)
      panic ("That does it (Unhandled trap in kernel).");
    else
      env_destroy (curenv);
///END
  }

///LAB4
#if 0
///END
  panic ("in lab3 don't continue..");
///LAB4
#endif
///END


}


void
page_fault_handler (struct Trapframe *tf)
{

  u_int va = rcr2 ();
#if 0
  u_int env_id = curenv ? curenv->env_id : -1;
  printf ("%%%% [0x%x] Page fault (code 0x%x) for VA 0x%x (env 0x%x)\n"
	  "   eip = 0x%x, cs = 0x%x, eflags = 0x%x (pte 0x%x)\n",
	  tf->tf_trapno, 0xffff & tf->tf_err, va, env_id,
	  tf->tf_eip, 0xffff & tf->tf_cs, tf->tf_eflags, 
	  vpd[PDENO(va)] & PG_P ? vpt[PGNO(va)] : -1);

  /* Only traps from user mode push %ss:%esp */
  if (tf->tf_err & FEC_U)
    printf ("   esp = 0x%x, ss = 0x%x\n", tf->tf_esp, 0xffff & tf->tf_ss);
#endif

///LAB200
  if (tf->tf_err & FEC_U) {
    if (curenv->env_id == 0)
      panic ("Unexpected page fault in idle environment");

    if (curenv->env_pgfault_handler) {
      // switch to the exception stack (if we are not already on it).
#if 0
      u_int *xsp = (u_int *)(tf->tf_esp <= USTACKTOP ? UXSTACKTOP : tf->tf_esp);
#else
      //printf ("%x %x\n", tf->tf_esp, curenv->env_xstacktop);

      u_int *xsp = (u_int *)curenv->env_xstacktop;
      if ((tf->tf_esp < (u_int)xsp) && (tf->tf_esp > (u_int)xsp - NBPG)) {
      	xsp = (u_int *)tf->tf_esp;
	//printf ("ON XSTACK [%x]: ", (u_int)xsp);
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
	//printf ("popping...eip %x\n", tf->tf_eip);
	//printf ("popping...esp %x\n", tf->tf_esp);
	env_pop_tf (tf);
      }
    }
    printf ("Killing faulting user environment\n");
    env_destroy (curenv);
  } else {
    panic ("Unexpected kernel page fault");
  }
///END
}

// 4491726


///END 3
