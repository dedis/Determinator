/* See COPYRIGHT for copyright information. */

///LAB3

#ifndef _KERN_TRAP_H_
#define _KERN_TRAP_H_

#include <inc/trap.h>


/* The user trap frame is always at the top of the kernel stack */
#define utf ((struct Trapframe *) (KSTACKTOP-sizeof(struct Trapframe)))

void page_fault_handler (struct Trapframe *);
void backtrace (struct Trapframe *);

extern u_int page_fault_mode;

/* Trap page faults in a particular line of code. */
#define pfm(pfm, code...)			\
{						\
  int __m_m = page_fault_mode;			\
  page_fault_mode = pfm;			\
  code;						\
  page_fault_mode = __m_m;			\
}


#endif /* !__ASSEMBLER__ */

#endif /* _KERN_TRAP_H_ */
///END
