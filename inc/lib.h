#if LAB >= 3

// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef _INC_LIB_H_
#define _INC_LIB_H_ 1

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/pmap.h>
#include <inc/syscall.h>
#if LAB >= 5
#include <inc/fs.h>
#include <inc/libfd.h>
#include <inc/args.h>
#endif

#define USED(x) (void)(x)

// libos.c or entry.S
extern char *binaryname;
extern struct Env *env;
extern struct Env envs[NENV];
extern struct Page pages[];
void	exit(void);

// pgfault.c
void	set_pgfault_handler(void(*)(u_int va, u_int err));

// readline.c
char *	readline(const char *buf);

// syscall.c
void	sys_cputs(char*);
int	sys_cgetc(void);
u_int	sys_getenvid(void);
int	sys_env_destroy(u_int);
int	sys_set_pgfault_handler(u_int, u_int);
int	sys_mem_alloc(u_int, u_int, u_int);
int	sys_mem_map(u_int, u_int, u_int, u_int, u_int);
int	sys_mem_unmap(u_int, u_int);
#if LAB >= 4
// int	sys_env_alloc(void);
int	sys_set_trapframe(u_int, struct Trapframe*);
int	sys_set_status(u_int, u_int);
void	sys_yield(void);
int	sys_ipc_can_send(u_int, u_int, u_int, u_int);
void	sys_ipc_recv(u_int);

// This must be inlined.  
// Exercise for reader: why?
static inline int
sys_env_alloc(void)
{
	int ret;

	asm volatile("int %2"
		: "=a" (ret)
		: "a" (SYS_env_alloc),
		  "i" (T_SYSCALL)
	);
	return ret;
}

// ipc.c
void	ipc_send(u_int whom, u_int val, u_int srcva, u_int perm);
u_int	ipc_recv(u_int *whom, u_int dstva, u_int *perm);

// fork.c
#define	PTE_LIBRARY	0x400
int	fork(void);
int	sfork(void);	// Challenge!
#endif	// LAB >= 4

#if LAB >= 5
// fd.c
int	close(int fd);
int	read(int fd, void *buf, u_int nbytes);
int	write(int fd, const void *buf, u_int nbytes);
int	seek(int fd, u_int offset);
void	close_all(void);
int	readn(int fd, void *buf, u_int nbytes);
int	dup(int oldfd, int newfd);
int	fstat(int fd, struct Stat*);
int	stat(const char *path, struct Stat*);

// file.c
int	open(const char *path, int mode);
int	read_map(int fd, u_int offset, void **blk);
int	delete(const char *path);
int	ftruncate(int fd, u_int size);
int	sync(void);

// fprintf.c
int	fprintf(int fd, const char*, ...);

// fsipc.c
int	fsipc_open(const char*, u_int, struct Fd*);
int	fsipc_map(u_int, u_int, u_int);
int	fsipc_set_size(u_int, u_int);
int	fsipc_close(u_int);
int	fsipc_dirty(u_int, u_int);
int	fsipc_remove(const char*);
int	fsipc_sync(void);
int	fsipc_incref(u_int);

// pageref.c
int	pageref(void*);

// spawn.c
int	spawn(char*, char**);
int	spawnl(char*, char*, ...);
#endif  // LAB >= 5

#if LAB >= 6
// console.c
void	putchar(int c);
int	getchar(void);
int	iscons(int);
int	opencons(void);

// pipe.c
int	pipe(int[2]);
int	pipeisclosed(int);

// wait.c
void	wait(u_int);
#endif  // LAB >= 6

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

#endif	// not _INC_LIB_H_
#endif	// LAB >= 3
