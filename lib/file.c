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


// Although 'files' itself could be a preprocessor symbol like FILES,
// that makes this symbol invisible to and unusable under GDB,
// which is a bit of a pain for debugging purposes.
// This way 'files' is a real pointer variable that GDB knows about.
filestate *const files = FILES;


////////// File inode functions //////////

// Find and return the index of a currently unused file inode in this process.
// If no inodes are available, returns -1 and sets errno accordingly.
int
fileino_alloc(void)
{
	int i;
	for (i = FILEINO_GENERAL; i < FILE_INODES; i++)
		if (files->fi[i].de.d_name[0] == 0)
			return i;

	warn("fileino_alloc: no free inodes\n");
	errno = ENOSPC;
	return -1;
}

// Find or create an inode with a given parent directory inode and filename.
// Returns the index of the inode found or created.
// A newly-created inode is left in the "deleted" state, with mode == 0.
// If no inodes are available, returns -1 and sets errno accordingly.
int
fileino_create(filestate *fs, int dino, const char *name)
{
	assert(dino != 0);
	assert(name != NULL && name[0] != 0);
	assert(strlen(name) <= NAME_MAX);

	// First see if an inode already exists for this directory and name.
	int i;
	for (i = FILEINO_GENERAL; i < FILE_INODES; i++)
		if (fs->fi[i].dino == dino
				&& strcmp(fs->fi[i].de.d_name, name) == 0)
			return i;

	// No inode allocated to this name - find a free one to allocate.
	for (i = FILEINO_GENERAL; i < FILE_INODES; i++)
		if (fs->fi[i].de.d_name[0] == 0) {
			fs->fi[i].dino = dino;
			strcpy(fs->fi[i].de.d_name, name);
			return i;
		}

	warn("fileino_create: no free inodes\n");
	errno = ENOSPC;
	return -1;
}

// Read up to 'count' data elements each of size 'eltsize',
// starting at absolute byte offset 'ofs' within the file in inode 'ino'.
// Returns the number of elements (NOT the number of bytes!) actually read,
// or if an error occurs, returns -1 and sets errno appropriately.
// The number of elements returned is normally equal to the 'count' parameter,
// but may be less (without resulting in an error)
// if the file is not large enough to read that many elements.
ssize_t
fileino_read(int ino, off_t ofs, void *buf, size_t eltsize, size_t count)
{
	assert(fileino_isreg(ino));
	assert(ofs >= 0);
	assert(eltsize > 0);

	fileinode *fi = &files->fi[ino];
	assert(fi->size <= FILE_MAXSIZE);

#if SOL >= 4
	ssize_t actual = 0;
	while (count > 0) {
		// Read as many elements as we can from the file.
		// Note: fd->ofs could well point beyond the end of file,
		// which means that avail will be negative - but that's OK.
		ssize_t avail = MIN(count, (fi->size - ofs) / eltsize);
		if (ofs >= fi->size)
			avail = 0;
		if (avail > 0) {
			memmove(buf, FILEDATA(ino) + ofs, avail * eltsize);
			buf += avail * eltsize;
			actual += avail;
			count -= avail;
		}

		// If there's no more we can read, stop now.
		if (count == 0 || !(fi->mode & S_IFPART))
			break;

		// Wait for our parent to extend (or close) the file.
#if LAB >= 99
		cprintf("fileino_read: waiting for input on file %d\n",
			fd - files->fd);
#endif
		sys_ret();
	}
	return actual;
#else	// ! SOL >= 4
	// Lab 4: insert your file reading code here.
	warn("fileino_read() not implemented");
	errno = EINVAL;
	return -1;
#endif	// ! SOL >= 4
}

