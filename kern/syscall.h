#if LAB >= 3
#ifndef PIOS_KERN_SYSCALL_H
#define PIOS_KERN_SYSCALL_H
#ifndef PIOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/syscall.h>

int32_t syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

#endif /* !PIOS_KERN_SYSCALL_H */
#endif /* LAB >= 3 */
