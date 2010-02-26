#if LAB >= 99
// File control definitions in Unix-compatibility API.
#ifndef PIOS_INC_FCNTL_H
#define PIOS_INC_FCNTL_H 1

#include <inc/types.h>
#include <inc/file.h>


int	creat(const char *path, mode_t mode);
int	open(const char *path, int flags, ...);
int	close(int fd);

#endif	// !PIOS_INC_FCNTL_H
#endif	// LAB >= 4
