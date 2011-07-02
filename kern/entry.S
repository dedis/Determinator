#if LAB >= 1

/*
 * Kernel entrypoint and stack setup.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 * Adapted for 64-bit PIOS by Rajat Goyal at IIT Delhi
 */

#include <inc/mmu.h>
#include <kern/cpu.h>

#define MULTIBOOT_PAGE_ALIGN  (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_MEMORY_INFO | MULTIBOOT_PAGE_ALIGN)
#define CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

###################################################################
# kernel entry point
# boot loader leaves us in 32-bit mode
###################################################################

.text
# The Multiboot header
.p2align 2
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM

.code32
.globl	start,_start
start: _start:
	movw	$0x1234,0x472			# warm boot BIOS flag

	cld

	// activate our own protected-mode GDT
	lgdt	kerngdtptr

	// load our protected mode data segment registers
	movw	$SEG_KERN_DS_32,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movw	%ax,%fs
	movw	%ax,%gs

	// clear our bss segment
	// (now that we have enough addressing reach to access it)
	xorl	%eax,%eax
	movl	$edata,%edi
	movl	$end,%ecx
	subl	%edi,%ecx
	shrl	$2,%ecx
	rep stosl

	// enable various paging extensions we want use
	movl	%cr4,%eax
	orl	$KERN_CR4,%eax
	movl	%eax,%cr4

	// Set up the boot page tables to map the bottom 4KB of memory,
	// using a single 4KB  global (non-TLB-flushed) page translation.
	// We map it only at virtual address 0
	movl	$bootp3tab+(PTE_P|PTE_W),bootp4tab
	movl	$bootp2tab+(PTE_P|PTE_W),bootp3tab
	movl	$0+(PTE_P|PTE_W|PTE_G|PTE_PS),bootp2tab

	// load CR3 to point to our boot page table structure
	movl	$bootp4tab,%eax
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
	ljmp	$SEG_KERN_CS_64,$entry64

.code64
entry64:	// we are now in 64-bit mode.

	// Load our long-mode GDT, IDT, and TSS
	// All these are loaded again in init
	// lgdt	kerngdtptr
	// IDTR and TSS for each CPU loaded in trap_init()
	// lidt	kernidtptr
	// ltr	$SEG_TSS

	movw	$SEG_KERN_DS_64,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movw	%ax,%fs
	movw	%ax,%gs

	// initialize stack and frame pointers
	movq	$start,%rsp
	# Clear the frame pointer register (RBP)
        # so that once we get into debugging C code,
        # stack backtraces will be terminated properly.
	xorq	%rbp,%rbp

	// now to C code
	call	init

	// should not reach here
spin:	jmp	spin


// Temporary GDT for switching to protected mode, then long mode.
// Place it in our text segment to ensure that it stays
// in the lowest 64KB real-mode segment.
	.p2align 3
	.globl gdt
gdt:
	SEGNULL					// null segment
	SEG64(0,0xfffff,STA_X | STA_R,0,0)	// 0x10: 32-bit kernel code segment
	SEG64(0,0xfffff,STA_W,0,0)		// 0x20: 32-bit kernel data segment
	SEG64(0,0xfffff,STA_X | STA_R,0,1)	// 0x30: 64-bit kernel code segment
	SEG64(0,0xfffff,STA_W,0,1)		// 0x40: 64-bit kernel data segment
	SEG64(0,0xfffff,STA_X | STA_R,3,1)	// 0x50: 64-bit user segment
gdtend:

	.data
kerngdtptr:
        .word   gdtend-gdt-1            // limit
        .quad   gdt                     // base (kernel linear address)


// Space for minimal bootstrap page table structures
	.bss
	.p2align 12
	.globl bootp4tab, bootp3tab, bootp2tab, bootp1tab
bootp4tab:
	.space	4096
bootp3tab:
	.space	4096
bootp2tab:
	.space	4096
bootp1tab:
	.space	4096


#endif // LAB >= 1
