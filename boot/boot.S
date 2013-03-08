#if LAB >= 1
/*
 * Boot loader entrypoint and protected mode setup.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for 64-bit PIOS by Rajat Goyal at IIT Delhi
 */

#include <inc/mmu.h>

# Start the CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.set SEG_KERN_CS_32, 0x8         # kernel code segment selector
.set SEG_KERN_DS_32, 0x10        # kernel data segment selector
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

  # Before switching to protected mode, get memory map info
  # first, using int 0x15, eax = 0xe820 bios function.
  # Memory map info will be stored to area starting from
  # 0x2004 (es:di = 0000:2004), each entry will be 20 byte long.
  # 0x2000 stores number of entries
e820:
  xorl    %ebx, %ebx
  xorw    %bp, %bp
  movw    $0x2004, %di
  movl    $0x534D4150, %edx       # "SMAP"
  movl    $0xe820, %eax
  movw    $20, %cx
  int     $0x15
  jc      spin                    # first carry means "unsupported function"
  movl    $0x534D4150, %edx       # "SMAP"
  cmpl    %eax, %edx
  jne     spin
  test    %ebx, %ebx
  jz      spin                    # ebx = 0 means end of list
  jmp     e820_next
e820_read:
  movl    $0x534D4150, %edx       # "SMAP"
  movl    $0xe820, %eax
  movw    $20, %cx
  int     $0x15
  jc      e820_finish             # carry now means "end of list reached"
  cmpw    $20, %cx
  jg      e820_skip               # entry has at least 20 bytes
e820_next:
  inc     %bp
  addw    $20, %di
e820_skip:
  test    %ebx, %ebx
  jnz     e820_read
e820_finish:
  movw    $0x2000, %di
  movl    %ebp, (%di)
  jmp     setprot

  # Switch from real to protected mode, using a bootstrap GDT
  # and segment translation that makes virtual addresses 
  # identical to their physical addresses, so that the 
  # effective memory map does not change during the switch.
setprot:
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
  call bootmain

  # If bootmain returns (it shouldn't), loop.
spin:
  jmp spin

.p2align 2
gdt:
        SEGNULL32                              # null segment
        SEG32(1,STA_X|STA_R,0,0xffffffff,0,0)  # 0x10: code segment
        SEG32(1,STA_W,0,0xffffffff,0,0)        # 0x20: data segment

gdtdesc:
  .word   (gdtdesc-gdt-1)                 # sizeof(gdt) - 1
  .long   gdt                             # address gdt

#endif /* LAB >= 1 */
