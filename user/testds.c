// Test deterministic scheduling-based pthreads

#define PIOS_DSCHED

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/unistd.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/pthread.h>
#include <inc/trap.h>

pthread_mutex_t m;
pthread_cond_t c;
int full;

void *exiter(void *arg)
{
	return (void*)((int)arg << 4);
}

void *joiner(void *arg)
{
	pthread_t t = arg;
	pthread_join(t, &arg);
	return (void*)((int)arg << 8);
}

void *producer(void *arg)
{
	int i;
	for (i = 0; i < 5; i++) {
		pthread_mutex_lock(&m);
		while (full)
			pthread_cond_wait(&c, &m);
		full = 1;
		cprintf("produced %d\n", i);
		pthread_cond_signal(&c);
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

void *consumer(void *arg)
{
	int i;
	for (i = 0; i < 5; i++) {
		pthread_mutex_lock(&m);
		while (!full)
			pthread_cond_wait(&c, &m);
		full = 0;
		cprintf("consumed %d\n", i);
		pthread_mutex_unlock(&m);
		pthread_cond_signal(&c);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	// try allocating some memory - this will initialize dsthreads
	malloc(123);
	malloc(123);
	malloc(123);

	// Create and join a thread
	pthread_t t;
	int r = pthread_create(&t, NULL, exiter, (void*)0xdeadbeef);
	assert(!r);
	void *st;
	r = pthread_join(t, &st); assert(!r);
	assert(st == (void*)0xeadbeef0);

	// OK, try two threads
	pthread_t t1,t2;
	r = pthread_create(&t1, NULL, exiter, (void*)0xea1); assert(!r);
	r = pthread_create(&t2, NULL, exiter, (void*)0xea2); assert(!r);
	r = pthread_join(t1, &st); assert(!r); assert(st == (void*)0xea10);
	r = pthread_join(t2, &st); assert(!r); assert(st == (void*)0xea20);

	// One thread joining a thread it didn't fork
	r = pthread_create(&t1, NULL, exiter, (void*)0xbad); assert(!r);
	r = pthread_create(&t2, NULL, joiner, t1); assert(!r);
	r = pthread_join(t2, &st); assert(!r); assert(st == (void*)0xbad000);

	// Producer/consumer thread pair
	r = pthread_mutex_init(&m, NULL); assert(!r);
	r = pthread_cond_init(&c, NULL); assert(!r);
	r = pthread_create(&t1, NULL, producer, NULL); assert(!r);
	r = pthread_create(&t2, NULL, consumer, NULL); assert(!r);
	r = pthread_join(t1, &st); assert(!r); assert(st == NULL);
	r = pthread_join(t2, &st); assert(!r); assert(st == NULL);
	assert(full == 0);

	return 0;
}

