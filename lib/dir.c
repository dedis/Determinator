#if LAB >= 4
/*
 * Directory walking and scanning for the PIOS user-space file system.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#include <inc/file.h>
#include <inc/stat.h>
#include <inc/errno.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/dirent.h>


int
dir_walk(const char *path, mode_t createmode)
{
	assert(path != 0 && *path != 0);

	// Start at the current or root directory as appropriate
	int dino = files->cwd;
	if (*path == '/') {
		dino = FILEINO_ROOTDIR;
		do { path++; } while (*path == '/');	// skip leading slashes
		if (*path == 0)
			return dino;	// Just looking up root directory
	}

	// Search for the appropriate entry in this directory
	searchdir:
	assert(fileino_isdir(dino));
	assert(fileino_isdir(files->fi[dino].dino));

	// Look for a regular directory entry with a matching name.
	int ino, len;
	for (ino = 1; ino < FILE_INODES; ino++) {
		if (!fileino_alloced(ino) || files->fi[ino].dino != dino)
			continue;	// not an entry in directory 'dino'

		// Does this inode's name match our next path component?
		len = strlen(files->fi[ino].de.d_name);
		if (memcmp(path, files->fi[ino].de.d_name, len) != 0)
			continue;	// no match
		found:
		if (path[len] == 0) {
			// Exact match at end of path - but does it exist?
			if (fileino_exists(ino))
				return ino;	// yes - return it

			// no - existed, but was deleted.  re-create?
			if (!createmode) {
				errno = ENOENT;
				return -1;
			}
			files->fi[ino].ver++;	// an exclusive change
			files->fi[ino].mode = createmode;
			files->fi[ino].size = 0;
			return ino;
		}
		if (path[len] != '/')
			continue;	// no match

		// Make sure this dirent refers to a directory
		if (!fileino_isdir(ino)) {
			errno = ENOTDIR;
			return -1;
		}

		// Skip slashes to find next component
		do { len++; } while (path[len] == '/');
		if (path[len] == 0)
			return ino;	// matched directory at end of path

		// Walk the next directory in the path
		dino = ino;
		path += len;
		goto searchdir;
	}

	// Looking for one of the special entries '.' or '..'?
	if (path[0] == '.' && (path[1] == 0 || path[1] == '/')) {
		len = 1;
		ino = dino;	// just leads to this same directory
		goto found;
	}
	if (path[0] == '.' && path[1] == '.'
			&& (path[2] == 0 || path[2] == '/')) {
		len = 2;
		ino = files->fi[dino].dino;	// leads to root directory
		goto found;
	}

	// Path component not found - see if we should create it
	if (!createmode || strchr(path, '/') != NULL) {
		errno = ENOENT;
		return -1;
	}
	if (strlen(path) > NAME_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	// Allocate a new inode and create this entry with the given mode.
	ino = fileino_alloc();
	if (ino < 0)
		return -1;
	assert(fileino_isvalid(ino) && !fileino_alloced(ino));
	strcpy(files->fi[ino].de.d_name, path);
	files->fi[ino].dino = dino;
	files->fi[ino].ver = 0;
	files->fi[ino].mode = createmode;
	files->fi[ino].size = 0;
	return ino;
}

// Open a directory for scanning.
// For simplicity, DIR is simply a filedesc like other file descriptors,
// except we interpret fd->ofs as an inode number for scanning,
// instead of as a byte offset as in a regular file.
DIR *opendir(const char *path)
{
	filedesc *fd = filedesc_open(NULL, path, O_RDONLY, 0);
	if (fd == NULL)
		return NULL;

	// Make sure it's a directory
	assert(fileino_exists(fd->ino));
	fileinode *fi = &files->fi[fd->ino];
	if (!S_ISDIR(fi->mode)) {
		filedesc_close(fd);
		errno = ENOTDIR;
		return NULL;
	}

	return fd;
}

int closedir(DIR *dir)
{
	filedesc_close(dir);
	return 0;
}

// Scan an open directory filedesc and return the next entry.
// Returns a pointer to the next matching file inode's 'dirent' struct,
// or NULL if the directory being scanned contains no more entries.
struct dirent *readdir(DIR *dir)
{
#if SOL >= 4
	assert(filedesc_isopen(dir));
	int ino;
	while ((ino = dir->ofs++) < FILE_INODES) {
		if (!fileino_exists(ino) || files->fi[ino].dino != dir->ino)
			continue;
		return &files->fi[ino].de;	// Return inode's dirent
	}
	return NULL;	// End of directory
#else	// ! SOL >= 4
	// Lab 4: insert your directory scanning code here.
	// Hint: a fileinode's 'dino' field indicates
	// what directory the file is in;
	// this function shouldn't return entries from other directories!
	warn("readdir() not implemented");
	return NULL;
#endif	// ! SOL >= 4
}

void rewinddir(DIR *dir)
{
	dir->ofs = 0;
}

void seekdir(DIR *dir, long ofs)
{
	dir->ofs = ofs;
}

long telldir(DIR *dir)
{
	return dir->ofs;
}

#if LAB >= 9
int rename(const char *oldpath, const char *newpath)
{
	panic("rename() not implemented");
}

int unlink(const char *path)
{
	warn("unlink() not implemented");
	errno = ENOSYS;
	return -1;
}
#endif

#endif	// LAB >= 4
