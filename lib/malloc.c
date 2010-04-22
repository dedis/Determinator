#include <inc/malloc.h>
#include <inc/stdio.h>
#include <inc/stdlib.h>


extern char start[], end[];


void * malloc(size_t size) {
	fprintf(stderr, "end %p\n", end);
	return NULL;
}
