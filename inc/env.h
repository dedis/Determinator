
/*
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */

#ifndef _ENV_H_
#define _ENV_H_

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/trap.h>
#include <inc/mmu.h> 

#define LOG2NENV 10
#define NENV (1<<LOG2NENV)
#define envidx(envid) ((envid) & (NENV - 1))

/* Values of env_status in struct Env */
#define ENV_FREE 0
#define ENV_OK 1
#define ENV_NOTRUNNABLE 2

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
  u_int env_ipc_blocked;          /* true (1) or false (0) */
  /* if env_ipc_blocked is true, if and only if
     env_ipc_value and env_ipc_from are valid */

  u_int env_pgfault_handler;      /* page fault state */
  u_int env_xstacktop;            /* top of exception stack */
};

#endif /* !_ENV_H_ */
