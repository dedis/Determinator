#if LAB >= 4
// File control definitions in Unix-compatibility API.
#ifndef PIOS_INC_FCNTL_H
#define PIOS_INC_FCNTL_H 1

#include <inc/types.h>
#include <inc/file.h>


// unistd.c
int	open(const char *path, int flags, ...);
int	creat(const char *path, mode_t mode);
int	close(int fd);

#endif	// !PIOS_INC_FCNTL_H
#endif	// LAB >= 4
