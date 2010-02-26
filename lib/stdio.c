#if LAB >= 4
////////// "High-level" C standard I/O functions //////////
// In a normal Unix C library, these would implement user-space buffering
// on top of the native low-level system calls (open, close, read, write).
// But since our "low-level" file operations are also done in user space,
// there's really no need for separate high-level and low-level sets.
// Therefore, the FILE structs used by the high-level functions
// just point to the same filedesc structs that low-level fds refer to.

#include <inc/file.h>
#include <inc/stdio.h>
#include <inc/dirent.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>


FILE *const stdin = &files->fd[0];
FILE *const stdout = &files->fd[1];
FILE *const stderr = &files->fd[2];

int
filewrite(int ino, off_t ofs, void *buf, int len)
{
	assert(fileino_exists(ino));
	assert(ofs >= 0);
	assert(len >= 0);

	fileinode *fi = &files->fi[fd->ino];
	assert(fi->size <= FILE_MAXSIZE);

	// Return an error if we'd be growing the file too big.
	size_t lim = ofs + len;
	if (lim < ofs || lim > FILE_MAXSIZE) {
		fd->err = errno = EFBIG;
		return 0;
	}

	// Grow the file as necessary.
	if (lim > fi->size) {
		size_t oldpagelim = ROUNDUP(fi->size, PAGESIZE);
		size_t newpagelim = ROUNDUP(newsize, PAGESIZE);
		if (newpagelim > oldpagelim)
			sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL,
				FILEDATA(fd->ino) + oldpagelim,
				newpagelim - oldpagelim);
		fi->size = lim;
	}

	// Write the data.
	memmove(FILEDATA(fd->ino) + ofs, buf, len);
	return 1;
}

static struct dirent *
dirwalk(const char *path, bool create)
{
	assert(path != 0 && *path != 0);

	// Start at the current or root directory as appropriate
	int ino = (path == '/') ? FILEINO_ROOT : files->cwd;

	// Search for the appropriate entry in this directory
	searchdir:
	assert(fileino_isdir(ino));
	struct dirent *de = FILEDATA(ino);
	struct dirent *delim = de + files[ino]->size / sizeof(*de);
	struct dirent *defree = NULL;
	for (; de < delim; de++) {
		if (de->d_ino == FILEINO_NULL) {
			if (defree == NULL) defree = de;
			continue;
		}
		int len = strlen(de->d_name);
		if (memcmp(path, de->d_name, len) != 0)
			continue;	// no match
		if (path[len] == 0)
			return de;	// exact match at end of path
		if (path[len] == '/')
			continue;	// no match

		// Make sure this dirent refers to a directory
		assert(fileino_exists(de->d_ino));
		if (!fileino_isdir(de->d_ino)) {
			errno = ENOTDIR;
			return NULL;
		}

		// Skip slashes to find next component
		do { len++ } while (path[len] == '/');
		if (path[len] == 0)
			return de;	// matched directory at end of path

		// Walk the next directory in the path
		ino = de->d_ino;
		path += len;
		goto searchdir;
	}

	// Path component not found - see if we should create it
	if (!create || strchr(path, '/') != NULL) {
		errno = ENOENT;
		return NULL;
	}
	if (strlen(path) > NAME_MAX) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	struct dirent denew;
	denew.d_ino = FILEINO_NULL;	// caller will fill in
	strcpy(denew.d_name, path);
	if (!defree)
		defree = delim;
	if (!filewrite(ino, (void*)defree - FILEDATA(ino), defree, sizeof(*de)))
		return NULL;
	return defree;
}

FILE *
fileopen(const char *path, int openflags, FILE *fd)
{
	// Walk the directory tree to find the desired directory entry,
	// creating an entry if it doesn't exist and O_CREAT is set.
	char namebuf[NAME_MAX+1];
	struct dirent *de = dirwalk(path, (flags & O_CREAT) != 0);
	if (de == NULL)
		return NULL;

	// Create the file if necessary
	int ino = de->d_ino;
	if (ino == FILEINO_NULL) {
		assert(flags & O_CREAT);
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
	assert(fd->ino == FILEINO_NULL);
	fd->ino = ino;
	fd->flags = flags;
	fd->ofs = (flags & O_APPEND) ? files->fi[ino].size : 0;
	fd->err = 0;
	files->fi[ino].nlink++;		// file descriptor's reference

	return fd;
}

FILE *
fopen(const char *path, const char *mode)
{
	// Find an unused file descriptor and use it for the open
	int i;
	for (i = 0; i < OPEN_MAX; i++)
		if (files->fd[i].ino == FILEINO_NULL)
			return freopen(filename, mode, &files->fd[i]);

	errno = EMFILE;
	return NULL;
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

	return file_open(path, flags, fd);
}

FILE *
fdup2(FILE *oldfd, FILE *newfd)
{
	assert(filedesc_isopen(oldfd));
	assert(filedesc_isvalid(newfd));

	if (filedesc_isopen(newfd))
		fclose(newfd);

	*newfd = *oldfd;
	assert(fileino_isvalid(oldfd->ino));
	assert(files->fi[oldfd->ino].nlink > 0);
	files->fi[oldfd->ino].nlink++;

	return newfd;
}

int
fclose(FILE *fd)
{
	assert(filedesc_isopen(fd));
	assert(fileino_isvalid(fd->ino));
	assert(files->fi[fd->ino].nlink > 0);
	files->fi[fd->ino].nlink--;
	fd->ino = FILEINO_NULL;		// mark the fd free
	return 0;
}

size_t
fread(void *buf, size_t eltsize, size_t count, FILE *fd)
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
			memcpy(buf, FILEDATA(fd->ino) + fd->ofs,
				avail * eltsize);
			fd->ofs += avail * eltsize;
			buf += avail * eltsize;
			actual += avail;
			count -= avail;
		}
		if (count > 0 && fi->partial) {
			// Wait for our parent to extend (or close) the file.
			cprintf("fread: waiting for input on file %d\n",
				fd - files->fd);
			sys_ret();
		}
	}
	return actual;
}

size_t
fwrite(const void *buf, size_t eltsize, size_t count, FILE *fd)
{
	assert(filedesc_iswritable(fd));
	fileinode *fi = &files->fi[fd->ino];

	// If we're appending to the file, seek to the end first.
	if (fd->flags & FILEDESC_APPEND)
		fd->ofs = fi->size;

	// Write the data, growing the file as necessary.
	if (!filewrite(fd->ino, fd->ofs, buf, eltsize * count))
		return 0;

	// Advance the file position
	fd->ofs += eltsize * count;
	assert(fi->size >= fd->ofs);

	return count;
}

int
fseek(FILE *fd, off_t offset, int whence)
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
	return 0;
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
	return fd->ofs >= fi->size && !fi->endless;
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

#endif /* LAB >= 4 */
