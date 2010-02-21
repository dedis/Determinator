#if LAB >= 4
////////// "High-level" C standard I/O functions //////////
// In a normal Unix C library, these would implement user-space buffering
// on top of the native low-level system calls (open, close, read, write).
// But since our "low-level" file operations are also done in user space,
// there's really no need for separate high-level and low-level sets.
// Therefore, the FILE structs used by the high-level functions
// just point to the same unixfd structs that low-level fds refer to.

#include <inc/unix.h>
#include <inc/stdio.h>


int
fclose(FILE *ufd)
{
	assert(unixfd_open(ufd));
	ufd->dev->ref(ufd, -1);		// drop the fd's reference
	ufd->dev = NULL;		// mark the fd free
	return 0;
}

size_t
fread(void *buf, size_t size, size_t count, FILE *ufd)
{
	assert(unixfd_open(ufd));
	ssize_t actual = ufd->dev->read(ufd, buf, size * count, ufd->ofs);
	if (actual < 0)
		return 0;
	ufd->ofs += actual;
	return actual / size;
}

size_t
fwrite(const void *buf, size_t size, size_t count, FILE *ufd)
{
	assert(unixfd_open(ufd));
	ssize_t actual = ufd->dev->write(ufd, buf, size * count, ufd->ofs);
	if (actual < 0)
		return 0;
	ufd->ofs += actual;
	return actual / size;
}

int
fseek(FILE *ufd, off_t offset, int whence)
{
	assert(unixfd_open(ufd));
	assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
	return ufd->dev->seek(ufd, offset, whence);
}

long
ftell(FILE *fh)
{
	assert(unixfd_open(ufd));
	return ufd->ofs;
}

#endif /* LAB >= 4 */