// Write 'count' data elements each of size 'eltsize'
// starting at absolute byte offset 'ofs' within the file in inode 'ino'.
// Returns the number of elements actually written,
// which should always be equal to the 'count' input parameter
// unless an error occurs, in which case this function
// returns -1 and sets errno appropriately.
// Since PIOS files can be up to only FILE_MAXSIZE bytes in size (4MB),
// one particular reason an error might occur is if an application
// tries to grow a file beyond this maximum file size,
// in which case this function generates the EFBIG error.
ssize_t
fileino_write(int ino, off_t ofs, const void *buf, size_t eltsize, size_t count)
{
	assert(fileino_isreg(ino));
	assert(ofs >= 0);
	assert(eltsize > 0);

	fileinode *fi = &files->fi[ino];
	assert(fi->size <= FILE_MAXSIZE);

#if SOL >= 4
	// Return an error if we'd be growing the file too big.
	size_t len = eltsize * count;
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
	return count;
#else	// ! SOL >= 4
	// Lab 4: insert your file writing code here.
	warn("fileino_write() not implemented");
	errno = EINVAL;
	return -1;
#endif	// ! SOL >= 4
}

// Return file statistics about a particular inode.
// The specified inode must indicate a file that exists,
// but it can be any type of object: e.g., file, directory, special file, etc.
int
fileino_stat(int ino, struct stat *st)
{
	assert(fileino_exists(ino));

	fileinode *fi = &files->fi[ino];
	assert(fileino_isdir(fi->dino));	// Should be in a directory!
#if LAB >= 9
	memset(st, 0, sizeof(*st));		// Clear unused parts of struct
#endif
	st->st_ino = ino;
	st->st_mode = fi->mode;
	st->st_size = fi->size;

	return 0;
}

// Grow or shrink a file to exactly a specified size.
// If growing a file, then fills the new space with zeros.
// Returns 0 if successful, or returns -1 and sets errno on error.
int
fileino_truncate(int ino, off_t newsize)
{
	assert(fileino_isvalid(ino));
	assert(newsize >= 0 && newsize <= FILE_MAXSIZE);

	size_t oldsize = files->fi[ino].size;
	size_t oldpagelim = ROUNDUP(files->fi[ino].size, PAGESIZE);
	size_t newpagelim = ROUNDUP(newsize, PAGESIZE);
	if (newsize > oldsize) {
		// Grow the file and fill the new space with zeros.
		sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL,
			FILEDATA(ino) + oldpagelim,
			newpagelim - oldpagelim);
		memset(FILEDATA(ino) + oldsize, 0, newsize - oldsize);
	} else if (newsize > 0) {
		// Shrink the file, but not all the way to empty.
		// Would prefer to use SYS_ZERO to free the file content,
		// but SYS_ZERO isn't guaranteed to work at page granularity.
		sys_get(SYS_PERM, 0, NULL, NULL,
			FILEDATA(ino) + newpagelim, FILE_MAXSIZE - newpagelim);
	} else {
		// Shrink the file to empty.  Use SYS_ZERO to free completely.
		sys_get(SYS_ZERO, 0, NULL, NULL, FILEDATA(ino), FILE_MAXSIZE);
	}
	files->fi[ino].size = newsize;
	files->fi[ino].ver++;	// truncation is always an exclusive change
	return 0;
}

