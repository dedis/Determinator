#if LAB >= 2
/*
 * Boot code for Application Processors (APs) in a multiprocessor system.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 */

#include <inc/mmu.h>
#include <kern/mem.h>

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must sit
# at an address in the low 2^16 bytes.
#
# Bootothers (in main.c) sends the STARTUPs, one at a time.
# It puts this code (start) at 0x7000.
# It puts the correct %esp in start-4,
# and the place to jump to in start-8.
#
# This code is identical to bootasm.S except:
#   - it does not need to enable A20
#   - it uses the address at start-4 for the %esp
#   - it jumps to the address at start-8 instead of calling bootmain

#define SEG_KCODE 1  // kernel code
#define SEG_KDATA 2  // kernel data+stack

#define SEG_CODE_16 3 //16 bit CS
#define SEG_DATA_16 4 //16 bit CS


.code16                       # Assemble for 16-bit mode
.globl start
start:
	.long bootother
	.long bioscall

bootother:

	cli                         # Disable interrupts

	# Set up the important data segment registers (DS, ES, SS).
	xorw    %ax,%ax             # Segment number zero
	movw    %ax,%ds             # -> Data Segment
	movw    %ax,%es             # -> Extra Segment
	movw    %ax,%ss             # -> Stack Segment

	# Switch from real to protected mode, using a bootstrap GDT
	# and segment translation that makes virtual addresses 
	# identical to physical addresses, so that the 
	# effective memory map does not change during the switch.
	lgdt    gdtdesc
	movl    %cr0, %eax
	orl     $CR0_PE, %eax
	movl    %eax, %cr0

	# Jump to next instruction, but in 32-bit code segment.
	# Switches processor into 32-bit mode.
	ljmp    $(SEG_KCODE<<3), $start32

.code32                       # Assemble for 32-bit mode
start32:
	# Set up the protected-mode data segment registers
	movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
	movw    %ax, %ds                # -> DS: Data Segment
	movw    %ax, %es                # -> ES: Extra Segment
	movw    %ax, %ss                # -> SS: Stack Segment
	movw    $0, %ax                 # Zero segments not ready for use
	movw    %ax, %fs                # -> FS
	movw    %ax, %gs                # -> GS

	# Set up the stack pointer and call into C.
	movl    start-4, %esp
	call	*(start-8)

	# If the call returns (it shouldn't), trigger a Bochs
	# breakpoint if running under Bochs, then loop.
	movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
	movw    %ax, %dx
	outw    %ax, %dx
	movw    $0x8e00, %ax            # 0x8e00 -> port 0x8a00
	outw    %ax, %dx
spin:
	jmp     spin


.code32
.globl bioscall
bioscall:
	//we are still in 32-bit mode.

	pushal
	pushl %fs
	pushl %gs
	pushl %ds
	pushl %es
	pushl %ss

	pushl %ebx
	pushl %esi
	pushl %edi
	pushl %ebp

	movl $PROT_ESP,%eax
	movl %esp,0x0(%eax)

	//save the protected mode IDT and GDT
	sidt IDT_MEM_LOC
	sgdt GDT_MEM_LOC

	movl SMAP,%edx
	movl $24,%ecx
	movl $0,%ebx


	//start the transition into real mode
	cli

	// disable paging 
//TODO:: Save the paging bit in a location and restore it. Dont disable and enable (since bios may be called w or w/o paging)
	movl    %cr0,%eax
	//andl    $~CR0_PG,%eax
	movl    %eax,%cr0

	//flush TLB
	movl $0,%eax
	movl  %eax,%cr3


	lgdt gdtdesc
	ljmp  $(SEG_CODE_16<<3),$1f

.code16
1:
	//in 16 bit protected mode

	movw $(SEG_DATA_16<<3),%ax
	movw %ax,%ds
	movw %ax,%ss
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs

	lidt realidtptr

	//patch the int instruction
	movw $start,%bp 
	movb INT_NO(%bp),%al
	movb %al,int_call+1

	//disable protection bit
	movl %cr0,%eax
	andl $~CR0_PE,%eax
	movl %eax,%cr0


	ljmp $0,$1f

//real mode begins
1:

	// reload the real stack segment
	xorw %ax,%ax
	movw %ax,%ss
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs

	//set up the real mode sp
	movw REAL_STACK_HI,%sp
	
	//setup for the bios call
	movw $start,%bp 
	//load the registers needed by the BIOS
	movl EAX(%bp),%eax
	movl EBX(%bp),%ebx
	movl ECX(%bp),%ecx
	movl EDX(%bp),%edx
	movl ESI(%bp),%esi
	movl EDI(%bp),%edi
	movw DS(%bp),%ds
	movw ES(%bp),%es

	

	//make the bios call
	//XXX: Use interrupt vector at $start+INT_NO !!
	cli
int_call:
	int $0


	//move the register values back to the struct
	movw $start,%bp 
	movl %eax,EAX(%bp)
	movl %ebx,EBX(%bp)
	movl %ecx,ECX(%bp)
	movl %edx,EDX(%bp)
	movl %esi,ESI(%bp)
	movl %edi,EDI(%bp)
	movw %ds,DS(%bp)
	movw %es,ES(%bp)

	//check the carry flag
	setc %al
	movb %al,CF(%bp)

	//prepare to go back in 32 bit
//	cli

	//load the protected mode gdt	
	//lgdt gdtdesc
	lidt IDT_MEM_LOC
	lgdt GDT_MEM_LOC


	// re-enter protected mode
	movl    %cr0, %eax
	orl     $CR0_PE, %eax
	movl    %eax, %cr0

	ljmp    $(SEG_KCODE<<3), $1f
.code32
1:      // we are now in a 32-bit protected mode code segment.


	//set the segment registers
	movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
	movw    %ax, %ds                # -> DS: Data Segment
	movw    %ax, %es                # -> ES: Extra Segment
	movw    %ax, %ss                # -> SS: Stack Segment
	movw    $0, %ax                 # Zero segments not ready for use
	movw    %ax, %fs                # -> FS
	movw    %ax, %gs                # -> GS

	//enable paging
	movl    %cr0,%eax
	//orl    $CR0_PG,%eax
	movl    %eax,%cr0


	//restore protected mode stack
	movl $PROT_ESP,%eax
	movl 0x0(%eax),%esp


	popl %ebp
	popl %edi
	popl %esi
	popl %ebx

	popl %ss
	popl %es
	popl %ds
	popl %gs
	popl %fs
	popal

	ret


# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
	SEG_NULL				# null seg
	SEG(STA_X|STA_R, 0x0, 0xffffffff)	# code seg (32 bit)
	SEG(STA_W, 0x0, 0xffffffff)		# data seg (32 bit)
	SEG16(STA_X|STA_R|STA_W,0x0,0xffff)  		#code seg (16 bit)
	SEG16(STA_W|STA_R,0x0,0xffff)        		#data seg (16 bit)

gdtdesc:
	.word   (gdtdesc - gdt -1)             # sizeof(gdt) - 1
	.long   gdt                             # address gdt

realidtptr:
	.word   4*256-1                 // limit
	.long   0                       // base


#endif	// LAB >= 2
