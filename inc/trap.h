#if LAB >= 2	// pmap.c must allocate Envs, which contain Trapframes
#ifndef _TRAP_H_
#define _TRAP_H_

/* these are processor defined */ 
#define T_DIVIDE     0    /* divide error */
#define T_DEBUG      1    /* debug exception */
#define T_NMI        2    /* non-maskable interrupt */
#define T_BRKPT      3    /* breakpoint */
#define T_OFLOW      4    /* overflow */
#define T_BOUND      5    /* bounds check */
#define T_ILLOP      6    /* illegal opcode */
#define T_DEVICE     7    /* device not available */ 
#define T_DBLFLT     8    /* double fault */
                          /* 9 is reserved */
#define T_TSS       10    /* invalid task switch segment */
#define T_SEGNP     11    /* segment not present */
#define T_STACK     12    /* stack exception */
#define T_GPFLT     13    /* genernal protection fault */
#define T_PGFLT     14    /* page fault */
                          /* 15 is reserved */
#define T_FPERR     16    /* floating point error */
#define T_ALIGN     17    /* aligment check */
#define T_MCHK      18    /* machine check */

/* These are arbitrarily chosen, but with care not to overlap
 * processor defined exceptions or interrupt vectors.
 */
#define T_SYSCALL   0x30 /* system call */
#define T_DEFAULT   500  /* catchall */

#ifndef __ASSEMBLER__

#include <inc/types.h>

struct Trapframe {
	u_int tf_edi;
	u_int tf_esi;
	u_int tf_ebp;
	u_int tf_oesp;		/* Useless */
	u_int tf_ebx;
	u_int tf_edx;
	u_int tf_ecx;
	u_int tf_eax;
	u_short tf_es;
	u_int : 0;
	u_short tf_ds;
	u_int : 0;
	u_int tf_trapno;
	/* below here defined by x86 hardware */
	u_int tf_err;
	u_int tf_eip;
	u_short tf_cs;
	u_int : 0;
	u_int tf_eflags;
	/* below only when crossing rings(e.g. user to kernel) */
	u_int tf_esp;
	u_short tf_ss;
	u_int : 0;
};

#endif /* !__ASSEMBLER__ */

#endif /* _TRAP_H_ */
#endif /* LAB >= 2 */
