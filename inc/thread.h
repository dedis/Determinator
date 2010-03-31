// Simple thread fork/join API useful for benchmarking
#ifndef PIOS_INC_THREAD_H
#define PIOS_INC_THREAD_H

#ifdef PIOS_USER
#include <types.h>
#define printf cprintf	// use immediate console printing
#else
#include <sys/types.h>
#include <stdint.h>
#endif

void tfork(unsigned char child, void *(*fun)(void *), void *arg);
void tjoin(unsigned char child);
unsigned long long sys_time(void);

#endif	// PIOS_INC_THREAD_H
