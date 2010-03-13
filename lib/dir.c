#if LAB >= 4

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

	// Not a special directory - look for a regular entry.
	int ino, len;
	for (ino = 1; ino < FILE_INODES; ino++) {
		if (!fileino_alloced(ino) || files->fi[ino].dino != dino)
			continue;	// not an entry in directory 'dino'
		len = strlen(files->fi[ino].de.d_name);
		if (memcmp(path, files->fi[ino].de.d_name, len) != 0)
			continue;	// no match
		found:
		if (path[len] == 0)
			return ino;	// exact match at end of path
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

	// Allocate a new inode for this entry
	// (but leave it in "deleted" state to be filled by the caller).
	ino = fileino_alloc();
	if (ino < 0)
		return -1;
	assert(fileino_isvalid(ino) && !fileino_alloced(ino));
	strcpy(files->fi[ino].de.d_name, path);
	files->fi[ino].dino = dino;
	files->fi[ino].mode = createmode;
	files->fi[ino].size = 0;
	return ino;
}

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

struct dirent *readdir(DIR *dir)
{
	assert(filedesc_isopen(dir));
	int ino;
	while ((ino = dir->ofs++) < FILE_INODES) {
		if (!fileino_exists(ino) || files->fi[ino].dino != dir->ino)
			continue;
		return &files->fi[ino].de;	// Return inode's dirent
	}
	return NULL;	// End of directory
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

#endif	// LAB >= 4
