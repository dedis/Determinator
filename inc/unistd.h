#if LAB >= 4
// Unix compatibility API - standard Unix functions
#ifndef PIOS_INC_UNISTD_H
#define PIOS_INC_UNISTD_H 1

#include <inc/types.h>


#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifndef SEEK_SET
#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#endif


pid_t	fork(void);
pid_t	wait(int *status);
pid_t	waitpid(pid_t pid, int *status, int options);
int	execl(const char *path, const char *arg0, ...);
int	execv(const char *path, char *const argv[]);
void	exit(int status);

int	open(const char *path, int flags, ...);
int	creat(const char *path, mode_t mode);
int	close(int fn);
ssize_t	read(int fn, void *buf, size_t nbytes);
ssize_t	write(int fn, const void *buf, size_t nbytes);
off_t	lseek(int fn, off_t offset, int whence);
int	dup(int oldfn);
int	dup2(int oldfn, int newfn);
int	ftruncate(int fn, off_t newlength);
int	truncate(const char *path, off_t newlength);
int	isatty(int fn);
int	remove(const char *path);
int	fsync(int fn);

#endif	// !PIOS_INC_UNISTD_H
#endif	// LAB >= 4
