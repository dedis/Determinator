///LAB4

#include "libos.h"

////////////////////////////////////////////////////////////////////////////////
// ping pong test 
//    -- parent/child ping pong a number which increments at each step

void
pingpong_loop (char *prefix)
{
  // respond back to sending envid with value + 1 
  while (1) {
    u_int envid;
    u_int value = ipc_read (&envid);
    sys_cputs (prefix);
    print (" ipc_read value = ", value, "\n");
    ipc_send (envid, value + 1);
  }
}
     
void
child_pingpong ()
{
  sys_cputs (" - child ()\n");
  pingpong_loop (" - child");
}

void
pingpong ()
{
  int child_envid = spawn (&child_pingpong);
  print ("- parent (child_envid = ", child_envid, ")\n");
  ipc_send (child_envid, 0);
  pingpong_loop (" - parent");
}


////////////////////////////////////////////////////////////////////////////////
// sieve of eratosthenes
// see http://plan9.bell-labs.com/who/rsc/thread.html
// You can ignore almost all of the web page.  Half way down the page,
// there's a diagram that shows how the sieve works.


#define GOAL 100

void
child_sieve ()
{
  u_int prime;
  u_int child_envid;
  u_int from_envid;

  //  print ("--- child_sieve () ", sys_getenvid(), "\n");
  prime = ipc_read (&from_envid);
  print ("prime = ", prime, "\n");
  if (prime == 0) {
    sys_cputs ("ERR: prime == 0\n");
    exit ();
  }

  if (prime > GOAL) {
    sys_cputs ("GOAL reached...exiting\n");
    exit ();
  }

  child_envid = spawn (&child_sieve);
  while (1) {
    u_int value = ipc_read (&from_envid);
    ///print ("read value = ", value, "\n");
    if (value % prime)
      ipc_send (child_envid, value);
  }
}


void
sieve ()
{
  int i;
  int child_envid;

  child_envid = spawn (&child_sieve);
  for (i=2; i <= GOAL; i++)
    ipc_send (child_envid, i);
  exit ();
}


void
umain (int argc, char *argv[])
{
  sys_cputs ("(simple.c) Hello from user space..\n");

  //pingpong ();
  sieve ();
}


///END 
