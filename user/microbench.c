//#if SOL >= 4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

#include <inc/bench.h>


#define MAXTHREADS	8

int pg[1024][1024];

struct args {
	int npages;
	int fullpage;
	int nthreads;
	int thread;
	int val;
};

void forktest(void)
{
	pid_t child = fork();
	if (child == 0)
		exit(0);	
	assert(child > 0);
	waitpid(child, NULL, 0);
}

void *writefun(void *arg)
{
	struct args *a = (struct args*)arg;
	int nth = a->nthreads;
	int th = a->thread;
	int np = a->npages;
	int val = a->val;
	int i, j;

	if (a->fullpage) {
		for (i = 0; i < np; i++)
			for (j = th; j < 1024; j += nth)
				pg[i][j] = val;
	} else {
		for (i = 0; i < np; i++)
			pg[i][th] = val;
	}
	return NULL;
}

void writetest(struct args *a)
{
	int th;

	for (th = 0; th < a->nthreads; th++) {
		a->thread = th;
		bench_fork(th, writefun, &a);
	}
	for (th = 0; th < a->nthreads; th++) {
		bench_join(th);
	}
}

int main(int argc, char **argv)
{
	int full, np, nth, i, j, val = 0;
	struct args a;

	forktest();	// once to warm up
	const int forkiters = 10000;
	uint64_t ts = bench_time();
	for (i = 0; i < forkiters; i++)
		forktest();
	uint64_t td = (bench_time() - ts) / forkiters;
	printf("proc fork/wait: %lld ns\n", (long long)td);

	for (full = 0; full < 2; full++) {
		for (np = 1; np <= 1024; np *= 2) {
			for (nth = 1; nth <= MAXTHREADS; nth *= 2) {
				a.npages = np;
				a.fullpage = full;
				a.nthreads = nth;
				a.val = val++;
				writetest(&a);		// once to warm up
				const int iters = 10000;
				ts = bench_time();
				for (j = 0; j < iters; j++)
					writetest(&a);
				td = (bench_time() - ts) / iters;
				printf("fork/join x%d, %s %d pages: %lld ns\n",
					nth,
					full ? "scrubbing" : "touching", np,
					(long long)td);
			}
		}
	}

	return 0;
}

//#endif
