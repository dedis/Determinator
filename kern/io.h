#if LAB >= 4
/* See COPYRIGHT for copyright information. */
#ifndef PIOS_KERN_IO_H
#define PIOS_KERN_IO_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/gcc.h>
#include <inc/io.h>

struct trapframe;

void io_init(void);
void io_sleep(struct trapframe *tf) gcc_noreturn;
void io_check(void);

#endif /* !PIOS_KERN_IO_H */
#endif // LAB >= 4
