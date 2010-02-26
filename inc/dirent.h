#if LAB >= 4
// Directory entry (dirent) structure for the user-space Unix file system.
#ifndef PIOS_INC_DIRENT_H
#define PIOS_INC_DIRENT_H 1

#include <inc/limits.h>

struct dirent {				// Directory entry - should be 64 bytes
	int	d_ino;			// File inode number, 0 if free dirent
	char	d_name[NAME_MAX+1];	// Entry name
};

typedef struct unixfiledesc DIR;	// A DIR is just a file descriptor

DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
void seekdir(DIR *dir, long ofs);
long telldir(DIR *dir);

#endif	// !PIOS_INC_DIRENT_H
#endif	// LAB >= 4
