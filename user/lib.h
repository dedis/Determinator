#if LAB >= 4
#include <inc/types.h>
#include <inc/stdarg.h>
#include <inc/error.h>
#include <inc/env.h>
#include <inc/pmap.h>
#include <inc/syscall.h>


// ipc.c
void	ipc_send(u_int whom, u_int val, u_int srcva, u_int perm);
u_int	ipc_recv(u_int *whom, u_int dstva, u_int *perm);

// printf.c
void	_panic(const char*, int, const char*, ...);
void	printf(const char*, ...);
int	snprintf(char*, int, const char*, ...);
int	vsnprintf(char*, int, const char*, va_list);
void	warn(const char*, ...);
#define assert(x)	\
	do {	if (!(x)) panic("assertion failed: %s", #x); } while (0)
#define	panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

// string.c
void	bcopy(const void*, void*, u_int);
void	bzero(void*, u_int);
char*	strcpy(char*, const char*);
int	strlen(const char*);

// libos.c or entry.S
extern struct Env *env;
extern struct Env envs[NENV];
extern struct Page *pages;

// fork.c
int	fork(void);
int	sfork(void);	// Challenge!

// pgfault.c
void	set_pgfault_handler(void(*)(u_int va, u_int err));

// syscall.c
void	sys_cputs(char*);
// int	sys_env_alloc(void);
void	sys_env_destroy(void);
u_int	sys_getenvid(void);
int	sys_ipc_can_send(u_int, u_int, u_int, u_int);
void	sys_ipc_recv(u_int);
int	sys_set_env_status(u_int, u_int);
int	sys_set_pgfault_handler(u_int, u_int, u_int);
void	sys_yield(void);
int	sys_mem_alloc(u_int, u_int, u_int);
int	sys_mem_map(u_int, u_int, u_int, u_int, u_int);
int	sys_mem_unmap(u_int, u_int);

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

// file.c
int	open(const char *path, int mode);
int	close(int fd);
int	read(int fd, void *buf, int nbytes);
int	write(int fd, const void *buf, int nbytes);
int	delete(const char *path);

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */


#endif
