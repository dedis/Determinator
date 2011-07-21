#if LAB >= 1
/*
 * Processor trap handling module definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_KERN_TRAP_H
#define PIOS_KERN_TRAP_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>
#include <inc/cdefs.h>


// Arguments that trap_check() passes to trap recovery code,
// so that the latter can resume the trapping code
// at the appropriate point and return the trap number.
// There are two versions of the trap recovery code:
// one in kern/trap.c (used twice), the other in kern/proc.c.
typedef struct trap_check_args {
	void *rrip;		// In: RIP at which to resume trapping code
	int trapno;		// Out: trap number from trapframe
} trap_check_args;


// Initialize the trap-handling module and the processor's IDT.
void trap_init(void);

// Return a string constant describing a given trap number,
// or "(unknown trap)" if not known.
const char *trap_name(int trapno);

// Pretty-print the general-purpose register save area in a trapframe.
void trap_print_regs(trapframe *tf);

// Pretty-print the entire contents of a trapframe to the console.
void trap_print(trapframe *tf);

void trap(trapframe *tf) gcc_noreturn;
void trap_return(trapframe *tf) gcc_noreturn;

// Check for correct operation of trap handling.
void trap_check_kernel(void);
void trap_check_user(void);
void trap_check(void **argsp);

#endif /* PIOS_KERN_TRAP_H */
#endif /* LAB >= 1 */
