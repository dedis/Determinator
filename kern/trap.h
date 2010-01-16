#if LAB >= 1
// Trap handling module definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_TRAP_H
#define PIOS_KERN_TRAP_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>
#include <inc/gcc.h>


// Initialize the trap-handling module and the processor's IDT.
void trap_init(void);

// Return a string constant describing a given trap number,
// or "(unknown trap)" if not known.
const char *trap_name(int trapno);

// Pretty-print the general-purpose register save area in a trapframe.
void trap_print_regs(pushregs *regs);

// Pretty-print the entire contents of a trapframe to the console.
void trap_print(trapframe *tf);

void trap(trapframe *tf) gcc_noreturn;
void trap_return(trapframe *tf) gcc_noreturn;


#endif /* PIOS_KERN_TRAP_H */
#endif /* LAB >= 1 */
