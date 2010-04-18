#include <inc/stdlib.h>
#include <inc/stdio.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/file.h>
#include <inc/errno.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/vm.h>
#include <inc/pthread.h>


extern void tresume(int);
extern void tbarrier_merge(uint16_t *, int);


static int array[10];  


void print_array() {

	int i;
	for (i = 0; i < 10; i++) 
		printf("%d ", array[i]);
	printf("\n");
}


void * func(void * args_ptr) {

	sys_cputs("In func\n");
	return (void *)5;

}


void * func2(void * args_ptr) {

	return NULL;
}


int main(int argc, char ** argv) {

	int master;
	tparallel_begin(&master, 2, &func, NULL);
	printf("In parent again.\n");
	tparallel_end(master);
		
	return EXIT_SUCCESS;
}
