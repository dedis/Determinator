#if LAB >= 6

#include <inc/lib.h>

/*
 * Simple malloc/free.
 *
 * Uses the address space to do most of the hard work.
 * The address space from mbegin to mend is scanned
 * in order.  Pages are allocated, used to fill successive
 * malloc requests, and then left alone.  Free decrements
 * a ref count maintained in the page; the page is freed
 * when the ref count hits zero.
 *
 * If we need to allocate a large amount (more than a page)
 * we can't put a ref count at the end of each page,
 * so we mark the pte entry with the bit PTE_CONTINUED.
 */
enum
{
	MAXMALLOC = 1024*1024	/* max size of one allocated chunk */
};

#define PTE_CONTINUED 0x400

static u_char *mbegin = (u_char*)0x08000000;
static u_char *mend   = (u_char*)0x10000000;
static u_char *mptr;

static int
isfree(void *v, int n)
{
	u_int va;

	va = (u_int)v;
	for(va=(u_int)v; n>0; va+=BY2PG, n-=BY2PG)
		if(va >= (u_int)mend || ((vpd[PDX(va)]&PTE_P) && (vpt[VPN(va)]&PTE_P)))
			return 0;
	return 1;
}

void*
malloc(u_int n)
{
	int i, cont;
	int nwrap;
	u_int *ref;
	void *v;

	if(mptr == 0)
		mptr = mbegin;

	if(n >= MAXMALLOC)
		return 0;

	if((u_int)mptr % BY2PG){
		/*
		 * we're in the middle of a partially
		 * allocated page - can we add this chunk?
		 * the +4 below is for the ref count.
		 */
		if((u_int)mptr/BY2PG == (u_int)(mptr+n-1+4)/BY2PG){
			ref = (u_int*)(mptr - (u_int)mptr%BY2PG + BY2PG-4);
			(*ref)++;
			v = mptr;
			mptr += n;
			return v;
		}
		/*
		 * stop working on this page and move on.
		 */
		mptr += BY2PG - (u_int)mptr%BY2PG;
	}

	/*
	 * now we need to find some address space for this chunk.
	 * if it's less than a page we leave it open for allocation.
	 * runs of more than a page can't have ref counts so we 
	 * flag the PTE entries instead.
	 */
	nwrap = 0;
	for(;;){
		if(isfree(mptr, n+4))
			break;
		mptr += BY2PG;
		if(mptr == mend){
			mptr = mbegin;
			if(++nwrap == 2)
				return 0;	/* out of address space */
		}
	}

	/*
	 * allocate at mptr - the +4 makes sure we allocate a ref count.
	 */
	for(i=0; i<n+4; i+=BY2PG){
		cont = (i+BY2PG < n+4) ? PTE_CONTINUED : 0;
		if(sys_mem_alloc(0, (u_int)mptr+i, PTE_P|PTE_U|PTE_W|cont) < 0){
			for(; i>=0; i-=BY2PG)
				sys_mem_unmap(0, (u_int)mptr+i);
			return 0;	/* out of physical memory */
		}
	}

	ref = (u_int*)(mptr+i-4);
	(*ref)++;
	v = mptr;
	mptr += n;
	return v;
}

void
free(void *v)
{
	u_char *c;
	u_int *ref;

	if(v == 0)
		return;

	c = v;
	assert(mbegin <= c && c < mend);
	c -= (u_int)c%BY2PG;

	while(vpt[VPN((u_int)c)]&PTE_CONTINUED){
		sys_mem_unmap(0, (u_int)c);
		c += BY2PG;
		assert(mbegin <= c && c < mend);
	}

	/*
	 * c is just a piece of this page, so dec the ref count
	 * and maybe free the page.
	 */
	ref = (u_int*)(c+BY2PG-4);
	if(--(*ref) == 0)
		sys_mem_unmap(0, (u_int)c);	
}

#endif
