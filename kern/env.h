/* See COPYRIGHT for copyright information. */

#ifndef _KERN_ENV_H_
#define _KERN_ENV_H_

#include <inc/env.h>

LIST_HEAD(Env_list, Env);
extern struct Env *__envs;		/* All environments */
extern struct Env *curenv;	        /* the current env */

void env_init (void);
int env_alloc (struct Env **e, u_int parent_id);
void env_free (struct Env *);
void env_create (u_char *binary, int size);
void env_destroy (struct Env *e);

struct Env *envid2env (u_int envid, int *error);
void load_aout (struct Env* e, u_char *binary, u_int size);
void env_run (struct Env *e);
void env_pop_tf (struct Trapframe *tf);

#endif /* !_KERN_ENV_H_ */
