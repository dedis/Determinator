#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#ifndef _ENV_H_
#define _ENV_H_

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/trap.h>
#include <inc/pmap.h>

#define LOG2NENV		10
#define NENV			(1<<LOG2NENV)
#define ENVX(envid)		((envid) & (NENV - 1))

// Values of env_status in struct Env
#define ENV_FREE		0
#define ENV_RUNNABLE		1
#define ENV_NOT_RUNNABLE	2

struct Env {
	struct Trapframe env_tf;        // Saved registers
	LIST_ENTRY(Env) env_link;       // Free list link pointers
	u_int env_id;                   // Unique environment identifier
	u_int env_parent_id;            // env_id of this env's parent
	u_int env_status;               // Status of the environment

	// Address space
	Pde  *env_pgdir;                // Kernel virtual address of page dir
	u_int env_cr3;                  // Physical address of page dir

	// Exception handling
	u_int env_pgfault_entry;      // page fault state

#if LAB >= 4
	// Lab 4 IPC
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;		// va at which to map received page
	u_int env_ipc_perm;		// perm of page mapping received

#if LAB >= 6
	// Lab 6 scheduler counts
	u_int env_runs;			// number of times been env_run'ed
#endif // LAB >= 6
#endif // LAB >= 4
};

#endif // !_ENV_H_
#endif // LAB >= 3
