/* See COPYRIGHT for copyright information. */

///LAB3

#ifndef _KERN_TRAP_H_
#define _KERN_TRAP_H_

#include <inc/trap.h>
#include <inc/mmu.h>

extern struct Gatedesc idt[];
void idt_init();

/* The user trap frame is always at the top of the kernel stack */
#define UTF	((struct Trapframe*)(KSTACKTOP-sizeof(struct Trapframe)))

void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);
extern u_int page_fault_mode;

#endif /* !__ASSEMBLER__ */

#endif /* _KERN_TRAP_H_ */
///END
