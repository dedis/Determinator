///BEGIN 4

#include <inc/stdarg.h>
#include "libos.h"
#include "../../tools/mkimg/mkimg.h"

// This code does not allocate or initialize its BSS (section for
// uninitialized data).  Thus, we force __env to be located in the
// data section by initializing it.
//
struct Env *__env = 0;

///BEGIN 200
/* XXX zero the BSS ??

 * The -N in the LDFLAGS in GNUmakefile.global instructs the linker to
 * place the text and data section adjacent to one another in the
 * binary.  The bintoc program then pads the binary with zeros out to
 * the next page.  As a result there is often more than enough zeros
 * in the bintoc output for use as the BSS.
 *
 * So, we can usually get away with out explicitely setuping up the
 * BSS (if the BSS isn't too big and there is enough space left after
 * the text+data). Though, it is a lurking bug.  We solve the problem
 * above by forcing __env to be located in the data section.
 *
 * We should fix this for real sometime. We could do the following:
 *
 * #if 0
 * void
 * setup_bss ()
 * {
 *   int ret;
 *   u_int va;  
 *   for (va = PGROUNDUP(&edata); va < &end; va += NBPG) {
 *     ret = sys_mem_alloc (0, va, PG_P|PG_U|PG_W);
 *     if (ret < 0)
 *       xpanic ("Couldn't alloc bss, ret ", ret, "\n");
 *   }
 * 
 *   bzero (&edata, &end - &edata);
 * }
 * #endif
 *
 * Or we could demand allocate the BSS in the user page fault handler.
 * Though, on startup, bzero (&edata, PGROUNDUP(&edata))
 */
///END


extern void asm_pgfault_handler ();

unsigned int
ntohl (unsigned int x)
{
  unsigned char *s = (unsigned char *)&x;
  return (unsigned int)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
}

