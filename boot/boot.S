#if LAB >= 1
/*
 * Boot loader entrypoint and protected mode setup.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 */

/*
// OLD SEGMENT DESCRIPTORS
// Macros to build GDT entries in assembly.
#define SEG_NULL						\
	.word 0, 0;						\
	.byte 0, 0, 0, 0
#define SEG(type,base,limit)					\
	.word (((limit) >> 12) & 0xffff), ((base) & 0xffff);	\
	.byte (((base) >> 16) & 0xff), (0x90 | (type)),		\
		(0xC0 | (((limit) >> 28) & 0xf)), (((base) >> 24) & 0xff)
*/

// NEW SEGMENT DESCRIPTORS
#define SEGNULL         \
        .long 0,0,0,0
#define SEG32(base,limit,type,dpl,l)                                      \
        .word (((limit) >> 12) & 0xffff), ((base) & 0xffff);      \
        .byte (((base) >> 16) & 0xff), (0x90 | (type) | ((dpl)<<5)),         \
                (0x80 | ((((~(l)) << 1) | (l)) << 5) | (((limit) >> 28) & 0xf)), (((base) >> 24) & 0xff)
#define SEG64(base,limit,type,dpl,l) \
        SEG32(base,limit,type,dpl,l); \
        .long   (base >> 32), 0


# Start the CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.set SEG_KERN_CS_32, 0x10         # kernel code segment selector
.set SEG_KERN_DS_32, 0x20        # kernel data segment selector
.set CR0_PE_ON,      0x1         # protected mode enable flag

.globl start
start:
  .code16                     # Assemble for 16-bit mode
  cli                         # Disable interrupts
  cld                         # String operations increment

  # Set up the important data segment registers (DS, ES, SS).
  xorw    %ax,%ax             # Segment number zero
  movw    %ax,%ds             # -> Data Segment
  movw    %ax,%es             # -> Extra Segment
  movw    %ax,%ss             # -> Stack Segment

  # Enable A20:
  #   For backwards compatibility with the earliest PCs, physical
  #   address line 20 is tied low, so that addresses higher than
  #   1MB wrap around to zero by default.  This code undoes this.
seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60

  # Switch from real to protected mode, using a bootstrap GDT
  # and segment translation that makes virtual addresses 
  # identical to their physical addresses, so that the 
  # effective memory map does not change during the switch.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0
  
  # Jump to next instruction, but in 32-bit code segment.
  # Switches processor into 32-bit mode.
  ljmp    $SEG_KERN_CS_32, $protcseg

  .code32                     # Assemble for 32-bit mode
protcseg:
  # Set up the protected-mode data segment registers
  movw    $SEG_KERN_DS_32, %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  
  # Set up the stack pointer and call into C.
  movl    $start, %esp
  movl    $start, %ebp
  call bootmain

  # If bootmain returns (it shouldn't), loop.
spin:
  jmp spin

/*
// OLD GDT
# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
  SEG_NULL				# null seg
  SEG(0xA, 0x0, 0xffffffff)	# code seg
  SEG(0x2, 0x0, 0xffffffff)	        # data seg
*/

// NEW GDT
// Temporary 32-bit GDT for switching to protected mode, then long mode.
// Place it in our text segment to ensure that it stays
// in the lowest 64KB real-mode segment.
        .p2align 3
gdt:
        SEGNULL                         // null segment
        SEG64(0,0xfffff,0xa,0,0)        // 0x10: 32-bit kernel code segment
        SEG32(0,0xfffff,0x2,0,0)        // 0x20: 32-bit kernel data segment

gdtdesc:
  .word   (gdtdesc-gdt-1)                            # sizeof(gdt) - 1
  .long   gdt                             # address gdt

#endif /* LAB >= 1 */
