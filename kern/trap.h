#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>


/* The user trap frame is always at the top of the kernel stack */
#define UTF	((struct Trapframe*)(KSTACKTOP - sizeof(struct Trapframe)))

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];

// Page fault modes inside kernel.
#define PFM_NONE	0	// No page faults expected: must be
				// a kernel bug.  On fault, panic.
#define PFM_KILL	1	// On fault, kill user process.

extern int page_fault_mode;


void idt_init(void);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

#endif /* JOS_KERN_TRAP_H */
#endif /* LAB >= 3 */
