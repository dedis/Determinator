#if LAB >= 4

#include <inc/file.h>
#include <inc/stat.h>
#include <inc/errno.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/dirent.h>


struct dirent *
dir_walk(const char *path, bool create)
{
	assert(path != 0 && *path != 0);

	// Start at the current or root directory as appropriate
	int ino = (*path == '/') ? FILEINO_ROOTDIR : files->cwd;
	while (*path == '/')
		path++;		// skip leading slashes

	// Search for the appropriate entry in this directory
	searchdir:
	assert(fileino_isdir(ino));
	struct dirent *de = FILEDATA(ino);
	struct dirent *delim = de + files->fi[ino].size / sizeof(*de);
	struct dirent *defree = NULL;
	for (; de < delim; de++) {
		if (de->d_ino == FILEINO_NULL) {
			if (defree == NULL)
				defree = de;
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
		do { len++; } while (path[len] == '/');
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
	if (fileino_write(ino, (void*)defree - FILEDATA(ino), defree,
				sizeof(*de)) < 0)
		return NULL;
	return defree;
}

DIR *opendir(const char *path)
{
	filedesc *fd = filedesc_open(NULL, path, O_RDONLY);
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
	// Read a directory entry into the file descriptor's dirent struct.
	ssize_t actual = filedesc_read(dir, &dir->de, sizeof(dir->de), 1);
	if (actual < 1)
		return NULL;
	return &dir->de;
}

void rewinddir(DIR *dir)
{
	filedesc_seek(dir, 0, SEEK_SET);
}

void seekdir(DIR *dir, long ofs)
{
	filedesc_seek(dir, ofs, SEEK_SET);
}

long telldir(DIR *dir)
{
	return dir->ofs;
}

#endif	// LAB >= 4
