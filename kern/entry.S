#if LAB >= 1
/* See COPYRIGHT for copyright information. */

#if LAB >= 3
#include <inc/trap.h>
#endif


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

.globl		_start
_start:
	movw	$0x1234,0x472			# warm boot BIOS flag

	# Clear the frame pointer register (EBP)
	# so that once we get into debugging C code,
	# stack backtraces will be terminated properly.
	movl	$0x0,%ebp			# nuke frame pointer

#if LAB >= 3
	# Leave a few words on the stack for the user trap frame
	movl	$(bootstacktop-SIZEOF_STRUCT_TRAPFRAME),%esp
#else
	# Set the stack pointer
	movl	$(bootstacktop),%esp
#endif

	# now to C code
	call	init

	# Should never get here, but in case we do, just spin.
spin:	jmp	spin


###################################################################
# boot stack
###################################################################
#	.p2align	PAGESHIFT		# XXX force page alignment
	.globl		bootstack
bootstack:
	.space		8*4096			# XXX KSTACKSIZE
	.globl		bootstacktop   
bootstacktop:

#endif // LAB >= 1
