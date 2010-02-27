#if LAB >= 4
// Basic user-space file and I/O support functions,
// used by the standard I/O functions in stdio.c.

#include <inc/file.h>
#include <inc/unistd.h>
#include <inc/dirent.h>
#include <inc/assert.h>

int
stat(const char *path, struct stat *statbuf)
{
	struct dirent *de = dir_walk(path, 0);
	if (de == NULL)
		return -1;
	return fileino_stat(de->d_ino, statbuf);
}

int
fstat(int fd, struct stat *statbuf)
{
	assert(filedesc_isopen(&files->fd[fd]));
	return fileino_stat(files->fd[fd].ino, statbuf);
}

#endif /* LAB >= 4 */
