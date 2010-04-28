#if LAB >= 9
// Unix compatibility API - file control functions
#ifndef PIOS_INC_FCNTL_H
#define PIOS_INC_FCNTL_H 1

#include <types.h>
#include <unistd.h>


// File control commands passed to fcntl()
#define F_DUPFD		1	// Duplicate file descriptor
#define F_GETFD		2	// Get file descriptor flags
#define F_SETFD		3	// Set file descriptor flags
#define F_GETFL		4	// Get file status flags and access modes
#define F_SETFL		5	// Set file status flags
#define F_GETLK		6	// Get record locking information
#define F_SETLK		7	// Set record locking information
#define F_SETLKW	8	// Set record lock, wait if blocked
#define F_GETOWN	9	// Get process or group to receive SIGURG sigs
#define F_SETOWN	10	// Set process or group to receive SIGURG sigs

#define FD_CLOEXEC	0x01	// Close-on-exec flag


// File control functions
int	open(const char *path, int flags, ...);
int	creat(const char *path, mode_t mode);
int	fcntl(int, int, ...);

#endif	// !PIOS_INC_FCNTL_H
#endif	// LAB >= 9
