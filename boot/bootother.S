#if LAB >= 2
/*
 * Boot code for Application Processors (APs) in a multiprocessor system.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 */

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


#include <inc/mmu.h>

.set SEG_KERN_CS_16, 0x10
.set SEG_KERN_CS_32, 0x20
.set SEG_KERN_DS_32, 0x30
.set SEG_KERN_CS_64, 0x40
.set SEG_USER_CS_64, 0x50

.code16                       	# Assemble for 16-bit mode
.text
.globl start
start:
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
	ljmp    $(SEG_KERN_CS_32), $start32

.code32                       # Assemble for 32-bit mode
start32:
	# Set up the protected-mode data segment registers
	movw    $(SEG_KERN_DS_32), %ax    # Our data segment selector
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
	orl		$KERN_EFER,%eax
	wrmsr

	// enable paging, and thus activate long mode
	movl	%cr0,%eax
	orl		$KERN_CR0,%eax
	movl	%eax,%cr0
	ljmp	$SEG_KERN_CS_64,$start64

.code64
start64:	// we are now in 64-bit mode.

	// Load our long-mode GDT, IDT, and TSS
	// All these are loaded again in init
	lgdt	kerngdtptr

	movw	$SEG_KERN_CS_64,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss

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
        SEGNULL                         // null segment
        SEG64(0,0xffff,0xa,0,0)         // 0x10: 16-bit kernel code segment
        SEG64(0,0xfffff,0xa,0,0)        // 0x20: 32-bit kernel code segment
        SEG64(0,0xfffff,0x2,0,0)        // 0x30: 32-bit kernel data segment
        SEG64(0,0xfffff,0xa,0,1)        // 0x40: 64-bit kernel segment
        SEG64(0,0xfffff,0xa,3,1)        // 0x50: 64-bit user segment
gdtend:

kerngdtptr:
	.word	gdtend-gdt-1		// limit
	.quad	gdt			// base (kernel linear address)

#endif	// LAB >= 2
