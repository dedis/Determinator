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


#define NUM_CHILDREN 4

static int array[10];  
static pthread_barrier_t barrier;


void print_array() {

	int i;
	for (i = 0; i < 10; i++) 
		printf("%d ", array[i]);
	printf("\n");
}


void * func(void * args_ptr) {

	int s = pthread_self();
	switch(s) {
	case 1: sys_cputs("1\n"); break;
	case 2: sys_cputs("2\n"); break;
	case 3: sys_cputs("3\n"); break;
	case 4: sys_cputs("4\n"); break;
	default: sys_cputs("other\n"); break;
	}
	array[s] = s;
	sys_cputs("In func before barrier.\n");
	pthread_barrier_wait(&barrier);
	sys_cputs("In func after barrier.\n");
       	array[s + NUM_CHILDREN] = s;


	return (void *)7;

}

int main(int argc, char ** argv) {

	int i, status, ret;
	pthread_t threads[NUM_CHILDREN];

	ret = pthread_barrier_init(&barrier, NULL, NUM_CHILDREN);
	if (ret != 0)
		fprintf(stderr, "*** pthread_barrier_init  %d\n", ret);


	fprintf(stderr, "In parent.\n");

	print_array();

	for (i = 0; i < NUM_CHILDREN; i++)
		pthread_create(&threads[i], NULL, &func, NULL);
	fprintf(stderr, "In parent again.\n");
	for (i = 0; i < NUM_CHILDREN; i++)
		pthread_join(threads[i], (void **)&status);
		
	ret = pthread_barrier_destroy(&barrier);
	if (ret != 0)
		fprintf(stderr, "*** pthread_barrier_destroy  %d\n", ret);

	print_array();

	return EXIT_SUCCESS;
}
