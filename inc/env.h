/* See COPYRIGHT for copyright information. */

#ifndef _ENV_H_
#define _ENV_H_

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/trap.h>
#include <inc/mmu.h> 

#define LOG2NENV	10
#define NENV		(1<<LOG2NENV)
#define ENVX(envid)	((envid) & (NENV - 1))

/* Values of env_status in struct Env */
#define ENV_FREE	0
#define ENV_RUNNABLE		1
#define ENV_NOTRUNNABLE	2

struct Env {
	struct Trapframe env_tf;        /* Saved registers */
	LIST_ENTRY(Env) env_link;       /* Free list */
	u_int env_id;                   /* Unique environment identifier */
	u_int env_parent_id;            /* env_id of this env's parent */
	u_int env_status;               /* Status of the environment */
	Pde  *env_pgdir;                /* Kernel virtual address of page dir */
	u_int env_cr3;                  /* Physical address of page dir */

	/* (below here: not used in lab 3) */
	/* IPC state */
	u_int env_ipc_value;            /* data value sent to us */ 
	u_int env_ipc_from;             /* envid of the sender */  
	u_int env_ipc_blocked;          /* true(1) or false(0) */
	/* env_ipc_blocked is true if and only if
	   env_ipc_value and env_ipc_from are valid */

	u_int env_pgfault_handler;      /* page fault state */
	u_int env_xstacktop;            /* top of exception stack */
};

#endif /* !_ENV_H_ */
