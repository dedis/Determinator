#if LAB >= 1
/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/trap.h>

#include <kern/cpu.h>
#if LAB >= 4
#include <kern/picirq.h>
#endif


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

#if SOL >= 1

/* CPU traps */
TRAPHANDLER_NOEC(Xdivide, T_DIVIDE)
TRAPHANDLER_NOEC(Xdebug,  T_DEBUG)
TRAPHANDLER_NOEC(Xnmi,    T_NMI)
TRAPHANDLER_NOEC(Xbrkpt,  T_BRKPT)
TRAPHANDLER_NOEC(Xoflow,  T_OFLOW)
TRAPHANDLER_NOEC(Xbound,  T_BOUND)
TRAPHANDLER_NOEC(Xillop,  T_ILLOP)
TRAPHANDLER_NOEC(Xdevice, T_DEVICE)
TRAPHANDLER     (Xdblflt, T_DBLFLT)
TRAPHANDLER     (Xtss,    T_TSS)
TRAPHANDLER     (Xsegnp,  T_SEGNP)
TRAPHANDLER     (Xstack,  T_STACK)
TRAPHANDLER     (Xgpflt,  T_GPFLT)
TRAPHANDLER     (Xpgflt,  T_PGFLT)
TRAPHANDLER_NOEC(Xfperr,  T_FPERR)
TRAPHANDLER     (Xalign,  T_ALIGN)
TRAPHANDLER_NOEC(Xmchk,   T_MCHK)

#if SOL >= 2
/* system call entry point */
TRAPHANDLER_NOEC(Xsyscall, T_SYSCALL)

#if SOL >= 4
/* the external device interrupts */
TRAPHANDLER_NOEC(Xirq0,   T_IRQ0+0)
TRAPHANDLER_NOEC(Xirq1,   T_IRQ0+1)
TRAPHANDLER_NOEC(Xirq2,   T_IRQ0+2)
TRAPHANDLER_NOEC(Xirq3,   T_IRQ0+3)
TRAPHANDLER_NOEC(Xirq4,   T_IRQ0+4)
TRAPHANDLER_NOEC(Xirq5,   T_IRQ0+5)
TRAPHANDLER_NOEC(Xirq6,   T_IRQ0+6)
TRAPHANDLER_NOEC(Xirq7,   T_IRQ0+7)
TRAPHANDLER_NOEC(Xirq8,   T_IRQ0+8)
TRAPHANDLER_NOEC(Xirq9,   T_IRQ0+9)
TRAPHANDLER_NOEC(Xirq10,  T_IRQ0+10)
TRAPHANDLER_NOEC(Xirq11,  T_IRQ0+11)
TRAPHANDLER_NOEC(Xirq12,  T_IRQ0+12)
TRAPHANDLER_NOEC(Xirq13,  T_IRQ0+13)
TRAPHANDLER_NOEC(Xirq14,  T_IRQ0+14)
TRAPHANDLER_NOEC(Xirq15,  T_IRQ0+15)

#endif	// SOL >=4
#endif	// SOL >= 2
/* default handler -- not for any specific trap */
TRAPHANDLER     (Xdefault, T_DEFAULT)


#else /* SOL >= 1 */
/*
 * Lab 1: Your code here for generating entry points for the different traps.
 */
#endif /* SOL >= 1 */

#if SOL >= 1
.globl	_alltraps
.type	_alltraps,@function
.p2align 4, 0x90		/* 16-byte alignment, nop filled */
_alltraps:
	pushl %ds		# build trap frame
	pushl %es
	pushal

	movl $CPU_GDT_KDATA,%eax # load kernel's data segment
	movw %ax,%ds
	movw %ax,%es

	pushl %esp		# pass pointer to this trapframe 
	call trap		# and call trap (does not return)

spin:	jmp spin		# should never get here...
#else /* SOL >= 1 */	
/*
 * Lab 1: Your code here for _alltraps
 */
	
#endif // SOL >= 1
#endif // LAB >= 1
