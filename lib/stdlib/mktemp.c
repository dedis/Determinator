
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

static int counter;

char *
mktemp(char *template)
{
	int pos = strlen(template) - 6;
	assert(pos >= 0);

	struct stat st;
	do {
		sprintf(&template[pos], "%06x", ++counter);
	} while (stat(template, &st) >= 0);
	return template;
}

int
mkstemp(char *template)
{
	int fd;
	do {
		mktemp(template);
		fd = open(template, O_RDWR | O_CREAT | O_EXCL, 0666);
	} while (fd < 0 && errno == EEXIST);
	return fd;
}

