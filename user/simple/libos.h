///LAB4 
#ifndef _LIBOS_H_
#define _LIBOS_H_

#define INLINE_SYSCALLS

#include <inc/asm.h>
#include <inc/syscall.h>
#include <inc/env.h>
#include <inc/pmap.h>


#define PG_COW 0x400           /* page is copy-on-write */

void ipc_send (u_int, u_int);
u_int ipc_read (u_int *);
void print (char *, int, char *);
void printx (char *, int, char *);
void xpanic (char *, int, char *);
void exit ();
int spawn (void (*) (void));
void umain ();

int exec (char *name,...);


extern struct Env *__env;
extern struct Ppage __ppages[];
extern struct Env __envs[];

struct a_out_hdr {
     unsigned long	a_midmag;	/* flags<<26 | mid<<16 | magic */
     unsigned long	a_text;		/* text segment size */
     unsigned long	a_data;		/* initialized data size */
     unsigned long	a_bss;		/* uninitialized data size */
     unsigned long	a_syms;		/* symbol table size */
     unsigned long	a_entry;	/* entry point */
     unsigned long	a_trsize;	/* text relocation size */
     unsigned long	a_drsize;	/* data relocation size */
};

#endif /* _LIBOS_H_ */
///END 
