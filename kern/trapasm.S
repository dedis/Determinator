#if LAB >= 1
/* Trap handler entry and exit code. */
/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/trap.h>

#include <kern/cpu.h>



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

/* ISA device interrupts */
TRAPHANDLER_NOEC(Xirq0,   T_IRQ0+0)	// IRQ_PIT
TRAPHANDLER_NOEC(Xirq1,   T_IRQ0+1)	// IRQ_KBD
TRAPHANDLER_NOEC(Xirq2,   T_IRQ0+2)
TRAPHANDLER_NOEC(Xirq3,   T_IRQ0+3)
TRAPHANDLER_NOEC(Xirq4,   T_IRQ0+4)	// IRQ_SERIAL
TRAPHANDLER_NOEC(Xirq5,   T_IRQ0+5)
TRAPHANDLER_NOEC(Xirq6,   T_IRQ0+6)
TRAPHANDLER_NOEC(Xirq7,   T_IRQ0+7)	// IRQ_SPURIOUS
TRAPHANDLER_NOEC(Xirq8,   T_IRQ0+8)
TRAPHANDLER_NOEC(Xirq9,   T_IRQ0+9)
TRAPHANDLER_NOEC(Xirq10,  T_IRQ0+10)
TRAPHANDLER_NOEC(Xirq11,  T_IRQ0+11)
TRAPHANDLER_NOEC(Xirq12,  T_IRQ0+12)
TRAPHANDLER_NOEC(Xirq13,  T_IRQ0+13)
TRAPHANDLER_NOEC(Xirq14,  T_IRQ0+14)	// IRQ_IDE
TRAPHANDLER_NOEC(Xirq15,  T_IRQ0+15)

TRAPHANDLER_NOEC(Xsyscall, T_SYSCALL)	// System call
TRAPHANDLER_NOEC(Xltimer,  T_LTIMER)	// Local APIC timer
TRAPHANDLER_NOEC(Xlerror,  T_LERROR)	// Local APIC error
TRAPHANDLER_NOEC(Xperfctr,  T_PERFCTR)	// Performance counter interrupt

#endif	// SOL >= 2
/* default handler -- not for any specific trap */
TRAPHANDLER	(Xdefault, T_DEFAULT)


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

	xorl %ebp,%ebp		# don't let debug_trace() walk into user space

	pushl %esp		# pass pointer to this trapframe 
	call trap		# and call trap (does not return)

1:	jmp 1b			# should never get here; just spin...

#else /* SOL >= 1 */	
/*
 * Lab 1: Your code here for _alltraps
 */

#endif // SOL >= 1


//
// Trap return code.
// C code in the kernel will call this function to return from a trap,
// providing the 
// Restore the CPU state from a given trapframe struct
// and return from the trap using the processor's 'iret' instruction.
// This function does not return to the caller,
// since the new CPU state this function loads
// replaces the caller's stack pointer and other registers.
//
.globl	trap_return
.type	trap_return,@function
.p2align 4, 0x90		/* 16-byte alignment, nop filled */
trap_return:
#if SOL >= 1
	movl	4(%esp),%esp	// reset stack pointer to point to trap frame
	popal			// restore general-purpose registers except esp
	popl	%es		// restore data segment registers
	popl	%ds
	addl	$8,%esp		// skip tf_trapno and tf_errcode
	iret			// return from trap handler
#else // SOL >= 1
/*
 * Lab 1: Your code here for trap_return
 */
1:	jmp	1b		// just spin
#endif // SOL >= 1

#endif // LAB >= 1
