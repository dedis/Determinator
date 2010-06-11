#if LAB >= 4
/* 
 * Initial file system and file-based I/O support for the root process.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

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
