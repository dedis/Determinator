#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#include <inc/asm.h>
#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#if LAB >= 4
#include <kern/picirq.h>
#endif


###################################################################
# exceptions/interrupts
###################################################################

/* For certain traps the CPU automatically pushes an error code.  For 
 * all other traps, IDTFUNC_NOEC() pushes a 0 in place of the error code,
 * so the trap frame has the same format.
 */
#define IDTFNC(name,num)      ENTRY(name)           pushl $(num); jmp _alltraps
#define IDTFNC_NOEC(name,num) ENTRY(name) pushl $0; pushl $(num); jmp _alltraps

.text

#if SOL >= 3

/* CPU traps */
IDTFNC_NOEC(Xdivide, T_DIVIDE)
IDTFNC_NOEC(Xdebug,  T_DEBUG)
IDTFNC_NOEC(Xnmi,    T_NMI)
IDTFNC_NOEC(Xbrkpt,  T_BRKPT)
IDTFNC_NOEC(Xoflow,  T_OFLOW)
IDTFNC_NOEC(Xbound,  T_BOUND)
IDTFNC_NOEC(Xillop,  T_ILLOP)
IDTFNC_NOEC(Xdevice, T_DEVICE)
IDTFNC     (Xdblflt, T_DBLFLT)
IDTFNC     (Xtss,    T_TSS)
IDTFNC     (Xsegnp,  T_SEGNP)
IDTFNC     (Xstack,  T_STACK)
IDTFNC     (Xgpflt,  T_GPFLT)
IDTFNC     (Xpgflt,  T_PGFLT)
IDTFNC_NOEC(Xfperr,  T_FPERR)
IDTFNC     (Xalign,  T_ALIGN)
IDTFNC_NOEC(Xmchk,   T_MCHK)

#if SOL >= 4
/* the external device interrupts */
IDTFNC_NOEC(Xirq0,   IRQ_OFFSET+0)
IDTFNC_NOEC(Xirq1,   IRQ_OFFSET+1)
IDTFNC_NOEC(Xirq2,   IRQ_OFFSET+2)
IDTFNC_NOEC(Xirq3,   IRQ_OFFSET+3)
IDTFNC_NOEC(Xirq4,   IRQ_OFFSET+4)
IDTFNC_NOEC(Xirq5,   IRQ_OFFSET+5)
IDTFNC_NOEC(Xirq6,   IRQ_OFFSET+6)
IDTFNC_NOEC(Xirq7,   IRQ_OFFSET+7)
IDTFNC_NOEC(Xirq8,   IRQ_OFFSET+8)
IDTFNC_NOEC(Xirq9,   IRQ_OFFSET+9)
IDTFNC_NOEC(Xirq10,  IRQ_OFFSET+10)
IDTFNC_NOEC(Xirq11,  IRQ_OFFSET+11)
IDTFNC_NOEC(Xirq12,  IRQ_OFFSET+12)
IDTFNC_NOEC(Xirq13,  IRQ_OFFSET+13)
IDTFNC_NOEC(Xirq14,  IRQ_OFFSET+14)
IDTFNC_NOEC(Xirq15,  IRQ_OFFSET+15)
#endif

/* system call entry point */
IDTFNC_NOEC(Xsyscall, T_SYSCALL)

/* default handler -- not for any specific trap */
IDTFNC     (Xdefault, T_DEFAULT)


SUPERALIGN_TEXT
.globl	_alltraps
.type	_alltraps,@function
_alltraps:
	pushl   %ds		# build
	pushl	%es		#  trap
	pushal			#  frame
	movl $GD_KD,%eax	# load
	movw %ax,%ds		#  kernel
	movw %ax,%es		#  segments
	pushl %esp		# pass pointer to this trapframe 
	call	trap		#  and call trap (does not return)
spin:	jmp spin
#endif // SOL >= 3

#endif // LAB >= 3
