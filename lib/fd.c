#if LAB >= 4

#include <inc/debug.h>
#include <inc/unix.h>


// Table of all Unix "devices" the API supports
unixdev unixdevs[UNIXDEV_MAX] = {
	&unix_consdev,
	&unix_filedev,
#if LAB >= 99
	&unix_sockdev,
#endif
};


// Finds the smallest fd from 0 to OPEN_MAX-1 that isn't in use.
// Returns fd on success, -1 on error with errno set appropriately.
// Doesn't actually mark the returned fd in-use; caller must do that.
// Errors are:
//	EMFILE: too many open files - no available file descriptors
int
unixfd_alloc(void)
{
	int i;
	for (i = 0; i < OPEN_MAX; i++)
		if (unixstate->fd[i].ino == 0) {
			return i;
		}

	errno = EMFILE;
	return -1;
}

// Check that fd is in range and open.
// Returns a pointer to the unixfd struct on success,
// NULL on error with errno set appropriately.
// Errors are:
//	EBADF: bad file descriptor - fd was either not in range or not open.
unixfd *
unixfd_lookup(int fd)
{
	unixfd *ufd = &unixstate->fd[fd];
	if (fd < 0 || fd >= OPEN_MAX || ufd->dev == NULL) {
		warn("bad fd %d\n", fd);
		errno = EBADF;
		return NULL;
	}
	return ufd;
}

// Shorthand: lookup an open ufd, and return an error if invalid
#define LOOKUP(fd,ufd) \
	unixfd *ufd = unixfd_lookup(fd); \
	if (ufd == NULL) return -1;

int
close(int fd)
{
	LOOKUP(fd, ufd)
	return fclose(ufd);
}

int
dup(int oldfd)
{
	int newfd = unixfd_alloc();
	if (newfd < 0)
		return -1;

	return dup2(oldfd, newfd);
}

// Make file descriptor 'newfd' a duplicate of file descriptor 'oldfd'.
// For instance, writing onto either file descriptor will affect the
// file and the file offset of the other.
// Closes any previously open file descriptor at 'newfd'.
int
dup2(int oldfd, int newfd)
{
	LOOKUP(oldfd, oldufd)

	// Validate and lookup the new file descriptor
	unixfd *newufd = &unixstate->fd[newfd];
	if (newfd < 0 || newfd >= OPEN_MAX) {
		warn("bad fd %d\n", fd);
		errno = EBADF;
		return -1;
	}

	// Close newfd if it was open
	if (newufd->dev != NULL)
		newufd->dev->ref(newufd, -1);	// drop the newfd's reference

	// Copy oldufd to newufd
	*newufd = *oldufd;
	newufd->dev->ref(newufd, 1);	// add a reference
	return newfd;
}

ssize_t
read(int fd, void *buf, size_t nbytes)
{
	LOOKUP(fd, ufd)

	// Read starting at fd's current offset, then adjust the offset
	ssize_t actual = ufd->dev->read(ufd, buf, nbytes, ufd->ofs);
	if (actual >= 0)
		ufd->ofs += actual;

	return actual;
}

ssize_t
write(int fd, const void *buf, size_t n)
{
	LOOKUP(fd, ufd)

	// Write starting at fd's current offset, then adjust the offset
	ssize_t actual = ufd->dev->write(ufd, buf, nbytes, ufd->ofs);
	if (actual >= 0)
		ufd->ofs += actual;

	return actual;
}

off_t
lseek(int fd, off_t offset, int whence)
{
	LOOKUP(fd, ufd)

	if (ufd->dev->seek(ufd, offset, whence) < 0)
		return -1;
	return ufd->ofs;
}

int
ftruncate(int fd, off_t newsize)
{
	LOOKUP(fd, ufd)

	return ufd->dev->trunc(ufd, newsize);
}

int
fstat(int fd, struct stat *st)
{
	LOOKUP(fd, ufd)

	// Initialize the stat buffer to zero;
	// dev->stat only fills in relevant fields.
	memset(st, 0, sizeof(*st));

	return ufd->dev->stat(ufd, st);
}

int
stat(const char *path, struct Stat *stat)
{
	int fd, r;

	if ((fd = open(path, O_RDONLY)) < 0)
		return fd;
	r = fstat(fd, stat);
	close(fd);
	return r;
}

#endif /* LAB >= 4 */
