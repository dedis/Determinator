/* See COPYRIGHT for copyright information. */

///LAB2

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/printf.h>
#include <kern/kclock.h>

unsigned int freemem;           /* Pointer to next byte of free mem */
unsigned int p0cr3_boot;        /* Physical address of boot time pg dir */  
Pde *p0pgdir_boot;              /* Virtual address of boot time pg dir */  

struct Ppage *ppages;		/* Array of all Ppage structures */

/* These variables are set by i386_detect_memory() */
u_long maxphys;                 /* Amount of memory (in bytes) */
u_long nppage;                  /* Amount of memory (in pages) */
long basememsize;               /* Amount of base memory (in bytes) */
long extmemsize;                /* Amount of extended memory (in bytes) */

static struct Ppage_list free_list;	/* Free list of physical pages */

///SOL2
static u_int cpuid_features = 0; // We don't detect CPU type right now.
///END


// Global descriptor table.
//
// The kernel and user segments are identical (except for the DPL).
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
struct seg_desc gdt[] =
{
  /* 0x0 - unused (always faults--for trapping NULL far pointers) */
  mmu_seg_null,
  /* 0x8 - kernel code segment */
  [GD_KT >> 3] = seg (STA_X | STA_R, 0x0, 0xffffffff, 0),
  /* 0x10 - kernel data segment */
  [GD_KD >> 3] = seg (STA_W, 0x0, 0xffffffff, 0),
  /* 0x18 - user code segment */
  [GD_UT >> 3] = seg (STA_X | STA_R, 0x0, 0xffffffff, 3),
  /* 0x20 - user data segment */
  [GD_UD >> 3] = seg (STA_W, 0x0, 0xffffffff, 3),
  /* 0x40 - tss */
  [GD_TSS >> 3] =  mmu_seg_null /* initialized in idt_init () */
};

struct pseudo_desc gdt_pd =
{
  0, sizeof (gdt) - 1, (unsigned long) gdt,
};


///SOL2
//
// Allocate fixed space in physical memory.  *pt will be set to a page
// table containing the space allocated, so that the space can be
// mapped read-only in user space.
//

static void *
ptspace_alloc (Pte ** pt, u_int bytes)
{
  u_int i, lim;
  void *ret;
  
  *pt = (void *)freemem;
  freemem += NBPG;
  ret = (void *)freemem;
  lim = (bytes + PGMASK) >> PGSHIFT;
  if (lim > NLPG)
    /* bootup */
    panic ("pt_alloc: 4 Megs exceeded!");
  bzero (*pt, NBPG);
  for (i = 0; i < lim; i++, freemem += NBPG) {
    bzero ((void *)freemem, NBPG);
    (*pt)[i] = (freemem - KERNBASE) | PG_P | PG_U | PG_W;
  }
  return (ret);
}
///END

//
// initialize a ppage structure 
//
static void
ppage_initpp (struct Ppage *pp)
{
  bzero (pp, sizeof (*pp));
}


