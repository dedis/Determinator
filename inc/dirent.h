#if LAB >= 4
// Directory entry (dirent) structure for the user-space Unix file system.
#ifndef PIOS_INC_DIRENT_H
#define PIOS_INC_DIRENT_H 1

#include <inc/file.h>		// defines struct dirent


typedef struct filedesc DIR;	// A DIR in PIOS is just a file descriptor

struct dirent *dir_walk(const char *path, bool create);

DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
void seekdir(DIR *dir, long ofs);
long telldir(DIR *dir);

#endif	// !PIOS_INC_DIRENT_H
#endif	// LAB >= 4
