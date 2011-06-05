#if LAB >= 1
/*
 * Kernel entrypoint and stack setup.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#if LAB >= 3
#include <inc/trap.h>
#endif

#include <kern/symbols.h>

#define MULTIBOOT_PAGE_ALIGN  (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_MEMORY_INFO | MULTIBOOT_PAGE_ALIGN)
#define CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

#define KERN_CR0	(CR0_PE|CR0_MP|CR0_ET|CR0_WP|CR0_PG)
#define KERN_CR4	(CR4_PSE|CR4_PAE|CR4_PGE)
#define KERN_EFER	(EFER_LME|EFER_SCE|EFER_NXE)
#define SEG_KERN_CS_64  0x20    // 64-bit kernel code segment
#define MSR_EFER        0xc0000080      // Extended Feature Enable Register
#define KERNBASE        0xffffffffc0000000

###################################################################
# entry point
###################################################################
.code32
.text

# The Multiboot header
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM

.globl		start,_start
start: _start:
	movw	$0x1234,0x472			# warm boot BIOS flag

	# Clear the frame pointer register (EBP)
	# so that once we get into debugging C code,
	# stack backtraces will be terminated properly.
	movl	$0x0,%ebp			# nuke frame pointer

#if LAB >= 3
	# Leave a few words on the stack for the user trap frame
	movl	$(cpu_boot+4096-SIZEOF_STRUCT_TRAPFRAME),%esp
#else
	# Set the stack pointer
	movl	$(cpu_boot+4096),%esp
#endif
	// enable various paging extensions we want use
	movl	%cr4,%eax
	orl	$KERN_CR4,%eax
	movl	%eax,%cr4

	// enable long mode (and other EFER features we want)
	movl	$MSR_EFER,%ecx
	rdmsr
	orl	$KERN_EFER,%eax
	wrmsr

	// enable paging, and thus activate long mode
	movl	%cr0,%eax
	orl	$KERN_CR0,%eax
	movl	%eax,%cr0
	//ljmp	$SEG_KERN_CS_64,$1f-KERNBASE
	ljmp	$SEG_KERN_CS_64,$loopI

	.code64

loopI:	jmp loopI	

	# now to C code
	call	init

	# Should never get here, but in case we do, just spin.
spin:	jmp	spin


#endif // LAB >= 1
