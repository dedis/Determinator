#if LAB >= 5

#include <inc/lib.h>

#define debug 0

#define MAXFD 32
#define FILEBASE 0xd0000000
#define FDTABLE (FILEBASE-PDMAP)

#define INDEX2FD(i)	(FDTABLE+(i)*BY2PG)
#define INDEX2DATA(i)	(FILEBASE+(i)*PDMAP)

static struct Dev *devtab[] =
{
	&devfile,
	&devpipe,
	&devcons,
	0
};

int
dev_lookup(int dev_id, struct Dev **dev)
{
	int i;

	for (i=0; devtab[i]; i++)
		if (devtab[i]->dev_id == dev_id) {
			*dev = devtab[i];
			return 0;
		}
	printf("[%08x] unknown device type %d\n", env->env_id, dev_id);
	return -E_INVAL;
}

int
fd_alloc(struct Fd **fd)
{
#if SOL >= 6
	int i;
	u_int va;

	for (i=0; i<MAXFD; i++) {
		va = (u_int)INDEX2FD(i);
		if ((vpd[PDX(va)]&PTE_P) == 0 || (vpt[VPN(va)]&PTE_P) == 0) {
			*fd = (struct Fd*)va;
			return 0;
		}
	}
	return -E_MAX_OPEN;
#else
	// Your code here.
	//
	// Find the smallest i from 0 to MAXFD-1 that doesn't have
	// its fd page mapped.  Set *fd to the fd page virtual address.
	// (Do not allocate a page.  It is up to the caller to allocate
	// the page.  This means that if someone calls fd_alloc twice
	// in a row without allocating the first page we return, we'll
	// return the same page the second time.)
	// Return 0 on success, or an error code on error.

	panic("fd_alloc not implemented");
	return -E_MAX_OPEN;
#endif
}

void
fd_close(struct Fd *fd)
{
	sys_mem_unmap(0, (u_int)fd);
}

int
fd_lookup(int fdnum, struct Fd **fd)
{
#if SOL >= 6
	u_int va;

	if (fdnum < 0 || fdnum >= MAXFD) {
		if (debug) printf("[%08x] bad fd %d\n", env->env_id, fd);
		return -E_INVAL;
	}
	va = (u_int)INDEX2FD(fdnum);
	if(!(vpd[PDX(va)]&PTE_P) || !(vpt[VPN(va)]&PTE_P)) {
		if (debug) printf("[%08x] closed fd %d\n", env->env_id, fd);
		return -E_INVAL;
	}
	*fd = (struct Fd*)va;
	return 0;
#else
	// Your code here.
	// 
	// Check that fdnum is in range and mapped.  If not, return -E_INVAL.
	// Set *fd to the fd page virtual address.  Return 0.

	panic("fd_lookup not implemented");
	return -E_INVAL;
#endif
}

u_int
fd2data(struct Fd *fd)
{
	return INDEX2DATA(fd2num(fd));
}

int
fd2num(struct Fd *fd)
{
	return ((u_int)fd - FDTABLE)/BY2PG;
}

int
close(int fdnum)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
	||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
		return r;
	r = (*dev->dev_close)(fd);
	fd_close(fd);
	return r;
}

void
close_all(void)
{
	int i;

	for (i=0; i<MAXFD; i++)
		close(i);
}

int
dup(int oldfdnum, int newfdnum)
{
	int i, r;
	u_int ova, nva, pte;
	struct Fd *oldfd, *newfd;

	if ((r = fd_lookup(oldfdnum, &oldfd)) < 0)
		return r;
	close(newfdnum);

	newfd = (struct Fd*)INDEX2FD(newfdnum);
	ova = fd2data(oldfd);
	nva = fd2data(newfd);

#if SOL >= 6
#else
	if ((r = sys_mem_map(0, (u_int)oldfd, 0, (u_int)newfd, vpt[VPN(oldfd)]&PTE_USER)) < 0)
		goto err;
#endif
	if (vpd[PDX(ova)]) {
		for (i=0; i<PDMAP; i+=BY2PG) {
			pte = vpt[VPN(ova+i)];
			if(pte&PTE_P) {
				// should be no error here -- pd is already allocated
				if ((r = sys_mem_map(0, ova+i, 0, nva+i, pte&PTE_USER)) < 0)
					goto err;
			}
		}
	}
#if SOL >= 6
	if ((r = sys_mem_map(0, (u_int)oldfd, 0, (u_int)newfd, vpt[VPN(oldfd)]&PTE_USER)) < 0)
		goto err;
#endif

	return newfdnum;

err:
	sys_mem_unmap(0, (u_int)newfd);
	for (i=0; i<PDMAP; i+=BY2PG)
		sys_mem_unmap(0, nva+i);
	return r;
}

int
read(int fdnum, void *buf, u_int n)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
	||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
		return r;
	if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
		printf("[%08x] read %d -- bad mode\n", env->env_id, fdnum); 
		return -E_INVAL;
	}
	r = (*dev->dev_read)(fd, buf, n, fd->fd_offset);
	if (r >= 0)
		fd->fd_offset += r;
	return r;
}

int
readn(int fdnum, void *buf, u_int n)
{
	int m, tot;

	for (tot=0; tot<n; tot+=m) {
		m = read(fdnum, (char*)buf+tot, n-tot);
		if (m < 0)
			return m;
		if (m == 0)
			break;
	}
	return tot;
}

int
write(int fdnum, const void *buf, u_int n)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
	||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
		return r;
	if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
		printf("[%08x] write %d -- bad mode\n", env->env_id, fdnum);
		return -E_INVAL;
	}
	if (debug) printf("write %d %p %d via dev %s\n",
		fdnum, buf, n, dev->dev_name);
	r = (*dev->dev_write)(fd, buf, n, fd->fd_offset);
	if (r > 0)
		fd->fd_offset += r;
	return r;
}

int
seek(int fdnum, u_int offset)
{
	int r;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0)
		return r;
	fd->fd_offset = offset;
	return 0;
}

int
fstat(int fdnum, struct Stat *stat)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
	||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
		return r;
	stat->st_name[0] = 0;
	stat->st_size = 0;
	stat->st_isdir = 0;
	stat->st_dev = dev;
	return (*dev->dev_stat)(fd, stat);
}

int
stat(const char *path, struct Stat *stat)
{
	int fd, r;

	if ((fd = open(path, O_RDONLY)) < 0)
		return fd;
	r = fstat(fd, stat);
	close(fd);
	return r;
}

int
putchar(int ch)
{
	unsigned char c = ch;
	int r;

	r = write(0, &c, 1);
	if (r < 0)
		return r;
	return 0;
}

int getchar(void)
{
	unsigned char c;
	int r;

	r = read(0, &c, 1);
	if (r < 0)
		return r;
	if (r < 1)
		return -E_EOF;
	return c;
}

#endif /* LAB >= 5 */
