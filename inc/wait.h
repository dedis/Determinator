#if LAB >= 4
/*
 * Unix compatibility API - standard Unix "low-level" file I/O functions.
 * In PIOS, the "low-level" and "high-level" APIs actually refer to
 * the same set of file descriptors, just accessed slightly differently.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#ifndef PIOS_INC_UNISTD_H
#define PIOS_INC_UNISTD_H 1

#include <types.h>
#include <cdefs.h>
#if LAB >= 9
#include <sysconf.h>
#endif


#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifndef SEEK_SET
#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#endif

// Exit status codes from wait() and waitpid().
// These are traditionally in <sys/wait.h>.
#define WEXITED			0x0100	// Process exited via exit()
#define WSIGNALED		0x0200	// Process exited via uncaught signal
#if LAB >= 9
#define WSTOPPED		0x0400	// Process stopped due to a signal
#define WCONTINUED		0x0800	// Process continued due after stopping
#define WNOHANG			0x1000	// Do not hang if no status available
#define WNOWAIT			0x2000	// Peek status without collecting it
#define WUNTRACED		0x4000	// Report status of stopped child
#endif

#define WIFEXITED(x)		(((x) & 0xf00) == WEXITED)
#define WEXITSTATUS(x)		((x) & 0xff)
#define WIFSIGNALED(x)		(((x) & 0xf00) == WSIGNALED)
#define WTERMSIG(x)		((x) & 0xff)
#if LAB >= 9
#define WIFSTOPPED(x)		(((x) & 0xf00) == WSTOPPED)
#define WIFCONTINUED(x)		(((x) & 0xf00) == WCONTINUED)
#define WSTOPSIG(x)		((x) & 0xff)

// Constants for the access() function
#define F_OK		0	// test for file existence
#define R_OK		4	// test for read permission
#define W_OK		2	// test for write permission
#define X_OK		1	// test for execute/search permission
#endif	// LAB >= 9


// Process management functions
pid_t	fork(void);
pid_t	wait(int *status);				// trad. in sys/wait.h
pid_t	waitpid(pid_t pid, int *status, int options);	// trad. in sys/wait.h
int	execl(const char *path, const char *arg0, ...);
int	execv(const char *path, char *const argv[]);
#if LAB >= 9
void	_exit(int status) gcc_noreturn;
uid_t	getuid(void);
gid_t	getgid(void);
uid_t	geteuid(void);
gid_t	getegid(void);
pid_t	getpid(void);
pid_t	getppid(void);
pid_t	getpgid(pid_t);
pid_t	getpgrp(void);
int	setuid(uid_t uid);
int	setgid(gid_t gid);
int	seteuid(uid_t uid);
int	setegid(gid_t gid);
int	setreuid(uid_t ruid, uid_t euid);
int	setregid(gid_t rgid, gid_t egid);
int	setpgid(gid_t, gid_t);
pid_t	setpgrp(void);
#endif

// File management functions
int	open(const char *path, int flags, ...);		// trad. in fcntl.h
int	creat(const char *path, mode_t mode);		// trad. in fcntl.h
int	close(int fn);
ssize_t	read(int fn, void *buf, size_t nbytes);
ssize_t	write(int fn, const void *buf, size_t nbytes);
off_t	lseek(int fn, off_t offset, int whence);
int	dup(int oldfn);
int	dup2(int oldfn, int newfn);
int	truncate(const char *path, off_t newlength);
int	ftruncate(int fn, off_t newlength);
int	isatty(int fn);
int	remove(const char *path);
int	fsync(int fn);

#if LAB >= 9
int	access(const char *path, int accessmode);
int	chdir(const char *path);
int	fchdir(int fn);
int	rmdir(const char *path);
char *	getcwd(char *buf, size_t bufsize);
int	chown(const char *path, uid_t uid, gid_t gid);
int	lchown(const char *path, uid_t uid, gid_t gid);
int	fchown(int fn, uid_t uid, gid_t gid);
int	fdatasync(int fn);
int	link(const char *oldpath, const char *newpath);
int	symlink(const char *oldpath, const char *newpath);
ssize_t	readlink(const char *path, char *buf, size_t bufsize);
int	unlink(const char *path);
int	pipe(int fds[2]);
char *	mktemp(char *template);
int	mkstemp(char *template);

// Program convenience functions
int	getopt(int argc, char * argv[], const char * optstring);
#endif	// LAB >= 9

// PIOS-specific thread fork/join functions
int	tfork(uint16_t child);
void	tjoin(uint16_t child);
#if LAB >= 9
void	tparallel_begin(int * master, int num_children,
			void * (* start_routine)(void *),
			void * args, int status_array[]);
void	tparallel_end(int master);
#endif


#endif	// !PIOS_INC_UNISTD_H
#endif	// LAB >= 4
