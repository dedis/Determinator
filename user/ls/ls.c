///LAB4

#include "../simple/libos.h"

void
umain (int argc, char *argv[])
{
  int i;
  print ("ls.c::umain(): envid ", sys_getenvid(), "\n");
  print ("  argc ", argc, "\n");
  for (i = 0; i < argc; i++) {
    print ("  argv[", i, "] = ");
    sys_cputs (argv[i]);
    sys_cputs ("\n");
  }

}

///END
