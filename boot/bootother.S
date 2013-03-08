#if LAB >= 2
/*
 * Boot code for Application Processors (APs) in a multiprocessor system.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for 64-bit PIOS by Rajat Goyal at IIT Delhi
 */
/*
 *XXX We should really rename this 'kern/lowcode.S' or something like that,
 * since it's used in the kernel well after the bootloader is finished,
 * and it contains code that needs to run in the low 1MB of memory.
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
# It puts this code (start) at 0x7000. RAJAT says 0x1000
# It puts the correct %esp in start-4,
# and the place to jump to in start-8.
#
# This code is identical to boot.S except:
#   - it does not need to enable A20
#   - it uses the address at start-4 for the %esp
#   - it jumps to the address at start-8 instead of calling bootmain



.set SEG_KERN_CS_32, 0x10
.set SEG_KERN_DS_32, 0x20
.set SEG_KERN_CS_64, 0x30
.set SEG_KERN_DS_64, 0x40
.set SEG_USER_CS_64, 0x50
#ifdef BIOSCALL
.set SEG_KERN_CS_16, 0x60
.set SEG_KERN_DS_16, 0x70
#endif


.code16                       	# Assemble for 16-bit mode
.text
.globl start
start:	// Start with a table of pointers to lowcode entrypoints
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
	lgdt    kerngdtptr
	movl    %cr0, %eax
	orl     $CR0_PE, %eax
	movl    %eax, %cr0

	# Jump to next instruction, but in 32-bit code segment.
	# Switches processor into 32-bit mode.
	ljmp    $SEG_KERN_CS_32, $start32

.code32                       # Assemble for 32-bit mode
start32:
	# Set up the protected-mode data segment registers
	movw    $SEG_KERN_DS_32, %ax    # Our data segment selector
	movw    %ax, %ds                # -> DS: Data Segment
	movw    %ax, %es                # -> ES: Extra Segment
	movw    %ax, %ss                # -> SS: Stack Segment
	movw    $0, %ax                 # Zero segments not ready for use
	movw    %ax, %fs                # -> FS
	movw    %ax, %gs                # -> GS

	/*
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
	*/

	// enable various paging extensions we want use
	movl	%cr4,%eax
	orl	$KERN_CR4,%eax
	movl	%eax,%cr4

	// load CR3 to point to our boot page table structure
	movl	start-24,%eax
	movl	%eax,%cr3

	// enable long mode (and other EFER features we want)
	movl	$MSR_EFER,%ecx
	rdmsr
	orl	$KERN_EFER,%eax
	wrmsr

	// enable paging, and thus activate long mode
	movl	%cr0,%eax
	orl	$KERN_CR0,%eax
	movl	%eax,%cr0
	ljmp	$SEG_KERN_CS_64,$start64

.code64
start64:	// we are now in 64-bit mode.

	// Load our long-mode GDT, IDT, and TSS
	// All these are loaded again in init
	// lgdt	kerngdtptr

	movw	$SEG_KERN_DS_64,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movw	%ax,%fs
	movw	%ax,%gs

	// Set up the stack pointer, frame pointer and call into C.
	movq    start-8, %rsp
	xorq	%rbp,%rbp
	call	*(start-16)

	// have not setup Bochs breakpoint

	// should not reach here; if we do, just spin
spin:	jmp	spin


	.data
	.p2align 3
gdt:
	SEGNULL64				// null segment
	SEG64(1,STA_X|STA_R,0,0xffffffff,0,32)	// 0x10: 32-bit kernel code seg
	SEG64(1,STA_W,0,0xffffffff,0,32)	// 0x20: 32-bit kernel data seg
	SEG64(1,STA_X|STA_R,0,0xffffffff,0,64)	// 0x30: 64-bit kernel code seg
	SEG64(1,STA_W,0,0xffffffff,0,64)	// 0x40: 64-bit kernel data seg
	SEG64(1,STA_X|STA_R,0,0xffffffff,3,64)	// 0x50: 64-bit user code seg
	SEG64(1,STA_W,0,0xffffffff,3,64)	// 0x60: 64-bit user data seg
#ifdef BIOSCALL
	SEG64(XXX STA_X|STA_R|STA_W,0x0,0xffff)	// 0x70: 16-bit kernel code seg
	SEG64(XXX STA_W|STA_R,0x0,0xffff)      	// 0x80: 16-bit kernel data seg
#endif

kerngdtptr:
	.word	kerngdtptr-gdt-1	// limit
	.quad	gdt			// base (kernel linear address)



.code32
.globl bioscall
bioscall:

#ifdef BIOSCALL		// XXX not yet ported to 64-bit!
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

	movl $(BIOSCALL_MEM_START+PROT_ESP),%eax
	movl %esp,(%eax)

	//save the protected mode IDT and GDT
	sidt BIOSCALL_MEM_START+IDT_MEM_LOC
	sgdt BIOSCALL_MEM_START+GDT_MEM_LOC

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
	movw $(start-BIOSREGS_SIZE+BIOSREGS_INT_NO),%bp 
	movb (%bp),%al
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
	
	//load the registers needed by the BIOS
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EAX),%bp
	movl (%bp),%eax
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EBX),%bp
	movl (%bp),%ebx
	movw $(start-BIOSREGS_SIZE+BIOSREGS_ECX),%bp
	movl (%bp),%ecx
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EDX),%bp
	movl (%bp),%edx
	movw $(start-BIOSREGS_SIZE+BIOSREGS_ESI),%bp
	movl (%bp),%esi
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EDI),%bp
	movl (%bp),%edi
	movw $(start-BIOSREGS_SIZE+BIOSREGS_DS),%bp
	movw (%bp),%ds
	movw $(start-BIOSREGS_SIZE+BIOSREGS_ES),%bp
	movw (%bp),%es

	//make the bios call
	cli
int_call:
	int $0


	//move the register values back to the struct
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EAX),%bp
	movl %eax,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EBX),%bp
	movl %ebx,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_ECX),%bp
	movl %ecx,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EDX),%bp
	movl %edx,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_ESI),%bp
	movl %esi,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_EDI),%bp
	movl %edi,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_DS),%bp
	movw %ds,(%bp)
	movw $(start-BIOSREGS_SIZE+BIOSREGS_ES),%bp
	movw %es,(%bp)

	//check the carry flag
	setc %al
	movw $(start-BIOSREGS_SIZE+BIOSREGS_CF),%bp
	movb %al,(%bp)

	//prepare to go back in 32 bit
//	cli

	//load the protected mode gdt	
	//lgdt gdtdesc
	lidt BIOSCALL_MEM_START+IDT_MEM_LOC
	lgdt BIOSCALL_MEM_START+GDT_MEM_LOC


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
	movl $(BIOSCALL_MEM_START+PROT_ESP),%eax
	movl (%eax),%esp


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


	.data
	.p2align 3
realidtptr:
	.word   4*256-1                 // limit
	.long   0                       // base
#endif


#endif	// LAB >= 2
