///BEGIN 3
#include <kern/inc/env.h>
#include <kern/inc/pmap.h>
#include <kern/inc/picirq.h>
#include <kern/inc/printf.h>

void
clock ()
{
  printf ("*");
}


/* round-robin scheduling */
void
yield (void)
{
///BEGIN 5
  /* marks current position in the round-robin sweep */
  static int sched_idx = 0;
  int start = sched_idx;

  do {
    sched_idx++;
    sched_idx %= NENV; 
    // skip the idle env
    if (sched_idx && __envs[sched_idx].env_status == ENV_OK)
      env_run (&__envs[sched_idx]);
    // we fall out of the loop when we've
    // checked all envs except the idle env
  } while (start != sched_idx);

  // idle env must always be runnable
///END
  assert (__envs[0].env_status == ENV_OK);
  env_run (&__envs[0]);
}
///END
