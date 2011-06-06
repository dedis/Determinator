#if LAB >= 9
// Unix compatibility API - select() definitions
#ifndef PIOS_INC_SELECT_H
#define PIOS_INC_SELECT_H 1

#include <string.h>
#include <file.h>
#include <time.h>

typedef struct {
	uint32_t b[OPEN_MAX / 32];
} fd_set;

#define FD_SETSIZE	OPEN_MAX

static void FD_ZERO(fd_set *fd) {
	memset(fd, 0, sizeof(*fd));
}
static void FD_SET(int fd, fd_set *fs) {
	fs->b[fd >> 5] |= (1 << (fd & 31));
}
static void FD_CLR(int fd, fd_set *fs) {
	fs->b[fd >> 5] &= ~(1 << (fd & 31));
}
static int FD_ISSET(int fd, fd_set *fs) {
	return (fs->b[fd >> 5] & (1 << (fd & 31))) != 0;
}

int select(int nfds, fd_set *rs, fd_set *ws, fd_set *xs,
		struct timeval *timeout);

#endif	// !PIOS_INC_SELECT_H
#endif	// LAB >= 9
