#if LAB >= 4
// Prime sieve.  See http://plan9.bell-labs.com/~rsc/thread.html.
// The picture halfway down the page and the text surrounding it
// explain what's going on here.
//
// Since NENVS is 1024, we can print 1022 primes before running out.
// The remaining two environments are the integer generator at the bottom
// of main and user/idle.

#include "lib.h"

u_int
primeproc(void)
{
	int i, id, p;
	u_int envid;

	// fetch a prime from our left neighbor
top:
	p = ipc_recv(&envid);
	printf("%d ", p);

	// fork a right neighbor to continue the chain
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0)
		goto top;
	
	// filter out multiples of our prime
	for (;;) {
		i = ipc_recv(&envid);
		if (i%p)
			ipc_send(id, i);
	}
}

void
umain(void)
{
	int i, r, id;
	u_int addr;

	// fork the first prime process in the chain
	if ((id=fork()) < 0)
		panic("fork: %e", id);
	if (id == 0)
		primeproc();

	// feed all the integers through
	for (i=2;; i++)
		ipc_send(id, i);
}

#endif