void
ppage_check ()
{
  int r;
  struct Ppage *pp;
  struct Ppage *pp1;
  struct Ppage *pp2;
  // XXX
  // This alloc_list idea is dumb and it complicates
  // the code!
  //
  // Just reference ppage directly &ppages[ppn].
  //

  struct Ppage_list alloc_list;
  LIST_INIT (&alloc_list);

  while (1) {
    r = ppage_alloc (&pp);
    if (r < 0)
      break;
    LIST_INSERT_HEAD(&alloc_list, pp, pp_link);

    // must not return an in use page
    assert (pp->pp_refcnt == 0);
    assert ((u_int)pp2va(pp) < (u_int)IOPHYSMEM + KERNBASE
	    || (u_int)pp2va(pp) >= (u_int)freemem);
  }


  pp = LIST_FIRST (&alloc_list);   
  pp1 = LIST_NEXT (pp, pp_link);
  pp2 = LIST_NEXT (pp1, pp_link);
  LIST_REMOVE (pp, pp_link);
  LIST_REMOVE (pp1, pp_link);
  LIST_REMOVE (pp2, pp_link);

  // there is no free memory, so we can't allocate a page table 
  assert (ppage_insert (p0pgdir_boot, pp, 0x0, PG_P) < -1);
  ppage_free (pp);

  assert (ppage_insert (p0pgdir_boot, pp1, 0x0, PG_P) == 0);
  // expect: pp1 mapped 0x0
  assert (pgdir_get_ptep (p0pgdir_boot, 0x0));
  assert (*pgdir_get_ptep (p0pgdir_boot, 0x0) >> PGSHIFT == pp2ppn (pp1));

  assert (ppage_insert (p0pgdir_boot, pp2, NBPG, PG_P) == 0);
  // expect: pp1 mapped 0x0, pp2 at NBPG
  assert (pgdir_get_ptep (p0pgdir_boot, 0x0));
  assert (*pgdir_get_ptep (p0pgdir_boot, 0x0) >> PGSHIFT == pp2ppn (pp1));
  assert (pgdir_get_ptep (p0pgdir_boot, NBPG));
  assert (*pgdir_get_ptep (p0pgdir_boot, NBPG) >> PGSHIFT == pp2ppn (pp2));
  assert (pp1->pp_refcnt == 1);
  assert (pp2->pp_refcnt == 1);

  // insert pp1 at VA NBPG (pp2 is already mapped there)
  assert (ppage_insert (p0pgdir_boot, pp1, NBPG, PG_P) == 0);
  // expect: pp1 mapped at both 0x0 and NBPG, pp2 not mapped
  assert (pgdir_get_ptep (p0pgdir_boot, 0x0));
  assert (*pgdir_get_ptep (p0pgdir_boot, 0x0) >> PGSHIFT == pp2ppn (pp1));
  assert (pgdir_get_ptep (p0pgdir_boot, NBPG));
  assert (*pgdir_get_ptep (p0pgdir_boot, NBPG) >> PGSHIFT == pp2ppn (pp1));
  assert (pp1->pp_refcnt == 2);
  assert (pp2->pp_refcnt == 0);
  // alloc should return pp2, since it should be the only thing on the free_list
  assert (ppage_alloc (&pp) == 0);
  assert (pp == pp2);

  ppage_remove (p0pgdir_boot, 0x0);
  // expect: pp1 mapped at NBPG
  assert (pgdir_get_ptep (p0pgdir_boot, 0x0));
  assert (*pgdir_get_ptep (p0pgdir_boot, 0x0) == 0);
  assert (pgdir_get_ptep (p0pgdir_boot, NBPG));
  assert (*pgdir_get_ptep (p0pgdir_boot, NBPG) >> PGSHIFT == pp2ppn (pp1));
  assert (pp1->pp_refcnt == 1);
  assert (pp2->pp_refcnt == 0);

  ppage_remove (p0pgdir_boot, NBPG);
  // expect: pp1 not mapped
  assert (pgdir_get_ptep (p0pgdir_boot, 0x0));
  assert (*pgdir_get_ptep (p0pgdir_boot, 0x0) == 0);
  assert (pgdir_get_ptep (p0pgdir_boot, NBPG));
  assert (*pgdir_get_ptep (p0pgdir_boot, NBPG) == 0);
  assert (pp1->pp_refcnt == 0);
  assert (pp2->pp_refcnt == 0);

  // return all remaining memory to the free list
  ppage_free (pp);
  while (1) {
    pp = LIST_FIRST (&alloc_list);
    if (!pp)
      break;
    LIST_REMOVE (pp, pp_link);
    ppage_free (pp);
  }

  printf("ppage_check() succeeded!\n");
}


