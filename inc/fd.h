#if LAB >= 5
#ifndef _INC_FD_H_
#define _INC_FD_H_ 1

// pre-declare for forward references
struct Fd;
struct Stat;
struct Dev;

struct Dev
{
	int dev_char;
	char *dev_name;
	int (*dev_read)(struct Fd*, void*, u_int, u_int);
	int (*dev_write)(struct Fd*, const void*, u_int, u_int);
	int (*dev_close)(struct Fd*);
	int (*dev_stat)(struct Fd*, struct Stat*);
	int (*dev_seek)(struct Fd*, u_int);
};

struct Fd
{
	u_int fd_dev;
	u_int fd_offset;
	u_int fd_omode;
};

#include <inc/fs.h>

struct Stat
{
	char st_name[MAXNAMELEN];
	u_int st_size;
	u_int st_isdir;
	struct Dev *st_dev;
};

int fd_alloc(u_int *va);
int fd_lookup(int fd, struct Fd **pfd);
u_int fd2data(struct Fd*);
int fd2num(struct Fd*);
int dev_lookup(int dev_char, struct Dev **dev);

extern struct Dev devcons;
extern struct Dev devfile;
extern struct Dev devpipe;

#endif
#endif
