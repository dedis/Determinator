#include <inc/stdlib.h>
#include <inc/stdio.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/file.h>
#include <inc/errno.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/vm.h>


extern void tresume(int);


static int array[10];  


void print_array() {

	int i;
	for (i = 0; i < 10; i++) 
		printf("%d ", array[i]);
	printf("\n");
}


int main(int argc, char ** argv) {

	int fd, written, i;;
	char * filename;
	char * text = "exegi monumentum aere perennius\n";

	printf("This is a test.\n");
	if (argc > 1)
		filename = argv[1];
	else filename = "boo.txt";
	fd = open(filename, O_CREAT | O_WRONLY);
	if (fd == -1)
		panic("open: %s", strerror(errno));
	written = write(fd, text, strlen(text) + 1);
	if (written != strlen(text) + 1) 
		printf("written: %d  text length: %d\n", written, strlen(text));
	close(fd);


	print_array();

	int r = tfork(2);

	if (r == 0) {
		array[0] = 3;
		sys_cputs("Child\n");
		sys_ret();
		array[3] = 42;
		sys_cputs("Child again\n");
		sys_ret();
	}
	else {
		array[1] = 4;
		sys_cputs("Parent\n");
		tjoin(2);
		tresume(2);
		tjoin(2);
	}


	print_array();

	return EXIT_SUCCESS;
}
