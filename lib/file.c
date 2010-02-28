#if LAB >= 4
// Basic user-space file and I/O support functions,
// used by the standard I/O functions in stdio.c.

#include <inc/file.h>
#include <inc/stat.h>
#include <inc/stdio.h>
#include <inc/dirent.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/errno.h>
#include <inc/mmu.h>


////////// File inode functions //////////

ssize_t
fileino_write(int ino, off_t ofs, const void *buf, int len)
{
	assert(fileino_exists(ino));
	assert(ofs >= 0);
	assert(len >= 0);

	fileinode *fi = &files->fi[ino];
	assert(fi->size <= FILE_MAXSIZE);

	// Return an error if we'd be growing the file too big.
	size_t lim = ofs + len;
	if (lim < ofs || lim > FILE_MAXSIZE) {
		errno = EFBIG;
		return -1;
	}

	// Grow the file as necessary.
	if (lim > fi->size) {
		size_t oldpagelim = ROUNDUP(fi->size, PAGESIZE);
		size_t newpagelim = ROUNDUP(lim, PAGESIZE);
		if (newpagelim > oldpagelim)
			sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL,
				FILEDATA(ino) + oldpagelim,
				newpagelim - oldpagelim);
		fi->size = lim;
	}

	// Write the data.
	memmove(FILEDATA(ino) + ofs, buf, len);
	return len;
}

int
fileino_stat(int ino, struct stat *st)
{
	assert(fileino_exists(ino));

	fileinode *fi = &files->fi[ino];
	st->st_ino = ino;
	st->st_mode = fi->mode;
	st->st_size = fi->size;
	st->st_nlink = fi->nlink;

	return 0;
}


////////// File descriptor functions //////////

// Search the file descriptor table for the first free file descriptor,
// and return a pointer to that file descriptor.
// If no file descriptors are available,
// returns NULL and set errno appropriately.
filedesc *filedesc_alloc(void)
{
	int i;
	for (i = 0; i < OPEN_MAX; i++)
		if (files->fd[i].ino == FILEINO_NULL)
			return &files->fd[i];
	errno = EMFILE;
	return NULL;
}

// Find or create and open a file, optionally using a given file descriptor.
// The argument 'fd' must point to a currently unused file descriptor,
// or may be NULL, in which case this function finds an unused file descriptor.
// The 'openflags' determines whether the file is created, truncated, etc.
// Returns the opened file descriptor on success,
// or returns NULL and sets errno on failure.
filedesc *
filedesc_open(filedesc *fd, const char *path, int openflags)
{
	if (!fd && !(fd = filedesc_alloc()))
		return NULL;
	assert(fd->ino == FILEINO_NULL);

	// Walk the directory tree to find the desired directory entry,
	// creating an entry if it doesn't exist and O_CREAT is set.
	struct dirent *de = dir_walk(path, (openflags & O_CREAT) != 0);
	if (de == NULL)
		return NULL;

	// Create the file if necessary
	int ino = de->d_ino;
	if (ino == FILEINO_NULL) {
		assert(openflags & O_CREAT);
		for (ino = 1; ino < FILE_INODES; ino++)
			if (files->fi[ino].nlink == 0)
				break;
		if (ino == FILE_INODES) {
			warn("freopen: no free inodes\n");
			errno = ENOSPC;
			return NULL;
		}
		files->fi[ino].nlink = 1;	// dirent's reference
		files->fi[ino].size = 0;
		files->fi[ino].psize = -1;
		files->fi[ino].mode = S_IFREG;
		de->d_ino = ino;
	}

	// Initialize the file descriptor
	fd->ino = ino;
	fd->flags = openflags;
	fd->ofs = (openflags & O_APPEND) ? files->fi[ino].size : 0;
	fd->err = 0;
	files->fi[ino].nlink++;		// file descriptor's reference

	return fd;
}

ssize_t
filedesc_read(filedesc *fd, void *buf, size_t eltsize, size_t count)
{
	assert(filedesc_isreadable(fd));
	fileinode *fi = &files->fi[fd->ino];

	ssize_t actual = 0;
	while (count > 0) {
		// Read as many elements as we can from the file.
		// Note: fd->ofs could well point beyond the end of file,
		// which means that avail will be negative - but that's OK.
		ssize_t avail = MIN(count, (fi->size - fd->ofs) / eltsize);
		if (avail > 0) {
			memmove(buf, FILEDATA(fd->ino) + fd->ofs,
				avail * eltsize);
			fd->ofs += avail * eltsize;
			buf += avail * eltsize;
			actual += avail;
			count -= avail;
		}

		// If there's no more we can read, stop now.
		if (count == 0 || !(fi->mode & S_IFPART))
			break;

		// Wait for our parent to extend (or close) the file.
		cprintf("fread: waiting for input on file %d\n",
			fd - files->fd);
		sys_ret();
	}
	return actual;
}

ssize_t
filedesc_write(filedesc *fd, const void *buf, size_t eltsize, size_t count)
{
	assert(filedesc_iswritable(fd));
	fileinode *fi = &files->fi[fd->ino];

	// If we're appending to the file, seek to the end first.
	if (fd->flags & O_APPEND)
		fd->ofs = fi->size;

	// Write the data, growing the file as necessary.
	if (fileino_write(fd->ino, fd->ofs, buf, eltsize * count) < 0) {
		fd->err = errno;	// save error indication for ferror()
		return -1;
	}

	// Advance the file position
	fd->ofs += eltsize * count;
	assert(fi->size >= fd->ofs);

	return count;
}

// Seek the given file descriptor to a specificied position,
// which may be relative to the file start, end, or corrent position,
// depending on 'whence'.
// Returns the resulting absolute file position,
// or returns -1 and sets errno appropriately on error.
off_t filedesc_seek(filedesc *fd, off_t offset, int whence)
{
	assert(filedesc_isopen(fd));
	fileinode *fi = &files->fi[fd->ino];
	assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);

	off_t newofs = offset;
	if (whence == SEEK_CUR)
		newofs += fd->ofs;
	else if (whence == SEEK_END)
		newofs += fi->size;
	assert(newofs >= 0);

	fd->ofs = newofs;
	return newofs;
}

void
filedesc_close(filedesc *fd)
{
	assert(filedesc_isopen(fd));
	assert(fileino_isvalid(fd->ino));
	assert(files->fi[fd->ino].nlink > 0);

	files->fi[fd->ino].nlink--;
	fd->ino = FILEINO_NULL;		// mark the fd free
}

#endif /* LAB >= 4 */
