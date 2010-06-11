#if LAB >= 4
/*
 * "High-level" C standard I/O functions for PIOS.
 *
 * In a normal Unix C library, these would implement user-space buffering
 * on top of the native low-level system calls (open, close, read, write).
 * But since our "low-level" file operations are also done in user space,
 * there's really no need for separate high-level and low-level sets.
 * Therefore, the FILE structs used by the high-level functions
 * just point to the same filedesc structs that low-level fds refer to,
 * which simplifies PIOS's user-space C library code a lot.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#include <inc/file.h>
#include <inc/stat.h>
#include <inc/stdio.h>
#include <inc/errno.h>
#include <inc/dirent.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>


FILE *const stdin = &FILES->fd[0];
FILE *const stdout = &FILES->fd[1];
FILE *const stderr = &FILES->fd[2];

FILE *
fopen(const char *path, const char *mode)
{
	// Find an unused file descriptor and use it for the open
	FILE *fd = filedesc_alloc();
	if (fd == NULL)
		return NULL;

	return freopen(path, mode, fd);
}

FILE *
freopen(const char *path, const char *mode, FILE *fd)
{
	assert(filedesc_isvalid(fd));
	if (filedesc_isopen(fd))
		fclose(fd);

	// Parse the open mode string
	int flags;
	switch (*mode++) {
	case 'r':	flags = O_RDONLY; break;
	case 'w':	flags = O_WRONLY | O_CREAT | O_TRUNC; break;
	case 'a':	flags = O_WRONLY | O_CREAT | O_APPEND; break;
	default:	panic("freopen: unknown file mode '%c'\n", *--mode);
	}
	if (*mode == 'b')	// binary flag - compatibility only
		mode++;
	if (*mode == '+')
		flags |= O_RDWR;

	if (filedesc_open(fd, path, flags, 0666) != fd)
		return NULL;
	return fd;
}

int
fclose(FILE *fd)
{
	filedesc_close(fd);
	return 0;
}

int
fgetc(FILE *fd)
{
	unsigned char ch;
	if (filedesc_read(fd, &ch, 1, 1) < 1)
		return EOF;
	return ch;
}

int
fputc(int c, FILE *fd)
{
	unsigned char ch = c;
	if (filedesc_write(fd, &ch, 1, 1) < 1)
		return EOF;
	return ch;
}

size_t
fread(void *buf, size_t eltsize, size_t count, FILE *fd)
{
	ssize_t actual = filedesc_read(fd, buf, eltsize, count);
	return actual >= 0 ? actual : 0;	// no error indication
}

size_t
fwrite(const void *buf, size_t eltsize, size_t count, FILE *fd)
{
	ssize_t actual = filedesc_write(fd, buf, eltsize, count);
	return actual >= 0 ? actual : 0;	// no error indication
}

int
fseek(FILE *fd, off_t offset, int whence)
{
	if (filedesc_seek(fd, offset, whence) < 0)
		return -1;
	return 0;	// fseek() returns 0 on success, not the new position
}

long
ftell(FILE *fd)
{
	assert(filedesc_isopen(fd));
	return fd->ofs;
}

int
feof(FILE *fd)
{
	assert(filedesc_isopen(fd));
	fileinode *fi = &files->fi[fd->ino];
	return fd->ofs >= fi->size && !(fi->mode & S_IFPART);
}

int
ferror(FILE *fd)
{
	assert(filedesc_isopen(fd));
	return fd->err;
}

void
clearerr(FILE *fd)
{
	assert(filedesc_isopen(fd));
	fd->err = 0;
}

#if LAB >= 99
int
fileno(FILE *fd)
{
	assert(filedesc_isopen(fd));
	return fd - files->fd;
}
#endif

int
fflush(FILE *fd)
{
	assert(filedesc_isopen(fd));
	return fileino_flush(fd->ino);
}

#endif /* LAB >= 4 */
