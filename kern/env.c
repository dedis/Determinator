///LAB2
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

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/printf.h>

struct Env *__envs = NULL;		/* All environments */
static struct Env_list env_free_list;	/* Free list */
struct Env *curenv = NULL;	        /* the current env */
///END
///LAB3

//
// Calculates the envid for env e.  
//
static u_int
mkenvid (struct Env *e)
{
  static u_long next_env_id = 0;
  // lower bits of envid hold e's position in the __envs array
  u_int idx = e - __envs;
  // high bits of envid hold an increasing number
  return (next_env_id++ << (1 + LOG2NENV)) | idx;
}

//
// Converts an envid to an env pointer.
//
// RETURNS
//   env pointer -- on success and sets *error = 0
//   NULL -- on failure, and sets *error = the error number
//
struct Env *
envid2env (u_int envid, int *error)
{
  struct Env *e = &__envs[envidx (envid)];
  if (e->env_status == ENV_FREE || e->env_id != envid) {
    *error = -E_BAD_ENV;
    return NULL;
  } else {
    *error = 0;
    return e;
  }
}

//
// Sets up the the stack and program binary for a user process.
//   The binary image is loaded at VA UTEXT.
//   One page for the stack is mapped at VA USTACKTOP - NBPG.
//
void
load_aout (struct Env* e, u_char *binary, u_int size)
{
  // Hint: 
  //  Use ppage_alloc, ppage_insert, pp2va and e->env_pgdir

///SOL3
  int i, r;
  struct Ppage *pp;

  // Allocate and map physical pages
  for (i = 0; i < size; i += NBPG) {
    if ((r = ppage_alloc (&pp)) < 0)
      panic ("load_aout: could not alloc page. Errno %d\n", r);
    bcopy (&binary[i], pp2va (pp), min (NBPG, size - i));
    if ((r = ppage_insert (e->env_pgdir, pp, UTEXT + i, PG_P|PG_W|PG_U)) < 0)
      panic ("load_aout: could not map page. Errno %d\n", r);
  }
  

  /* Give it a stack */
  if ((r = ppage_alloc (&pp)) < 0)
    panic ("load_aout: could not alloc page. Errno %d\n", r);
  if ((r = ppage_insert (e->env_pgdir, pp, USTACKTOP - NBPG, PG_P|PG_W|PG_U)) < 0)
    panic ("load_aout: could not map page. Errno %d\n", r);
///END
}







//
// Marks all envs in __envs as free and insert them into 
// the env_free_list.  Insert in reverse order, so that
// the first call to env_alloc() returns __envs[0].
//
void
env_init (void)
{
///SOL3
  int i;
  LIST_INIT (&env_free_list);
  for (i = NENV - 1; i >= 0; i--) {
    __envs[i].env_status = ENV_FREE;    
    LIST_INSERT_HEAD (&env_free_list, &__envs[i], env_link);
  }
///END
}



//
// Initializes the kernel virtual memory layout for environment e.
//
// Allocates a page directory and initializes it.  Sets
// e->env_cr3 and e->env_pgdir accordingly.
//
// RETURNS
//   0 -- on sucess
//   <0 -- otherwise 
//
static int
env_setup_vm (struct Env *e)
{
  // Hint:

  int i, r;
  struct Ppage *pp1 = NULL;

  /* Allocate a page for the page directory */
  if ((r = ppage_alloc (&pp1)) < 0)
    return r;
  // Hint:
  //    - The VA space of all envs is identical above UTOP
  //      (except at VPT and UVPT) 
  //    - Use p0pgdir_boot
  //    - Do not make any calls to ppage_alloc 
  //    - Note: pp_refcnt is not maintained for physical pages mapped above UTOP.

///SOL3
  e->env_cr3 = pp2pa (pp1);
  e->env_pgdir = pp2va (pp1);
  bzero (e->env_pgdir, NBPG);

  /* The VA space of all envs is identical above UTOP...*/
  StaticAssert (UTOP % NBPD == 0);
  for (i = PDENO (UTOP); i < NLPG; i++)
    e->env_pgdir[i] = p0pgdir_boot[i];

///END

  /* ...except at VPT and UVPT.  These map the env's own page table */  
  e->env_pgdir[PDENO(VPT)]   = e->env_cr3 | PG_P | PG_W;
  e->env_pgdir[PDENO(UVPT)]  = e->env_cr3 | PG_P | PG_U;

  /* success */
  return 0;
}

