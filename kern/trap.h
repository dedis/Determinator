#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#ifndef _KERN_TRAP_H_
#define _KERN_TRAP_H_

#include <inc/trap.h>
#include <inc/mmu.h>


/* The user trap frame is always at the top of the kernel stack */
#define UTF	((struct Trapframe*)(KSTACKTOP-sizeof(struct Trapframe)))

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];

/*
 * Page fault modes inside kernel.
 */
#define PFM_NONE 0x0     // No page faults expected.  Must be a kernel bug
#define PFM_KILL 0x1     // On fault kill user process.

extern u_int page_fault_mode;


void idt_init(void);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

#endif /* _KERN_TRAP_H_ */
#endif /* LAB >= 3 */
