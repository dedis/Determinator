#///LAB2	
/*
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */


#include <inc/asm.h>
#include <inc/mmu.h>
#include <kern/trap.h>
#include <kern/picirq.h>

###################################################################
# The kernel (this code) is linked at address ~(KERNBASE + 1 Meg), 
# but the bootloader load it at address	~1 Meg.
#	
# RELOC(x) maps a symbol x from its link address to its actual
# location in physical memory (its load address).	 
###################################################################
	
#define	RELOC(x) ((x) - KERNBASE)
	
###################################################################	
# See mmu.h for a complete description of these two.
###################################################################
.data
	.globl	_vpt
	.set	_vpt, VPT
	.globl	_vpd
	.set	_vpd, (VPT + SRL(VPT, 10))

###################################################################
# boot stack
###################################################################
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		_bootstacktop   
_bootstacktop:

###################################################################
# setup the GDT	
###################################################################
.set CODE_SEL,0x8				# index of code seg within mygdt
.set DATA_SEL,0x10				# index of data seg within mygdt
.p2align	2				# force 4 byte alignment
mygdt:
	SEG_NULL				# null seg
	SEG(STA_X|STA_R, -KERNBASE, 0xffffffff)	# code seg
	SEG(STA_W, -KERNBASE, 0xffffffff)	# data seg
mygdtdesc:
	.word	0x17			# sizeof (mygdt) - 1
	.long	RELOC(mygdt)		# address mygdt
	
###################################################################
# entry point
###################################################################

.text
.globl		start
start:
	movw	$0x1234,0x472			# warm boot
	lgdt	RELOC(mygdtdesc)		# load descriptor
	movl	$DATA_SEL, %eax			# reload seg regs
	movw	%ax,%es				#
	movw	%ax,%ds				#
	movw	%ax,%ss				#
	ljmp	$CODE_SEL,$relocated		# reload CS
relocated:
	movl	$0x0,%ebp			# And nuke frame pointer
#///LAB200	
	# XXX use sizeof(.)
#///END	
        # Leave a few words on the stack for a user utf trap frame 
	movl	$(_bootstacktop-0x40),%esp	# set the stack pointer 

	/* Zero the BSS */
	xorl	%eax,%eax			# quickest way to get zero
	cld					# d=0 string op is ascending
	movl	$_edata,%edi			# destination of string op
	movl	$(3 + _end),%ecx		# repeat count for "rep"
	subl	$_edata,%ecx			#  = ceil ((_end-_edata)/4)
	shr	$2, %ecx
	rep
	 stosl					# store it!
	
	# now to C code
	call	_i386_init
	
#///END 2
#///LAB3
	
###################################################################
# exceptions/interrupts
###################################################################

/* For certain traps the CPU automatically pushes an error code, for 
 * all other traps the IDTFUNC_NOEC() pushes a 0 in place of the error code,
 * so the trap frame has the same format.
 */
#define IDTFNC(name,num)      ENTRY(name)           pushl $(num); jmp _alltraps
#define IDTFNC_NOEC(name,num) ENTRY(name) pushl $0; pushl $(num); jmp _alltraps 

#///LAB4

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
/* the device interrupts */
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
	call	_trap		#  and call trap
	addl $0x4,%esp		# deallocate arg.
	popal			# restore
	popl %es		#  registers
	popl %ds		#
	addl $0x8,%esp		# skip trapno and errcode
	iret			# go back..
#///END 4


.globl _clock_interrupt
_clock_interrupt:
	pushl	%eax		# push caller-saves
	pushl	%ecx		#  (clock() might trash these)
	pushl	%edx		#
	pushl   %ds		# load kernel seg regs 
	pushl	%es		#  saving the current contents 
	movl $GD_KD,%eax	
	movw %ax,%ds
	movw %ax,%es
	call	_clock		# call C handler
	popl	%es		# restore seg regs
	popl	%ds
	popl	%edx
	popl	%ecx
	popl	%eax
	iret			# return to whence we came..	
#///END