int
strcmp (const char *s1, const char *s2)
{
  /* this code is from FreeBSD's libkern */
  while (*s1 == *s2++)
    if (*s1++ == 0)
      return (0);
  return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

size_t
strlen(const char *str)
{
  /* this code is from FreeBSD's libkern */
  const char *s;
  for (s = str; *s; ++s);
  return (s - str);
}

char *
strcpy(char *to, const char *from)
{
  /* this code is from FreeBSD's libkern */
  char *save = to;
  for (; (*to = *from) != 0; ++from, ++to);
  return(save);
}



void
putx (u_int number)
{
  char *hex[] = {"0","1","2","3","4","5","6","7",
		"8","9","a","b","c","d","e","f"};
  int shift;
  sys_cputs ("0x");
  for (shift = 28; shift >= 0 ; shift -= 4) {
    u_char digit = (number >> shift) & 0xf;
    sys_cputs (hex[digit]);
  }
}

/* poor man's printf */
void
print (char *prefix, int number, char *suffix)
{
  sys_cputs (prefix);
  sys_cputu (number);
  sys_cputs (suffix);
}

void
printx (char *prefix, int number, char *suffix)
{
  sys_cputs (prefix);
  putx (number);
  sys_cputs (suffix);
}

void
exit ()
{
  print ("envid ", sys_getenvid(), " exiting..\n");
  sys_env_destroy ();
  /* NOT REACHED */
  sys_cputs ("FATAL: sys_env_destroy failed.\n");
  while (1)
    ;
}

void
xpanic (char *prefix, int number, char *suffix)
{
  print (prefix, number, suffix);
  exit ();
}


// like bcopy (but doesn't handle overlapping src/dst)
void
xbcopy (const void *src, void *dst, size_t len)
{
  void *max = dst + len;
  // copy 4bytes at a time while possible
  while (dst + 3 < max)
    *((u_int *)dst)++ = *((u_int *)src)++;
  // finish remaining 0-3 bytes
  while (dst < max)
    *(char *)dst++ = *(char *)src++;
}


void
handle_cow_fault (u_int va, Pte pte)
{
///BEGIN 5

#define COW_TEMP_PG  10*NBPD
  u_int ret;
  u_int orig_ppn = pte >> PGSHIFT;
  u_int refcnt = __ppages[orig_ppn].pp_refcnt;
#if 0
  printx ("COW va=", va, "\n");
#endif

  if (refcnt == 1) {
#if 0
    sys_cputs ("COW refcnt = 1\n"); 
#endif
    ret = sys_mod_perms (va, PG_W, PG_COW);
    if (ret < 0)
      xpanic ("Couldn't update COW pte, va ", va, "\n");
  } else {
#if 0
    sys_cputs ("COW refcnt ! 1\n"); 
#endif

    // 1. allocate a new physical page, mapped at COW_TEMP_PG
    if (sys_mem_alloc (0, COW_TEMP_PG, PG_U|PG_W|PG_P) < 0)
      xpanic ("Couldn't alloc new cow page ", va, "\n");

    // 2. copy from the COW page to the new page
    xbcopy ((void *)(va & ~PGMASK), (void *)(COW_TEMP_PG), NBPG);

    // 3. replace old physical page with new physical page at fault address
    if (sys_mem_remap (COW_TEMP_PG, 0, va & ~PGMASK, PG_W|PG_U|PG_P) < 0)
      xpanic ("couldn't re-map new cow page, va ", va, "\n");

    // 4. delete double mapping
    if (sys_mem_unmap (0, COW_TEMP_PG) < 0)
      xpanic ("couldn't unmap temp cow page, va ", va, "\n");
  }
///END
}

void
pgfault_handler (u_int va, u_int err, u_int esp, u_int eflags, u_int eip)
{
#if 0
  printx ("ENVID ", sys_getenvid(), " "); 
  printx ("PGFLT [va ", va, ""); 
  printx (", err ", err, ""); 
  printx (", esp ", esp, ""); 
  printx (", eflags ", eflags, ""); 
  printx (", eip ", eip, "]\n"); 
#endif



///BEGIN 5
  /* copy-on-write fault */
  if ((vpd[PDENO(va)] & (PG_P|PG_U)) == (PG_P|PG_U)) {
    Pte pte = vpt[PGNO(va)];
    if ((pte&PG_COW) && (err&FEC_WR)) {
      handle_cow_fault (va, pte);
    } else {
      sys_cputs ("NOT COW!\n");
    }
  } else {
    printx ("[ENVID ", sys_getenvid(), "]\n"); 
    printx ("Fatal user fault for va ", va, "");
    printx (" eip ", eip, ".  Exiting.\n");
    exit ();
  }
///END
}


// Sends 'value' to 'target_envid'.
// This function does not return until the value
// has been successfully sent.  And it should xpanic
// on any error other than -E_IPC_BLOCK.
// Hint: use sys_yield() to be CPU friendly.
void
ipc_send (u_int target_envid, u_int value)
{
///BEGIN 5
  int ret;
#if 0
  print ("IPC_SEND value ", value, ",");
  print (" from (us) ", sys_getenvid(), ",");
  print (" to ", target_envid, "\n");
#endif

 again:
  ret = sys_ipc_send (target_envid, value);
  if (ret == -E_IPC_BLOCKED) {
    sys_yield ();
    goto again;
  } else if (ret < 0)
    xpanic ("sys_ipc_send failed, ret ", ret, "\n");
///END
}



// Loops until an incoming IPC is received.
//   - returns the IPC value
//   - *from_envid is set to the sender
//
// Hint: loop something like this:
//    while (!*(volatile int *)&(__env->env_ipc_blocked))
//           sys_yield ();
//
// The volatile tells the compiler NOT to cache __env->env_ipc_blocked
// in a register.  Otherwise, we'll never see when that field is set.
//
// Make sure there are no race conditions in your code.  You must
// guarantee that each IPC sent is received exactly once.
//
u_int
ipc_read (u_int *from_envid)
{
///BEGIN 5
  u_int value;

  /* spin until ipc is blocked, then there is data ready */
  while (!*(volatile int *)&(__env->env_ipc_blocked))
    sys_yield ();

  *from_envid = __env->env_ipc_from; // MUST be before sys_ipc_unblock!
  value = __env->env_ipc_value;      // ditto

#if 0
  print ("IPC_READ value ", value, ",");
  print (" from ", *from_envid, ",");
  print (" to (us) ", sys_getenvid(), "\n");
#endif
  sys_ipc_unblock ();  
  return value;
///END
}

// an example of vpt[] and vpd[] 
// to dump the contents of the current process's 
// pgdir and page table.
//
void
dump_va_space ()
{
  int i;
  u_int va;
  Pde pde;
  Pte pte;

  // 0x800cbc, va 0x801000

  // try both of these..
  //u_int va_lim = 0xffffffff;
  u_int va_lim = UTOP; 
  for (va = 0; va < va_lim; ) {
    pde = vpd[PDENO(va)];

    if (!(pde & PG_P)) {
      va += NBPD;
      continue;
    }

    sys_cputs ("PGDIR[");
    sys_cputu (PDENO(va));
    printx ("] = ", pde, "\n");

    for (i = 0; i < NLPG; i++, va += NBPG) {
      pte = vpt[PGNO(va)];
      if (pte & PG_P) {
	sys_cputs ("    PGTBL[");
	sys_cputu (i);
	printx ("] = ", pte, "\n");
      }
    }
  }
}

// fork()
//
// RETURNS
//   on success
//      child's envid to the parent
//      0 to the child
//   on error
//      just do whatever (call xpanic, return <0) doesn't matter
//
// Hint: set __env correctly in the child.
//

int
fork (void)
{
///BEGIN 5
  u_int va, j, ret;
  int child_envid = sys_env_alloc (1, 0);
  if (child_envid == 0) {
    // child
    __env = &__envs[envidx(sys_getenvid ())];
  }
  //print ("sys_env_alloc child_envid = ", child_envid, "\n");
  if (child_envid <= 0) {
    return child_envid;
  }
  // mark parent address space COW and insert pages into child with
  // COW set.
  for (va = 0; va < UTOP; va += NBPD) {
    if (vpd[PDENO(va)] & PG_P) {
      Pte *pt = &vpt[PGNO(va)];
      for (j = 0; j < NLPG; j++) {
	if (pt[j] & (PG_P|PG_W)) {
	  Pte pte = pt[j];
	  //printx ("va = ", va + j*NBPG, "\n");
	  if (va + j*NBPG == UXSTACKTOP - NBPG) {
	    ret = sys_mem_alloc (child_envid, UXSTACKTOP - NBPG, PG_U|PG_W|PG_P);
	    if (ret < 0)
	      xpanic ("Couldn't alloc exception stack, ret ", ret, "\n");
	  } else {
	    pte &= ~PG_W;
	    pte |= PG_COW;
	    ret = sys_mem_remap (va + j*NBPG, child_envid, va + j*NBPG, PG_COW|PG_U|PG_P);
	    if (ret < 0)
	      xpanic ("sys_mem_remap: ", ret, "\n");

	    ret = sys_mod_perms (va + j*NBPG, PG_COW, PG_W);
	    if (ret < 0)
	      xpanic ("sys_mod_perms: ", ret, "\n");
	  }
	}
      }
    }
  }

  ret = sys_set_env_status (child_envid, ENV_OK);
  if (ret < 0)
    xpanic ("sys_set_env_status: ", ret, "\n");
  return child_envid;
///END
}

#define READ_TEMP_PG  (11*NBPD)

// +-------------+
// |   strings   |
// +-------------+
// |  argv array |
// | of char *'s |<--
// +-------------+  |
// |   argv ptr  |--/ 
// |   argc      |
// +-------------+

u_int
exec_build_args (char *va, char *argv[])
{
  char *va_orig = va;
  int i, argc;

  // build strings
  for (argc = 0; argv[argc]; argc++) {
    va -= strlen (argv[argc]) + 1;
    // argv[#] now points into "strings"
    argv[argc] = strcpy (va, argv[argc]);
  }

  // build argv array
  (u_int)va &= ~0x3;   // word aligned (not strictly necessary)
  // adjustment b/c 'va' will be mapped at USTACKTOP in child
  for (i = argc - 1; i >= 0; i--)
    *--(u_int *)va = argv[i] - va_orig + USTACKTOP;

  // push argv and argc so they appear as umain(...)'s arguments
  argv = (char **)(va - va_orig + USTACKTOP);
  *--(u_int *)va = (u_int)argv;
  *--(u_int *)va = argc;

  // return how much space was used
  return va_orig - va;
}



int
exec_internal (u_int diskno, u_int blockno, char *argv[])
{
  struct a_out_hdr *hdr;
  u_int i, ret, dest, nbytes;
  int child_envid;
  u_int sz;


  if (ret = sys_mem_alloc (0, READ_TEMP_PG, PG_U|PG_W|PG_P) < 0)
    xpanic ("sys_mem_alloc: ", ret, "\n");

  sz = exec_build_args ((char *)(READ_TEMP_PG + NBPG), argv);

  child_envid = sys_env_alloc (0, USTACKTOP - sz);
  if (child_envid < 0)
    return child_envid;
  
  if (ret = sys_mem_remap (READ_TEMP_PG, child_envid, USTACKTOP - NBPG, PG_W|PG_U|PG_P) < 0)
    xpanic ("sys_mem_remap: ", ret, "\n");

  if (ret = sys_disk_read (diskno, blockno, READ_TEMP_PG) < 0)
    xpanic ("Couldn't read disk, blockno ", blockno, "\n");

  hdr = (struct a_out_hdr *)READ_TEMP_PG;
#if 0
  printx ("text ", hdr->a_text, "\n");
  printx ("data ", hdr->a_data, "\n");
  printx ("bss  ", hdr->a_bss, "\n");
  printx ("entr ", hdr->a_entry, "\n");
#endif

  nbytes = sizeof (struct a_out_hdr) + hdr->a_text + hdr->a_data;
  dest = hdr->a_entry - sizeof (struct a_out_hdr);

  for (i = 0; i < nbytes; i += NBPG, blockno += 1) {
    if (ret = sys_disk_read (diskno, blockno, READ_TEMP_PG) < 0)
      xpanic ("Couldn't read disk, blockno ", blockno, "\n");
    if (ret = sys_mem_remap (READ_TEMP_PG, child_envid, dest + i, PG_W|PG_U|PG_P) < 0)
      xpanic ("sys_mem_remap: ", ret, "\n");
  }

  // XXX BSS
  if (ret = sys_set_env_status (child_envid, ENV_OK) < 0)
    xpanic ("sys_set_env_status: ", ret, "\n");

  // the parent exits, to simulate that the current process
  // is replaced by the new process
  exit ();
  /* NOT REACHED */
}


// Looks up 'name' in the disk image created by mkimg.  
//
//RETURNS:
//   block offset of 'name' in the disk
//   <0 -- on error (ie. 'name' does not exist) 
int
fs_lookup (char *name)
{
  int ret, i;
  struct toc_entry *ent;

  if (ret = sys_disk_read (1, 0, READ_TEMP_PG) < 0)
    xpanic ("Couldn't read disk table of contents, ret ", ret, "\n");

  ent = (struct toc_entry *)READ_TEMP_PG;
  for (i = 0; i < NBPG/sizeof(*ent); i++) {
    if (strcmp (name, ent[i].name) == 0) {
      return ntohl(ent[i].offset);
      break;
    }
  }
  return -1;
}

int
exec (char *name,...)
{
  int ret, offset;

  ret = fs_lookup (name);
  if (ret >= 0) {
    offset = ret;
    ret = exec_internal (1, offset, &name);
  }

  return ret;
}



// spawn a child process and return its envid
//
int
spawn (void (*child_fn) (void))
{
  int ret = fork ();
  if (ret < 0) {
    xpanic ("ERR: sys_env_fork, ret = ", ret, "\n");
  } else if (ret == 0) { // child see a 0 return value
    (*child_fn) ();
    sys_cputs ("ERR: spawn: child returned\n");
    exit ();
  }

  return ret; // return child's envid to parent
}



///BEGIN 200
#if 0
///END
void
setup_xstack ()
{
  sys_cputs ("setup_xstack: commented out until sys_mem_alloc is implemented\n");
  while (1)
    ;

#if 0
  int ret;
  // allocate exception stack
  ret = sys_mem_alloc (0, UXSTACKTOP - NBPG, PG_P|PG_U|PG_W);
  if (ret < 0)
    xpanic ("Couldn't alloc exception stack, ret ", ret, "\n");
  
  // setup pgfault handler
  sys_set_pgfault_handler ((u_int)&asm_pgfault_handler, UXSTACKTOP);
#endif
}
///BEGIN 200
#endif


void
setup_xstack ()
{
  int ret;
  // allocate exception stack
  ret = sys_mem_alloc (0, UXSTACKTOP - NBPG, PG_P|PG_U|PG_W);
  if (ret < 0)
    xpanic ("Couldn't alloc exception stack, ret ", ret, "\n");
  
  // setup pgfault handler
  sys_set_pgfault_handler ((u_int)&asm_pgfault_handler, UXSTACKTOP);
}
///END




void
exercise2 (void)
{
  int ret;
  int envid = sys_getenvid ();
  sys_cputs ("Hello from user space\n");
  sys_cputs ("My envid is: ");
  sys_cputu (envid);
  sys_cputs ("\n");
  if (envid != 2049)
    sys_cputs (" *** ERROR: expected 2049\n");


///BEGIN 200
#if 0
///END
  // now test bad sys_call number
  ret = sys_call (100, 0, 0, 0);
  if (ret != -E_INVAL)
    sys_cputs (" *** ERROR: expected -E_INVAL for bad syscall number\n");
///BEGIN 200
#endif
///END


  while (1)
    ;
}

///BEGIN 200
#if 0
///END
void
exercise3 (void)
{
  extern void pingpong_loop (char *);

  int envid = sys_getenvid ();
  if (envid != 2049) {
    ipc_send (2049, 0);
    pingpong_loop ("2nd init process");
  } else {
    pingpong_loop ("1st init process");
  }
}
///BEGIN 200
#endif
///END

void
exercise4_test1 (void)
{
  sys_cputs ((char *)0);
}

void
exercise4_test2 (void)
{
  sys_cputs ((char *)ULIM);
}

void
exercise4_test3 (void)
{
  sys_cputs ((char *)0xffffffff);
}

void
exercise5_test1 (void)
{
  setup_xstack ();
  // fault writing to VA 0 
  *(char *)0 = 'A';
}

void
exercise5_test2 (void)
{
  setup_xstack ();
  // fault writing to VA 0xff 
  *(char *)0xff = 'A';
}

//
//RETURNS: 1 + 2 + ... + depth
//
u_int
exercise5_test3_helper (u_int depth)
{
  if (depth == 1)
    return 1;
  else
    return depth + exercise5_test3_helper (depth - 1);
}

// When a user process starts the kernel has only allocatd
// one page (4096bytes) of stack for it.  We quickly 
// consume this with a deeply recursive call.  The 
// *USER'S* page fault handler routine should call
// sys_mem_alloc to add another page to the stack, as needed.
//
// You might as well add some policy to your page fault handler.  For
// instance, if the stack exceeds some maximum size the page fault
// handler might want to assume that there has been a programming
// error and simply exit.
//

void
exercise5_test3 (void)
{
  setup_xstack ();
  u_int x = 4000;
  u_int i = exercise5_test3_helper (x);
  if (i != x*(x + 1)/2)
    xpanic ("exercise5_test3 failed, i=", i, "\n");
}

// The kernel must be careful when putting the trap time values on 
// the user exception stack.  The user exception stack pointer is a value
// that comes from the user process, so the kernel can't trust that it 
// points at valid memory.  In fact, here the user process maliciously
// supplies a bad exception stack pointer. 
//
// The kernel should use the page_fault_mode/pfm() technique when it
// puts the trap time values on the user exception stack.  Otherwise
// the kernel might crash or corrupt memory.  
//
// Do not implement protections in sys_set_pgfault_handler, the MUST
// be done dynamically in the kernel's page fault handler.
//

void
exercise5_test4 (void)
{
  // if the kernel pushes on to the address "0", the pointer 
  // will wrap around and the kernel will overwrite the
  // top part of memory.
  sys_set_pgfault_handler ((u_int)&asm_pgfault_handler, 0);
  *(char *)0 = 'A';
}


void
exercise5_test5 (void)
{
  // try to get the kernel to overwrite the kernel stack
  sys_set_pgfault_handler ((u_int)&asm_pgfault_handler, KSTACKTOP);
  *(char *)0 = 'A';
}


void
__main (int argc, char *argv[])
{
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX move into entry.S
  DEF_SYM (__envs, UENVS);
  DEF_SYM (__ppages, UPPAGES);
  DEF_SYM (vpt, UVPT);
  DEF_SYM (vpd, UVPT + SRL(UVPT, 10));
  __env = &__envs[envidx(sys_getenvid ())];

  {
    int i;
    print ("__main: ", sys_getenvid(), "\n");
    print ("  argc ", argc, "\n");
    for (i = 0; i < argc; i++) {
      print ("  argv[", i, "] = ");
      sys_cputs (argv[i]);
      sys_cputs ("\n");
    }
  }


///BEGIN 200
#if 0
///END
  exercise2 ();
  //exercise3 ();
  //exercise4_test1 ();
  //exercise4_test2 ();
  //exercise4_test3 ();
  //exercise5_test1 ();
  //exercise5_test2 ();
  //exercise5_test3 ();
  //exercise5_test4 ();
  //exercise5_test5 ();

  //// run these lines for exercise 6
///BEGIN 200
#else
///END
  setup_xstack ();
  //exec ("simple", "hi", "a", "b", "c", "dddd", "eeeeeeeeeeeeeee", NULL);
  //exit ();
///BEGIN 200
#endif
///END
}


///END
