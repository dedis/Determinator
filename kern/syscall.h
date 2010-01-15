#if LAB >= 3
#ifndef PIOS_KERN_SYSCALL_H
#define PIOS_KERN_SYSCALL_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/syscall.h>
#include <inc/trap.h>

void syscall(trapframe *tf);

#endif /* !PIOS_KERN_SYSCALL_H */
#endif /* LAB >= 3 */