//
// Checks that the kernel part of virtual address space
// has been setup roughly correctly (by i386_vm_init ()).
//
// This function doesn't test every corner case,
// in fact it doesn't test the permission bits at all,
// but it is a pretty good sanity check. 
//
void
check_boot_page_directory ()
{
  extern char bootstacktop;
  u_int nbytes;
  int i;
  Pte *pt;
  u_int ptr;
  
  // check final value of freemem
  assert ((u_int)freemem == (u_int)&__envs[NENV]);
  
  // check __env array and its pg tbl
  ptr = (u_int)&__envs[0];
  pt = (Pte*)(ptr - NBPG); // __env's pgtbl resides right before it
  nbytes = PGROUNDUP (NENV * sizeof (struct Env));
  for (i = 0; i < nbytes / NBPG; i++, ptr += NBPG)
    assert ((pt[i] & ~PGMASK) == kva2pa (ptr));

  // check ppage array and its pg tbl
  ptr = (u_int)&ppages[0];
  pt = (Pte*)(ptr - NBPG); // ppage's pgtbl resides right before it
  nbytes = PGROUNDUP (nppage * sizeof (struct Ppage));
  for (i = 0; i < nbytes / NBPG; i++, ptr += NBPG)
    assert ((pt[i] & ~PGMASK) == kva2pa (ptr));

  // check the pgtbls that map phys memory
  pt = (Pte *)((u_int)pt - NBPG * NPPT);
  for (i = 0; i < NPPT * NLPG; i++)
    assert (pt[i] >> PGSHIFT == i);

  // check the pgtbl for the kernel stack
  for (i = 1; i <= KSTKSIZE/NBPG; i++) {
    ptr = (u_int)ptov (pt[-i] & ~PGMASK);
    assert (ptr < (u_int)&bootstacktop && ptr >= (u_int)&bootstacktop - KSTKSIZE);
  }

  // check the boot pgdir
  pt = (Pte *)((u_int)pt - 2*NBPG);
  assert ((u_int)pt == (u_int)p0pgdir_boot);
  
  // check for zero in certain PDEs 
  for (i = 0; i < NLPG; i++) {
    if (i >= PDENO(KERNBASE))  assert (p0pgdir_boot[i]);
    else if (i == PDENO(VPT)) assert (p0pgdir_boot[i]);
    else if (i == PDENO(UVPT)) assert (p0pgdir_boot[i]);
    else if (i == PDENO(KSTACKTOP-NBPD)) assert (p0pgdir_boot[i]);
    else if (i == PDENO(UPPAGES)) assert (p0pgdir_boot[i]);
    else if (i == PDENO(UENVS)) assert (p0pgdir_boot[i]);
    else assert (p0pgdir_boot[i] == 0);
  }

  printf("check_boot_page_directory() succeeded!\n");
}


void
i386_detect_memory ()
{
#define nvram_read(r) (mc146818_read(NULL, r))
  // As done by OpenBSD
  basememsize = ((nvram_read (NVRAM_BASEHI) << 18)
		 | (nvram_read (NVRAM_BASELO) << 10)) & ~PGMASK;
  extmemsize = ((nvram_read (NVRAM_EXTHI) << 18)
		| (nvram_read (NVRAM_EXTLO) << 10)) & ~PGMASK;

  // calculate the maxmium physical address based on whether
  // or not there is extendend memory 
  maxphys = extmemsize ? EXTPHYSMEM + extmemsize : basememsize;
  // Number of physical pages
  nppage = PGNO (maxphys);            

  printf ("Physical memory: %dK available, ", (int) (maxphys >> 10));
  printf ("base = %dK, ", (u_int) (basememsize >> 10));
}


// Setups a two-level page table:
//    p0pgdir_boot is its virtual address of the root
//    p0cr3_boot is the physical adresss of the root
// Then, turns on paging.  Also, segmentation to effectively turned
// off (ie, the segment base addrs are set to zero).

// This function only sets up the kernel part of the address space
// (ie. addresses >= UTOP).  (The user part of the address space
// will be setup later.)
//
// From UTOP to ULIM, the user is allowed to read but not write.
// Above ULIM the user cannot read (or write). 


