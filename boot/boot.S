///LAB1
#include <inc/asm.h>
#include <inc/mmu.h>
	
.set PROT_MODE_CSEG,0x8		# code segment selector
.set PROT_MODE_DSEG,0x10        # data segment selector
.set CR0_PE_ON,0x1		# protected mode enable flag
	
###################################################################################
# ENTRY POINT	
#   This code should be stored in the first sector of the hard disk.  When the  
#   BIOS runs, it loads this code at physical address 0x7c00 - 0x7d00 (512 bytes).
#   Then jumps to the beginning of it, in real-mode (BIOS runs in real mode).  
#	
# This code switches into protected mode (32-bit mode) so that all of
# memory can accessed, then calls into C.
###################################################################################
	
.globl start					# Entry point	
start:		.code16				# This runs in real mode
		cli				# Disable interrupts
		cld				# String ops inc
		xorw	%ax,%ax			# Zero
		movw	%ax,%es			# Address
		movw	%ax,%ds			#  data
		movw	%ax,%ss			# Set up
		movw	$start,%sp         	#  stack (grows down)
	
#### Enable A20:
####   Address line 20 is tied to low when the machine boots, 
####   obviously this a bit of a drag, such as when trying to
####   address memory above 1MB.  This code undoes this.
	
seta20.1:	inb	$0x64,%al		# Get status
		testb	$0x2,%al		# Busy?
		jnz	seta20.1		# Yes
		movb	$0xd1,%al		# Command: Write
		outb	%al,$0x64		#  output port
seta20.2:	inb	$0x64,%al		# Get status
		testb	$0x2,%al		# Busy?
		jnz	seta20.2		# Yes
		movb	$0xdf,%al		# Enable
		outb	%al,$0x60		#  A20

#### switch from real to protected mode	
####     The segments in GDT allow all of physical memory to be accessed.
####     Furthermore, the segments have base addresses of 0, so that the
####     segment translation is a NOP, ie. virtual addresses are identical to
####     their physical addresses.  With this setup, it appears to this code
####     that it is running directly on physical memory.
	
real_to_prot:	cli				# Mandatory since we dont set up an IDT
		lgdt	gdtdesc			# load GDT -- mandatory in protected mode
		movl	%cr0, %eax		# turn on protected mode
		orl	$CR0_PE_ON, %eax	# 
		movl	%eax, %cr0		# 
	        ### CPU magic: jump to relocation, flush prefetch queue, and reload %cs
		### Has the effect of just jmp to the next instruction, but simultaneous
		### loads CS with $PROT_MODE_CSEG.
		ljmp	$PROT_MODE_CSEG, $protcseg
	
#### we are in protected mode (hence the .code32)
.code32
protcseg:	
		movw	$PROT_MODE_DSEG, %ax	# set up data segment registers
		movw	%ax, %ds		#
		movw	%ax, %es		#
		movw	%ax, %fs		#
		movw	%ax, %gs		#
		movw	%ax, %ss		#
	
		call _cmain			# finish the boot load from C.
						# cmain() should not return
spin:		jmp spin			# ..but in case it does spin
	
.p2align 2					# force 4 byte alignment
gdt:
	SEG_NULL				# null seg
	SEG(STA_X|STA_R, 0x0, 0xffffffff)	# code seg
	SEG(STA_W, 0x0, 0xffffffff)	        # data seg
	
gdtdesc:
	.word	0x17			# sizeof (gdt) - 1
	.long	gdt			# address gdt
///END
