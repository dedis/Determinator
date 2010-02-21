#if LAB >= 4
// File control definitions in Unix-compatibility API.
#ifndef PIOS_INC_FCNTL_H
#define PIOS_INC_FCNTL_H 1

#include <inc/types.h>


/* File open modes */
#define	O_RDONLY	0x0000		// open for reading only
#define	O_WRONLY	0x0001		// open for writing only
#define	O_RDWR		0x0002		// open for reading and writing
#define	O_ACCMODE	0x0003		// mask for above modes

#define O_APPEND	0x0010		// writes always append to file
#define	O_CREAT		0x0020		// create if nonexistent
#define	O_TRUNC		0x0040		// truncate to zero length
#define	O_EXCL		0x0080		// error if already exists
#define O_MKDIR		0x0100		// create directory, not regular file


int	creat(const char *path, mode_t mode);
int	open(const char *path, int flags, ...);
int	close(int fd);

#endif	// !PIOS_INC_FCNTL_H
#endif	// LAB >= 4
