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
#include <inc/mmu.h>
#include <kern/cpu.h>
#endif

#define KERN_CR0	(CR0_PE|CR0_MP|CR0_ET|CR0_WP|CR0_PG)
#define KERN_CR4	(CR4_PSE|CR4_PAE|CR4_PGE)
#define KERN_EFER	(EFER_LME|EFER_SCE|EFER_NXE)

#define TSS_SIZE	0x68	// minimum possible size
// Pad code to an appropriate boundary for speed using NOPs
#define TEXTALIGN	.p2align 4,,15

#define MULTIBOOT_PAGE_ALIGN  (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_MEMORY_INFO | MULTIBOOT_PAGE_ALIGN)
#define CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

###################################################################
# entry point
###################################################################

.text

# The Multiboot header
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM

/*
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

	# now to C code
	call	init

	# Should never get here, but in case we do, just spin.
1:	jmp	1b
*/

.globl		start,_start
start: _start:

// A MultiBoot boot loader starts us out in 32-bit protected mode.
	.code32
entry32:
	cld

	// activate our own protected-mode GDT
	lgdt	kerngdtptr

	// load our protected mode data segment registers
	movw	$SEG_KERN_DS_32,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss

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

	// Set up the boot page tables to map the bottom 2MB of memory,
	// using a single 2MB global (non-TLB-flushed) page translation.
	// We map it only at virtual address 0
	movl	$bootp3tab+(PTE_P|PTE_RW),bootp4tab
	movl	$bootp2tab+(PTE_P|PTE_RW),bootp3tab
	movl	$0+(PTE_P|PTE_RW|P2E_PS|P1E_G),bootp2tab

	// load CR3 to point to our boot page table structure
	movl	$bootp4tab,%eax
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
	ljmp	$SEG_KERN_CS_64,$entry64

.code64
entry64:	// we are now in 64-bit mode.

	// Load our long-mode GDT, IDT, and TSS
	// All these are loaded again in init
	lgdt	kerngdtptr
	// RAJAT says we do not need this.
	// IDTR for each CPU loaded in trap_init()
	// lidt	kernidtptr
	movw	$SEG_TSS,%ax
	ltr		%ax

	// initialize stack and frame pointers
	movq	$start,%rsp	// XXX
	xorq	%rbp,%rbp

	// now to C code
	call	init

	// should not reach here
spin:	jmp	spin



// Temporary 32-bit GDT for switching to protected mode, then long mode.
// Place it in our text segment to ensure that it stays
// in the lowest 64KB real-mode segment.
	.p2align 3
	.globl gdt
gdt:
	SEG32(0,0,0,0)				// null segment
	SEG32(0,0xffff,0x9b,0x0)	// 0x08: 16-bit kernel code segment
	SEG32(0,0xfffff,0x9b,0xc)	// 0x10: 32-bit kernel code segment
	SEG32(0,0xfffff,0x93,0xc)	// 0x18: 32-bit kernel data segment
	SEG32(0,0xfffff,0x9a,0xa)	// 0x20: 64-bit kernel segment
	SEG32(0,0xfffff,0xfa,0xa)	// 0x28: 64-bit user segment
								// 0x30: 64-bit task state segment
	.word	TSS_SIZE-1			//	limit 15-0
	.word	tss					//	base 15-0
	.long	0x00008000 | (STS_T64A << 8)		//	base 31-16, limit 19-16, type
	.long	0					//	base address 63-32
	.long	0					//	reserved
gdtend:


