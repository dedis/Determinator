#if LAB >= 6
// Concurrent version of prime sieve of Eratosthenes.
// Invented by Doug McIlroy, inventor of Unix pipes.
// See http://plan9.bell-labs.com/~rsc/thread.html.
// The picture halfway down the page and the text surrounding it
// explain what's going on here.
//
// Since NENVS is 1024, we can print 1022 primes before running out.
// The remaining two environments are the integer generator at the bottom
// of main and user/idle.

#include "lib.h"

u_int
primeproc(int fd)
{
	int i, id, p, pfd[2], wfd;

	// fetch a prime from our left neighbor
top:
	if (readn(fd, &p, 4) != 4)
		panic("primeproc could not read initial prime");

	printf("%d ", p);

	// fork a right neighbor to continue the chain
	if ((i=pipe(pfd)) < 0)
		panic("pipe: %e", i);
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0) {
		close(fd);
		close(pfd[1]);
		fd = pfd[0];
		goto top;
	}

	close(pfd[0]);
	wfd = pfd[1];

	// filter out multiples of our prime
	for (;;) {
		if (readn(fd, &i, 4) != 4)
			panic("primeproc %d readn", p);
		if (i%p)
			if (write(wfd, &i, 4) != 4)
				panic("primeproc %d write", p);
	}
}

void
umain(void)
{
	int i, id, p[2];

	argv0 = "primespipe";

	if ((i=pipe(p)) < 0)
		panic("pipe: %e", i);

	// fork the first prime process in the chain
	if ((id=fork()) < 0)
		panic("fork: %e", id);

	if (id == 0) {
		close(p[1]);
		primeproc(p[0]);
	}

	close(p[0]);

	// feed all the integers through
	for (i=2;; i++)
		if (write(p[1], &i, 4) != 4)
			panic("generator write");
}

#endif
