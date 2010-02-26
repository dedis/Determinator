#if LAB >= 4
/* See COPYRIGHT for copyright information. */
#ifndef PIOS_KERN_FILE_H
#define PIOS_KERN_FILE_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/file.h>

struct proc;

void file_init(struct proc *root);

#endif /* !PIOS_KERN_FILE_H */
#endif // LAB >= 4