/*
////////// 64-bit Interrupt Descriptor Table //////////

// Macro to define IDT interrupt gates and corresponding entrypoints.
// - num: the trap number, which we push on the stack after the error code
// - dpl: descriptor privilege level for the gate
// - trap: 1 for a trap gate, 0 for an interrupt gate
// - ist: Interrupt Stack Table number
// - pusherr: code to push a dummy error code, if the processor didn't
#define IDTENT(num,dpl,trap,ist,pusherr)	\
	.data; \
	.word	1f, SEG_KERN_CS_64; \
	.word	0x8e00 | ((dpl)<<13) | (trap << 8) | ist; \
	.word	0; \
	.long	0,0;  \
	.text; \
	TEXTALIGN;  \
	1: pusherr; pushq $(num); jmp trap_entry

#define IDTENT_RESERVED \
	.data; .quad 0,0

// Define the actual IDT and trap entrypoints
	.data
	.p2align 3
	.globl	idt,idtend
idt:
IDTENT(T_DIVIDE,	0, 0, 0, pushq $0)
IDTENT(T_DEBUG,	0, 0, 0, pushq $0)
IDTENT(T_NMI,	0, 0, 0, pushq $0)
IDTENT(T_BRKPT,	3, 0, 0, pushq $0)
IDTENT(T_OFLOW,	0, 0, 0, pushq $0)
IDTENT(T_BOUND,	0, 0, 0, pushq $0)
IDTENT(T_ILLOP,	0, 0, 0, pushq $0)
IDTENT(T_DEVICE,	0, 0, 0, pushq $0)
IDTENT(T_DBLFLT,	0, 0, 1, pushq $0)	// switch to known-good stack
IDTENT_RESERVED
IDTENT(T_TSS,	0, 0, 0, )
IDTENT(T_SEGNP,	0, 0, 0, )
IDTENT(T_STACK,	0, 0, 0, )
IDTENT(T_GPFLT,	0, 0, 0, )
IDTENT(T_PGFLT,	0, 0, 0, )
IDTENT_RESERVED
IDTENT(T_FPERR,	0, 0, 0, pushq $0)
IDTENT(T_ALIGN,	0, 0, 0, )
IDTENT(T_MCHK,	0, 0, 1, pushq $0)	// switch to known-good stack
IDTENT(T_SIMD,	0, 0, 0, pushq $0)
	.data
1:	.space		(16*256)-(1b-idt)
idtend:

// Common trap entrypoint code
	.text
	TEXTALIGN
trap_entry:
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rdi
	pushq %rbp
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movw %ds,%ax
	pushw %ax
	movw %es,%ax
	pushw %ax
	pushw %fs
	pushw %gs

	movl $SEG_KERN_CS_64,%eax 	// load kernel's data segment
	movw %ax,%ds
	movw %ax,%es

	xorq %rbp,%rbp				// don't let debug_trace() walk into user space

	pushq %rsp					// pass pointer to this trapframe
	call	trap
1:	jmp 1b						// must not reach here; if it does, just spin

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
.p2align 4, 0x90		// 16-byte alignment, nop filled
trap_return:
	movq	8(%rsp),%rsp	// reset stack pointer to point to trap frame

	popw %gs
	popw %fs
	popw %ax
	movw %ax,%es
	popw %ax
	movw %ax,%ds
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rbp
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	addq	$8,%rsp		// skip trapno and errcode
	iretq				// return from trap handler
1:	jmp	1b				// must not reach here; if it does, just spin
*/


// 64-bit Task State Segment
	.data
	.p2align 2
	.globl	tss
tss:
	.long	0				// reserved
	.quad	start, 0, 0			// RSP0, RSP1, RSP2
	.quad	0				// reserved
	.quad	start, 0, 0, 0, 0, 0, 0		// IST1 ... IST7
	.quad	0				// reserved
	.word	0				// reserved
	.word	TSS_SIZE			// I/O Map Base Address


// Far pointers for loading the GDT and IDT
kerngdtptr:
	.word	gdtend-gdt-1		// limit
	.quad	gdt			// base (kernel linear address)

kernidtptr:
	.word	idtend-idt-1		// limit
	.quad	idt			// base


// Space for minimal bootstrap page table structures
	.bss
	.p2align 12
	.globl bootp4tab, bootp3tab, bootp2tab
bootp4tab:
	.space	4096
bootp3tab:
	.space	4096
bootp2tab:
	.space	4096


#endif // LAB >= 1
