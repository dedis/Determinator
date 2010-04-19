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
static pthread_barrier_t barrier;


void print_array() {

	int i;
	for (i = 0; i < 10; i++) 
		printf("%d ", array[i]);
	printf("\n");
}


void * func(void * args_ptr) {

	sys_cputs("In func\n");
	pthread_barrier_wait(&barrier);
	sys_cputs("In func after barrier\n");

	return (void *)7;

}

#define NUM_CHILDREN 4

int main(int argc, char ** argv) {

	// int master;
	// tparallel_begin(&master, 2, &func, NULL);
	int i, status, ret;
	pthread_t threads[NUM_CHILDREN];

	ret = pthread_barrier_init(&barrier, NULL, NUM_CHILDREN);
	if (ret != 0)
		fprintf(stderr, "*** pthread_barrier_init  %d\n", ret);


	sys_cputs("In parent.\n");
	// tparallel_end(master);
	for (i = 0; i < NUM_CHILDREN; i++)
		pthread_create(&threads[i], NULL, &func, NULL);
	sys_cputs("In parent again.\n");
	for (i = 0; i < NUM_CHILDREN; i++)
		pthread_join(threads[i], (void **)&status);
		
	ret = pthread_barrier_destroy(&barrier);
	if (ret != 0)
		fprintf(stderr, "*** pthread_barrier_destroy  %d\n", ret);

	return EXIT_SUCCESS;
}