void
i386_vm_init ()
{
  Pte *kstkpt;
  extern char end;
  u_int cr0;
  int i;
  extern char bootstacktop;
///SOL2
  // XXX get rid of these? and ptspace_alloc?
  Pte *ppage_upt;		/* Pt for read-only ppage structures */
  Pte *env_upt;			/* Pt for read-only and envs */
///END

  freemem = PGROUNDUP((u_int)&end);  // every thing past BSS is free

  //////////////////////////////////////////////////////////////////////
  // create initial page directory.
  bzero ((void *)freemem, NBPG);
  p0pgdir_boot = (void *)freemem;
  p0cr3_boot = (u_int)kva2pa(p0pgdir_boot);
  freemem += NBPG;

  //////////////////////////////////////////////////////////////////////
  // Recursively insert PD in itself as a page table, to form
  // a virtual page table at virtual address VPT.
  // (For now, you don't have understand the greater purpose of the
  // following two lines.)
  
  // Permissions: kernel RW, user NONE 
  p0pgdir_boot[PDENO(VPT)]  = (Pde)kva2pa(p0pgdir_boot)|PG_W|PG_P;

  // same for UVPT
  // Permissions: kernel R, user R 
  p0pgdir_boot[PDENO(UVPT)] = (Pde)kva2pa(p0pgdir_boot)|PG_U|PG_P;


///SOL2
///ELSE
  printf ("\n");
  panic ("i386_vm_init: This function is not finished\n");
///END

  //////////////////////////////////////////////////////////////////////
  // Map the kernel stack:
  //   [KSTACKTOP-NBPD, KSTACKTOP)  -- the complete VA range of the stack
  //     * [KSTACKTOP-KSTKSIZE, KSTACKTOP) -- backed by physical memory
  //     * [KSTACKTOP-NBPD, KSTACKTOP-KSTKSIZE) -- not backed => faults
  //   Permissions: kernel RW, user NONE

  // step 1: allocate a page table for the stack
  // step 2: insert the KSTKSIZE phys pages located at bootstacktop (see locore.S)
  //         into the page table
  // step 3: insert the page table into the page directory
  //
  // Your code goes here:

///SOL2
  bzero ((void *)freemem, NBPG);
  kstkpt = (void *)freemem;
  freemem += NBPG;

  assert ((u_int)&bootstacktop % NBPG == 0);
  assert (KSTKSIZE % NBPG == 0);
  for (i = 1; i <= KSTKSIZE/NBPG; i++)
    kstkpt[NLPG - i] = kva2pa(&bootstacktop - i*NBPG)|PG_P|PG_W;

  p0pgdir_boot[PDENO(KSTACKTOP-NBPD)] = kva2pa(kstkpt)|PG_P|PG_W;
///END

  //////////////////////////////////////////////////////////////////////
  // Map all of physical memory at KERNBASE. 
  // Ie.  the VA range [KERNBASE, 2^32 - 1] should map to
  //      the PA range [0, 2^32 - 1 - KERNBASE]   
  // We might not have that many (ie. 2^32 - 1 - KERNBASE)    
  // bytes of physical memory.  But we just set up the mapping anyway.
  // Permissions: kernel RW, user NONE

  // Step 1: allocate NPPT page tables
  // Step 2: initialize the page tables
  // Step 3: insert the page tables into the page directory
  // your code goes here: 

///SOL2
  if (cpuid_features & 0x8) {
    // use 4MB mappings
    lcr4 (rcr4() | CR4_PSE);
    for (i = 0; i < NPPT; i++)
      p0pgdir_boot[PDENO(KERNBASE) + i] = (i*NBPD)|PG_P|PG_W|PG_PS;
  } else {
    // create the page tables
    for (i = 0; i < NPPT * NLPG; i++)
      ((Pte *)freemem)[i] = i*NBPG |PG_P|PG_W;
    
    // insert the page tables into the page directory
    for (i = 0; i < NPPT; i++) {
      p0pgdir_boot[PDENO(KERNBASE) + i] = kva2pa(freemem)|PG_P|PG_W;
      freemem += NBPG;
    }
  }
///END

  //////////////////////////////////////////////////////////////////////
  // Make 'ppage' point to an array of size 'nppage' of 'struct Ppage'.   
  // Map this array read-only by the user at UPPAGES (ie. perm = PG_U | PG_P)
  // Permissions:
  //    - ppages -- kernel RW, user NONE
  //    - the image mapped at UPPAGES  -- kernel R, user R

  // Step 1: allocate a page table
  // Step 2: allocate the memory for the ppage array
  // Step 3: set ppages to point this memory
  // Step 4: fill in the page table to map the ppage array
  // Step 5: insert the page table into p0pgdir_boot
  // your code goes here: 


///SOL2
  printf("nppage %ld, Ppage_sizeof %u\n", nppage, (u_int)sizeof(struct Ppage));
  ppages = ptspace_alloc (&ppage_upt, nppage * sizeof(struct Ppage));
  p0pgdir_boot[PDENO(UPPAGES)]  = kva2pa(ppage_upt) | PG_U | PG_P;
///END


  //////////////////////////////////////////////////////////////////////
  // Make '__envs' point to an array of size 'NENV' of 'struct Env'.
  // Map this array read-only by the user at UENVS (ie. perm = PG_U | PG_P)
  // Permissions:
  //    - __envs -- kernel RW, user NONE
  //    - the image mapped at UENVS  -- kernel R, user R

  // Step 1: allocate a page table
  // Step 2: allocate the memory for the __env array
  // Step 3: set __env to point this memory
  // Step 4: fill in the page table to map the __env array
  // Step 5: insert the page table into p0pgdir_boot
  // your code goes here: 

///SOL2
  __envs = ptspace_alloc (&env_upt, NENV * sizeof (struct Env));
  p0pgdir_boot[PDENO(UENVS)]    = kva2pa(env_upt) | PG_U | PG_P;
///END

  check_boot_page_directory ();

  //////////////////////////////////////////////////////////////////////
  // On x86 segmentation maps a VA to a LA (linear addr) and
  // paging maps the LA to a PA. Ie. VA => LA => PA.  If paging is
  // turned off the LA is used as the PA.  Note: there is no way to
  // turn off segmentation.  The closest thing is to set the base
  // address to 0, so the VA => LA mapping is the identity.

  // Current mapping: KERNBASE+x => x. (seg base=-KERNBASE and paging is off)

  // From here on down we must maintain this VA KERNBASE + x => PA x
  // mapping, even though we are turning on paging and reconfiguring
  // segmentation.


  // VA 0:4MB => PA 0:4MB.  (Limits our kernel to <4MB)
  p0pgdir_boot[0] = p0pgdir_boot[PDENO(KERNBASE)];

  lcr3 (p0cr3_boot); // install page table   
  cr0 = rcr0 ();
  cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP;
  cr0 &= ~(CR0_TS|CR0_EM);
  lcr0 (cr0); // turn on paging.

  // Current mapping: KERNBASE+x => x => x.
  // (x < 4MB so uses paging p0pgdir_boot[0])

  // reload all segments.
  asm volatile ("lgdt _gdt_pd+2");
  asm volatile ("movw %%ax,%%gs" :: "a" (GD_UD|3));
  asm volatile ("movw %%ax,%%fs" :: "a" (GD_UD|3));
  asm volatile ("movw %%ax,%%es" :: "a" (GD_KD));
  asm volatile ("movw %%ax,%%ds" :: "a" (GD_KD));
  asm volatile ("movw %%ax,%%ss" :: "a" (GD_KD));
  asm volatile ("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));  // reload cs
  asm volatile ("lldt %0" :: "m" (0));

  // Final mapping: KERNBASE+x => KERNBASE+x => x.

  // This mapping was only used after paging was turned on but
  // before the segment registers were reloaded.
  p0pgdir_boot[0] = 0; 
}



//  
// Initialize ppage structure and memory free list.
//
void
ppage_init (void)
{
///SOL2
  int i;
  LIST_INIT (&free_list);


  /* Save the first physical page for good luck */
  ppages[0].pp_refcnt = 1;

  for (i = 1; i < nppage; i++) {
    // all of base mem and every thing above the kernel's mem is free
    int inuse = !(i < PGNO (basememsize) || i >= PGNO (freemem - KERNBASE));
    // except the I/O range
    if (i >= PGNO (IOPHYSMEM) && i < PGNO(EXTPHYSMEM))
      inuse = 1;

    ppages[i].pp_refcnt = inuse;
    if (!inuse)
      LIST_INSERT_HEAD(&free_list, &ppages[i], pp_link);
  }
///ELSE
  // The exaple code here marks all pages as free.
  // However this is not truly the case.  What memory is free?
  //  1) Mark page 0 as in use (for good luck) 
  //  2) Mark the rest of base memory as free.
  //  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM) => mark it as in use
  //     So that it can never be allocated.      
  //  4) Then extended memory (ie. >= EXTPHYSMEM):
  //     ==> some of it's in use some is free. Where is the kernel?
  //     Which pages are used for page tables and other data structures?    
  //
  // Change the code to reflect this.
  int i;
  LIST_INIT (&free_list);
  for (i = 0; i < nppage; i++) {
    ppages[i].pp_refcnt = 0;
    LIST_INSERT_HEAD(&free_list, &ppages[i], pp_link);
  }
///END
}



// ----------------------------------------------------------------------

//
// RETURNS:
//  on success: a pointer to the page table entry corresponding to 'va'
//  on error: NULL -- there might not be a page table allocated.
//
Pte *
pgdir_get_ptep (Pde *pgdir, u_int va) 
{
  Pde pde = pgdir[PDENO (va)];
  if (pde & PG_P) {
    Pte *pt = ptov (pde & ~PGMASK);
    return (&pt[PTENO (va)]);
  } 
  return NULL;
}


//
// Allocate a page table if one is needed for VA va. Insert the page
// table into the page directory given
//
// RETURNS: 0 on success, <0 on error
//
int
pgdir_check_and_alloc (Pde *pgdir, u_int va)
{
  Pde *pdep = &pgdir[PDENO (va)];
  struct Ppage *pp;
  int r;
  
  if (!(*pdep & PG_P)) {
    if ((r = ppage_alloc (&pp)) < 0) {
      warn ("pt_check_and_alloc: could not alloc page for pt");
      return r;
    }
    // make sure all those PG_P bits are zero
    bzero (pp2va (pp), NBPG);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table 
    // entries, if necessary.
    *pdep = pp2pa (pp) | PG_P | PG_W | PG_U;
  }
  return 0;
}


//
// Return a page to the free list.
// (this function should only be called when pp->pp_refcnt reaches 0)
//
void
ppage_free (struct Ppage *pp)
{
  // Fill this function in
///SOL2
  if (pp->pp_refcnt) {
    warn ("ppage_free: attempt to free mapped page");
    return;		/* be conservative and assume page is still used */
  }
  LIST_INSERT_HEAD(&free_list, pp, pp_link);
  pp->pp_refcnt = 0;
///END
}


//
// Allocates a physical ppage.
//
// *pp -- is set to point to the Ppage struct of the newly allocated
// page
//
// RETURNS 
//   0 -- on success
//   -E_NO_MEM -- otherwise 
//
// Hint: use LIST_FIRST, LIST_REMOVE, ppage_initpp()
// Hint: pp_refcnt should not be incremented 
int
ppage_alloc (struct Ppage **pp)
{
  // Fill this function in
///SOL2

  *pp = LIST_FIRST (&free_list);
  if (*pp) {
    LIST_REMOVE(*pp, pp_link);
    ppage_initpp (*pp);
    return 0;
  }

  warn ("ppage_alloc() can't find memory");
///END
  return -E_NO_MEM;
}


inline void
tlb_invalidate (u_int va, Pde *pgdir)
{
  /* only flush the entry if we're modifying the current address space */
  if (!curenv || curenv->env_pgdir == pgdir)
    invlpg (va);
}


//
// Unmaps the physical page at virtual address 'va'.
//
// Details:
//   - The ref count on the physical page should decrement.
//   - The physical page should be freed if the refcount reaches 0.
//   - The pg table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated (if you change the pg dir/pg table)
//
// Hint: The TA solution is implemented using
//  pgdir_get_ptep(), ppage_free(), tlb_invalidate()
//
void
ppage_remove (Pde *pgdir, u_int va) 
{
  // Fill this function in
///SOL2

  struct Ppage *pp;
  Pte *ptep;

  /* Find the PTE and unmap the page */
  ptep = pgdir_get_ptep (pgdir, va);
  if (!ptep)
    return;

  if (*ptep & PG_P) {
    if (PGNO(*ptep) >= nppage) {
      warn("ppage_remove: found bogus PTE 0x%08x (pgdir %p, va 0x%08x)", 
	   *ptep, pgdir, va);
      return;
    }
    
    pp = &ppages[PGNO(*ptep)];
    pp->pp_refcnt--;
    
    /* free the page if it's not mapped anywhere. */
    if (pp->pp_refcnt == 0)
      ppage_free (pp);
  }
  *ptep = 0;  
  tlb_invalidate (va, pgdir);
  
///END
}


//
// Map the physical page 'pp' at virtual address 'va'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm'.
//
// Details
//   - If there is already a page mapped at 'va', it is ppage_remove()d.
//   - If necesary, on demand, allocates a page table and inserts it into 'pgdir'.
//   - pp->pp_refcnt should be incremented if the insertion succeeds
//
// RETURNS: 
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
// Hint: The TA solution is implemented using
//  pgdir_check_and_alloc(), pgdir_get_ptep(), and ppage_remove()
//

int
ppage_insert (Pde *pgdir, struct Ppage *pp, u_int va, u_int perm) 
{
  // Fill this function in
///SOL2
  Pte *ptep;
  int r;

  va &= ~PGMASK;
  if ((r = pgdir_check_and_alloc (pgdir, va)) < 0)
    return r;

  pp->pp_refcnt++;

  ptep = pgdir_get_ptep (pgdir, va);
  assert (ptep);
  if (*ptep & PG_P)
    ppage_remove (pgdir, va);

  *ptep = pp2pa (pp) | perm;

///END
  return 0;
}
///END
