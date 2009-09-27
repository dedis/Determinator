#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

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

#if SOL >= 3

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

#if SOL >= 4
/* the external device interrupts */
TRAPHANDLER_NOEC(Xirq0,   IRQ_OFFSET+0)
TRAPHANDLER_NOEC(Xirq1,   IRQ_OFFSET+1)
TRAPHANDLER_NOEC(Xirq2,   IRQ_OFFSET+2)
TRAPHANDLER_NOEC(Xirq3,   IRQ_OFFSET+3)
TRAPHANDLER_NOEC(Xirq4,   IRQ_OFFSET+4)
TRAPHANDLER_NOEC(Xirq5,   IRQ_OFFSET+5)
TRAPHANDLER_NOEC(Xirq6,   IRQ_OFFSET+6)
TRAPHANDLER_NOEC(Xirq7,   IRQ_OFFSET+7)
TRAPHANDLER_NOEC(Xirq8,   IRQ_OFFSET+8)
TRAPHANDLER_NOEC(Xirq9,   IRQ_OFFSET+9)
TRAPHANDLER_NOEC(Xirq10,  IRQ_OFFSET+10)
TRAPHANDLER_NOEC(Xirq11,  IRQ_OFFSET+11)
TRAPHANDLER_NOEC(Xirq12,  IRQ_OFFSET+12)
TRAPHANDLER_NOEC(Xirq13,  IRQ_OFFSET+13)
TRAPHANDLER_NOEC(Xirq14,  IRQ_OFFSET+14)
TRAPHANDLER_NOEC(Xirq15,  IRQ_OFFSET+15)

#endif /* SOL >=4 */
/* system call entry point */
TRAPHANDLER_NOEC(Xsyscall, T_SYSCALL)

/* default handler -- not for any specific trap */
TRAPHANDLER     (Xdefault, T_DEFAULT)


#else /* SOL >= 3 */
/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */


#endif /* SOL >= 3 */

#if SOL >= 3	
.globl	_alltraps
.type	_alltraps,@function
.p2align 4, 0x90		/* 16-byte alignment, nop filled */
_alltraps:
	pushl %ds		# build trap frame
	pushl %es
	pushal
	movl $GD_KD,%eax	# load kernel segments
	movw %ax,%ds
	movw %ax,%es
	pushl %esp		# pass pointer to this trapframe 
	call trap		# and call trap (does not return)
spin:	jmp spin
#else /* SOL >= 3 */	
/*
 * Lab 3: Your code here for _alltraps
 */
	
#endif // SOL >= 3
#endif // LAB >= 3
