#if LAB >= 4
/*
 * User-space Unix compatibility API - stat() definitions.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#ifndef PIOS_INC_STAT_H
#define PIOS_INC_STAT_H 1

#include <types.h>

struct stat {
	ino_t		st_ino;		/* File inode number */
	mode_t		st_mode;	/* File access mode */
	off_t		st_size;	/* File size in bytes */
#if LAB >= 9
	nlink_t		st_nlink;	/* Number of hardlinks to file */
	dev_t		st_dev;		/* Device number containing file */
	dev_t		st_rdev;	/* Device type */
	uid_t		st_uid;		/* User ID of the file's owner */
	gid_t		st_gid;		/* Group ID of the file's group */
	time_t		st_atime;	/* Time of last access */
	time_t		st_mtime;	/* Time of last data modification */
	time_t		st_ctime;	/* Time of last file status change */
	blkcnt_t	st_blocks;	/* Blocks allocated for file */
	blksize_t	st_blksize;	/* Optimal blocksize for I/O */
#endif
};

#if LAB >= 9				/* Unix perms not needed in PIOS */
#define	S_ISUID		0004000		/* set user id on execution */
#define	S_ISGID		0002000		/* set group id on execution */
#define	S_ISVTX		0001000		/* save swapped text even after use */
                
#define	S_IRWXU		0000700		/* RWX mask for owner */
#define	S_IRUSR		0000400		/* R for owner */
#define	S_IWUSR		0000200		/* W for owner */
#define	S_IXUSR		0000100		/* X for owner */
                
#define	S_IRWXG		0000070		/* RWX mask for group */
#define	S_IRGRP		0000040		/* R for group */
#define	S_IWGRP		0000020		/* W for group */
#define	S_IXGRP		0000010		/* X for group */
                
#define	S_IRWXO		0000007		/* RWX mask for other */
#define	S_IROTH		0000004		/* R for other */
#define	S_IWOTH		0000002		/* W for other */
#define	S_IXOTH		0000001		/* X for other */

#define S_IREAD		S_IRUSR		/* C standard permissions */
#define S_IWRITE	S_IWUSR
#define S_IEXEC		S_IXUSR
#endif

#define	S_IFMT		0070000		/* type of file mask */
#define	S_IFREG		0010000		/* regular */
#define	S_IFDIR		0020000		/* directory */
#if LAB >= 9	       			/* Unix file types unused by PIOS */
#define	S_IFLNK		0030000		/* symbolic link */
#define	S_IFCHR		0040000		/* character special */
#define	S_IFBLK		0050000		/* block special */
#define	S_IFIFO		0060000		/* named pipe (fifo) */
#define	S_IFSOCK	0070000		/* socket */
#endif

#define S_IFPART	0100000		/* partial file: wait on read at end */
#define S_IFCONF	0200000		/* write/write conflict(s) detected */

#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)	/* regular file */
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)	/* directory */
#if LAB >= 9
#define	S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)	/* symbolic link */
#define	S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)	/* char special */
#define	S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)	/* block special */
#define	S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	/* fifo or socket */
#define	S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)	/* socket */
#endif

int	stat(const char *path, struct stat *statbuf);
int	fstat(int fd, struct stat *statbuf);
#if LAB >= 9
int	lstat(const char *path, struct stat *statbuf);
int	chmod(const char *path, mode_t mode);
int	fchmod(int fd, mode_t mode);
int	mkdir(const char *path, mode_t mode);
int	mknod(const char *path, mode_t mode, dev_t dev);
mode_t	umask(mode_t mask);
#endif

#endif	// !PIOS_INC_STAT_H
#endif	// LAB >= 4
