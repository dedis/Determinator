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

// Values of env_status in struct Env
#define ENV_FREE	0
#define ENV_RUNNABLE		1
#define ENV_NOT_RUNNABLE	2

struct Env {
	struct Trapframe env_tf;        // Saved registers
	LIST_ENTRY(Env) env_link;       // Free list
	u_int env_id;                   // Unique environment identifier
	u_int env_parent_id;            // env_id of this env's parent
	u_int env_status;               // Status of the environment
	Pde  *env_pgdir;                // Kernel virtual address of page dir
	u_int env_cr3;                  // Physical address of page dir

	// Lab 4 IPC
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;		// va at which to map received page
	u_int env_ipc_perm;		// perm of page mapping received

	// Lab 4 fault handling
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 scheduler counts
	u_int env_runs;			// number of times been env_run'ed
};

#endif // !_ENV_H_