// Flush any outstanding writes on this file to our parent process.
// (XXX should flushes propagate across multiple levels?)
int
fileino_flush(int ino)
{
	assert(fileino_isvalid(ino));

	if (files->fi[ino].size > files->fi[ino].rlen)
		sys_ret();	// synchronize and reconcile with parent
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
filedesc_open(filedesc *fd, const char *path, int openflags, mode_t mode)
{
	if (!fd && !(fd = filedesc_alloc()))
		return NULL;
	assert(fd->ino == FILEINO_NULL);

	// Determine the complete file mode if it is to be created.
	mode_t createmode = (openflags & O_CREAT) ? S_IFREG | (mode & 0777) : 0;

	// Walk the directory tree to find the desired directory entry,
	// creating an entry if it doesn't exist and O_CREAT is set.
	int ino = dir_walk(path, createmode);
	if (ino < 0)
		return NULL;
	assert(fileino_exists(ino));

	// Refuse to open conflict-marked files;
	// the user needs to resolve the conflict and clear the conflict flag,
	// or just delete the conflicted file.
	if (files->fi[ino].mode & S_IFCONF) {
		errno = ECONFLICT;
		return NULL;
	}

	// Truncate the file if we were asked to
	if (openflags & O_TRUNC) {
		if (!(openflags & O_WRONLY)) {
			warn("filedesc_open: can't truncate non-writable file");
			errno = EINVAL;
			return NULL;
		}
		if (fileino_truncate(ino, 0) < 0)
			return NULL;
	}

	// Initialize the file descriptor
	fd->ino = ino;
	fd->flags = openflags;
	fd->ofs = (openflags & O_APPEND) ? files->fi[ino].size : 0;
	fd->err = 0;

	assert(filedesc_isopen(fd));
	return fd;
}

// Read up to 'count' objects each of size 'eltsize'
// from the open file described by 'fd' into memory buffer 'buf',
// whose size must be at least 'count * eltsize' bytes.
// May read fewer than the requested number of objects
// if the end of file is reached, but always an integral number of objects.
// On success, returns the number of objects read (NOT the number of bytes).
// If an error (other than end-of-file) occurs, returns -1 and sets errno.
//
// If the file is a special device input file such as the console,
// this function pretends the file has no end and instead
// uses sys_ret() to wait for the file to extend the special file.
ssize_t
filedesc_read(filedesc *fd, void *buf, size_t eltsize, size_t count)
{
	assert(filedesc_isreadable(fd));
	fileinode *fi = &files->fi[fd->ino];

	ssize_t actual = fileino_read(fd->ino, fd->ofs, buf, eltsize, count);
	if (actual < 0) {
		fd->err = errno;	// save error indication for ferror()
		return -1;
	}

	// Advance the file position
	fd->ofs += eltsize * actual;
	assert(actual == 0 || fi->size >= fd->ofs);

	return actual;
}

// Write up to 'count' objects each of size 'eltsize'
// from memory buffer 'buf' to the open file described by 'fd'.
// The size of 'buf' must be at least 'count * eltsize' bytes.
// On success, returns the number of objects written (NOT the number of bytes).
// If an error occurs, returns -1 and sets errno appropriately.
ssize_t
filedesc_write(filedesc *fd, const void *buf, size_t eltsize, size_t count)
{
	assert(filedesc_iswritable(fd));
	fileinode *fi = &files->fi[fd->ino];

	// If we're appending to the file, seek to the end first.
	if (fd->flags & O_APPEND)
		fd->ofs = fi->size;

	// Write the data, growing the file as necessary.
	ssize_t actual = fileino_write(fd->ino, fd->ofs, buf, eltsize, count);
	if (actual < 0) {
		fd->err = errno;	// save error indication for ferror()
		return -1;
	}
	assert(actual == count);

	// Non-append-only writes constitute exclusive modifications,
	// so must bump the file's version number.
	if (!(fd->flags & O_APPEND))
		fi->ver++;

	// Advance the file position
	fd->ofs += eltsize * count;
	assert(fi->size >= fd->ofs);

	return count;
}

// Seek the given file descriptor to a specificied position,
// which may be relative to the file start, end, or corrent position,
// depending on 'whence' (SEEK_SET, SEEK_CUR, or SEEK_END).
// Returns the resulting absolute file position,
// or returns -1 and sets errno appropriately on error.
off_t filedesc_seek(filedesc *fd, off_t offset, int whence)
{
	assert(filedesc_isopen(fd));
	assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
	fileinode *fi = &files->fi[fd->ino];

#if SOL >= 4
	off_t newofs = offset;
	if (whence == SEEK_CUR)
		newofs += fd->ofs;
	else if (whence == SEEK_END)
		newofs += fi->size;
	assert(newofs >= 0);

	fd->ofs = newofs;
	return newofs;
#else	// ! SOL >= 4
	// Lab 4: insert your file descriptor seek implementation here.
	warn("filedesc_seek() not implemented");
	errno = EINVAL;
	return -1;
#endif	// ! SOL >= 4
}

void
filedesc_close(filedesc *fd)
{
	assert(filedesc_isopen(fd));
	assert(fileino_isvalid(fd->ino));

	fd->ino = FILEINO_NULL;		// mark the fd free
}

#endif /* LAB >= 4 */
