#if LAB >= 2
/*
 * System call handling definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the xv6 instructional operating system from MIT.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_KERN_SYSCALL_H
#define PIOS_KERN_SYSCALL_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/syscall.h>
#include <inc/trap.h>

void syscall(trapframe *tf);

#endif /* !PIOS_KERN_SYSCALL_H */
#endif /* LAB >= 2 */
