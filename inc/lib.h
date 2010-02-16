#if LAB >= 99
// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef PIOS_INC_LIB_H
#define PIOS_INC_LIB_H 1

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#if LAB >= 4
#include <inc/trap.h>
#endif
#if LAB >= 5
#include <inc/fs.h>
#include <inc/fd.h>
#include <inc/args.h>
#endif
#if LAB >= 6
#include <inc/malloc.h>
#include <inc/ns.h>
#endif

void	exit(void);

// readline.c
char*	readline(const char *buf);

#if LAB >= 5
// fd.c
int	close(int fd);
ssize_t	read(int fd, void *buf, size_t nbytes);
ssize_t	write(int fd, const void *buf, size_t nbytes);
int	seek(int fd, off_t offset);
void	close_all(void);
ssize_t	readn(int fd, void *buf, size_t nbytes);
int	dup(int oldfd, int newfd);
int	fstat(int fd, struct Stat *statbuf);
int	stat(const char *path, struct Stat *statbuf);

// file.c
int	open(const char *path, int mode);
int	read_map(int fd, off_t offset, void **blk);
int	ftruncate(int fd, off_t size);
int	remove(const char *path);
int	sync(void);

// fsipc.c
int	fsipc_open(const char *path, int omode, struct Fd *fd);
int	fsipc_map(int fileid, off_t offset, void *dst_va);
int	fsipc_set_size(int fileid, off_t size);
int	fsipc_close(int fileid);
int	fsipc_dirty(int fileid, off_t offset);
int	fsipc_remove(const char *path);
int	fsipc_sync(void);

#if LAB >= 6
// sockets.c
int     accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int     bind(int s, struct sockaddr *name, socklen_t namelen);
int     shutdown(int s, int how);
int     closesocket(int s);
int     connect(int s, const struct sockaddr *name, socklen_t namelen);
int     listen(int s, int backlog);
int     recv(int s, void *mem, int len, unsigned int flags);
int     send(int s, const void *dataptr, int size, unsigned int flags);
int     socket(int domain, int type, int protocol);

// nsipc.c
int     nsipc_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int     nsipc_bind(int s, struct sockaddr *name, socklen_t namelen);
int     nsipc_shutdown(int s, int how);
int     nsipc_close(int s);
int     nsipc_connect(int s, const struct sockaddr *name, socklen_t namelen);
int     nsipc_listen(int s, int backlog);
int     nsipc_recv(int s, void *mem, int len, unsigned int flags);
int     nsipc_send(int s, const void *dataptr, int size, unsigned int flags);
int     nsipc_socket(int domain, int type, int protocol);
#endif
// pageref.c
int	pageref(void *addr);

// spawn.c
envid_t	spawn(const char *program, const char **argv);
envid_t	spawnl(const char *program, const char *arg0, ...);
#endif  // LAB >= 5

#if LAB >= 7
// console.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);
int	opencons(void);

// pipe.c
int	pipe(int pipefds[2]);
int	pipeisclosed(int pipefd);

// wait.c
void	wait(envid_t env);
#endif  // LAB >= 7

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

#endif	// !PIOS_INC_LIB_H
#endif	// LAB >= 3