//
// Allocates and initializes a new env.
//
// RETURNS
//   0 -- on success, sets *new to point at the new env 
//   <0 -- on failure
//
int
env_alloc (struct Env **new, u_int parent_id)
{
  int r;
  struct Env *e;

  if (!(e = LIST_FIRST (&env_free_list)))
    return -E_NO_FREE_ENV;

  if ((r = env_setup_vm (e)) < 0)
    return r;

  e->env_id = mkenvid (e);
  e->env_parent_id = parent_id;
  e->env_status = ENV_OK;

  /* Set initial values of registers */
  /*  (lower 2 bits of the seg regs is the RPL -- 3 means user process) */
  e->env_tf.tf_ds = GD_UD | 3;
  e->env_tf.tf_es = GD_UD | 3;
  e->env_tf.tf_ss = GD_UD | 3;
  e->env_tf.tf_esp = USTACKTOP;
  e->env_tf.tf_cs = GD_UT | 3;
  // You also need to set tf_eip to the correct value.
  // Hint: see load_aout

///SOL3
  e->env_tf.tf_eip = UTEXT + 0x20; // right past a.out header
  e->env_tf.tf_eflags = FL_IF; // interrupts enabled
///ELSE
  e->env_tf.tf_eflags = 0;
///END

  e->env_ipc_blocked = 0;
  e->env_ipc_value = 0;
  e->env_ipc_from = 0;

  e->env_pgfault_handler = 0;
  e->env_xstacktop = 0;

  /* commit the allocation */
  LIST_REMOVE (e, env_link);
  *new = e;

  return 0; /* success */
}




//
// Allocates a new env and loads the a.out binary into it.
//  - new env's  parent env id is 0
void
env_create (u_char *binary, int size)
{
////SOL3
  int r;
  struct Env *e;
  if ((r = env_alloc (&e, 0)) < 0)
    panic ("env_create: could not allocate env.  Error %d\n", r);
  load_aout (e, binary, size);
////END
}


//
// Frees env e and all memory it uses.
// 
void
env_free (struct Env *e)
{
///SOL3
  Pte *pt;
  u_int pdeno, pteno;

  StaticAssert ( (UTOP % NBPD) == 0);
  /* Flush all pages */
  for (pdeno = 0; pdeno < PDENO (UTOP); pdeno++) {
    if (!(e->env_pgdir[pdeno] & PG_P))
      continue;
    pt = ptov (e->env_pgdir[pdeno] & ~PGMASK);
    for (pteno = 0; pteno < NLPG; pteno++)
      if (pt[pteno] & PG_P)
	ppage_remove (e->env_pgdir, (pdeno << PDSHIFT) | (pteno << PGSHIFT));
    ppage_free (kva2pp ((u_long) pt));
  }
  ppage_free (kva2pp ((u_long) (e->env_pgdir)));
///END 

  // For lab 3, env_free () doesn't really do
  // anything (except leak memory).  We'll fix
  // this in later labs.
  e->env_status = ENV_FREE;
  LIST_INSERT_HEAD (&env_free_list, e, env_link);
}


//
// Frees env e.  And schedules a new env
// if e is the current env.
//
void
env_destroy (struct Env *e) 
{
  env_free (e);
  if (curenv == e) {
    curenv = NULL;
    yield ();
  }
}


//
// Restores the register values in the Trapframe
//  (does not return)
//
void
env_pop_tf (struct Trapframe *tf)
{
#if 0
  printf (" --> %d 0x%x\n", envidx (curenv->env_id), tf->tf_eip);
#endif

  asm volatile ("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		:: "g" (tf) : "memory");
  panic ("iret failed");  /* mostly to placate the compiler */
}


//
// Context switch from curenv to env e.
// Note: is this is the first call to env_run, curenv is NULL.
//  (This function does not return.)
void
env_run (struct Env *e)
{
  // step 1: save register state of curenv
  // step 2: set curenv
  // step 3: use lcr3
  // step 4: use env_pop_tf ()

  // Hint: Skip step 1 until exercise 4.  You don't
  // need it for exercise 1, and in exercise 4 you'll better
  // understand what you need to do.
///SOL3
  // save register state of currently executing env
  if (curenv)
    curenv->env_tf = *utf;
  curenv = e;
  // switch to e's addressing context
  lcr3 (e->env_cr3);
  // restore e's register state
  env_pop_tf (&e->env_tf);
///END
}


///END
