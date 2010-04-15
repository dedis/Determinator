#include <inc/stdlib.h>
#include <inc/stdio.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/errno.h>
#include <inc/assert.h>

int main(int argc, char ** argv) {

	int fd, written;
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

	printf("cwd %s\n", files->fi[files->cwd].de.d_name);

	return EXIT_SUCCESS;
}
