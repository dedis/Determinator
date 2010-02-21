#if LAB >= 4
// Unix compatibility API - standard Unix functions
#ifndef PIOS_INC_UNISTD_H
#define PIOS_INC_UNISTD_H 1

#include <inc/types.h>

void	exit(int status);

int	close(int fd);
ssize_t	read(int fd, void *buf, size_t nbytes);
ssize_t	write(int fd, const void *buf, size_t nbytes);
off_t	lseek(int fd, off_t offset, int whence);
int	dup(int oldfd);
int	dup2(int oldfd, int newfd);
int	ftruncate(int fd, off_t size);

int	remove(const char *path);

#endif	// !PIOS_INC_UNISTD_H
#endif	// LAB >= 4
