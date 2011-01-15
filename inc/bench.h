#if LAB >= 9
// Simple thread API for multithreaded benchmarks.
// The API is easily compilable to pthreads as well as PIOS.
#ifndef PIOS_INC_BENCH_H
#define PIOS_INC_BENCH_H

#ifdef PIOS_USER
#include <types.h>
#define printf cprintf	// use immediate console printing
#else
#include <sys/types.h>
#include <stdint.h>
#define cprintf printf	// use system's normal printf
#endif

void bench_fork(unsigned char child, void *(*fun)(void *), void *arg);
void bench_join(unsigned char child);
unsigned long long bench_time(void);

#endif	// PIOS_INC_BENCH_H
#endif	// LAB >= 9
