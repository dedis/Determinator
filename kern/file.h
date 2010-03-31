#if LAB >= 4
/* See COPYRIGHT for copyright information. */
#ifndef PIOS_KERN_FILE_H
#define PIOS_KERN_FILE_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/file.h>

struct proc;
struct trapframe;

void file_init(void);
void file_initroot(struct proc *root);
void file_io(struct trapframe *tf) gcc_noreturn;
void file_wakeroot(void);

#endif /* !PIOS_KERN_FILE_H */
#endif // LAB >= 4
