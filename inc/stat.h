#if LAB >= 4
// User-space Unix compatibility API - stat() definitions.
#ifndef PIOS_INC_STAT_H
#define PIOS_INC_STAT_H 1

#include <inc/types.h>

struct stat {
	dev_t	st_dev;			/* "Device number" containing file */
	ino_t	st_ino;			/* File inode number */
	mode_t	st_mode;		/* File access mode */
	nlink_t	st_nlink;		/* Number of hardlinks to file */
	off_t	st_size;		/* File size */
#if LAB >= 99
	uid_t	  st_uid;		/* user ID of the file's owner */
	gid_t	  st_gid;		/* group ID of the file's group */
	dev_t	  st_rdev;		/* device type */
	time_t	  st_atime;		/* time of last access */
	time_t	  st_mtime;		/* time of last data modification */
	time_t	  st_ctime;		/* time of last file status change */
	off_t	  st_size;		/* file size, in bytes */
	int64_t	  st_blocks;		/* blocks allocated for file */
	u_int32_t st_blksize;		/* optimal blocksize for I/O */
#endif
};

#if LAB >= 99
#define	S_ISUID	0004000			/* set user id on execution */
#define	S_ISGID	0002000			/* set group id on execution */
#define	S_ISVTX	0001000			/* save swapped text even after use */

#define	S_IRWXU	0000700			/* RWX mask for owner */
#define	S_IRUSR	0000400			/* R for owner */
#define	S_IWUSR	0000200			/* W for owner */
#define	S_IXUSR	0000100			/* X for owner */

#define	S_IRWXG	0000070			/* RWX mask for group */
#define	S_IRGRP	0000040			/* R for group */
#define	S_IWGRP	0000020			/* W for group */
#define	S_IXGRP	0000010			/* X for group */

#define	S_IRWXO	0000007			/* RWX mask for other */
#define	S_IROTH	0000004			/* R for other */
#define	S_IWOTH	0000002			/* W for other */
#define	S_IXOTH	0000001			/* X for other */
#endif

#define	S_IFMT	 0070000		/* type of file mask */
#define	S_IFREG	 0010000		/* regular */
#define	S_IFDIR	 0020000		/* directory */
#define	S_IFLNK	 0030000		/* symbolic link */
#if LAB >= 99
#define	S_IFCHR	 0040000		/* character special */
#define	S_IFBLK	 0050000		/* block special */
#define	S_IFIFO	 0060000		/* named pipe (fifo) */
#define	S_IFSOCK 0070000		/* socket */
#endif

#define S_ISPART 0100000		/* partial file: wait on read at end */
#define S_ISRAND 0200000		/* file has been randomly written to */
#define S_ISCONF 0400000		/* write/write conflict(s) detected */

#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)	/* regular file */
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)	/* directory */
#define	S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)	/* symbolic link */
#if LAB >= 99
#define	S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)	/* char special */
#define	S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)	/* block special */
#define	S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	/* fifo or socket */
#define	S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)	/* socket */
#endif

int	stat(const char *path, struct stat *statbuf);
int	fstat(int fd, struct stat *statbuf);

#endif	// !PIOS_INC_STAT_H
#endif	// LAB >= 4
