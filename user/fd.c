#if LAB >= 5
#include "lib.h"
#include <inc/fd.h>

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
dev_lookup(int dev_char, struct Dev **dev)
{
	int i;

	for (i=0; devtab[i]; i++)
		if (devtab[i]->dev_char == dev_char) {
			*dev = devtab[i];
			return 0;
		}
	printf("[%08x] unknown device type %d\n", env->env_id, dev_char);
	return -E_INVAL;
}

int
fd_alloc(u_int *pva)
{
	int i, r;
	u_int va;

	if (!(vpd[PDX(FDTABLE)]&PTE_P)) {
		if ((r = sys_mem_alloc(0, FDTABLE, PTE_P|PTE_W|PTE_U|PTE_LIBRARY)) < 0)
			return -E_NO_MEM;
		sys_mem_unmap(0, FDTABLE);
	}

	for (i=0; i<MAXFD; i++) {
		va = (u_int)INDEX2FD(i);
		if (((vpd[PDX(va)]&PTE_P) == 0 || vpt[VPN(va)]&PTE_P) == 0) {
			*pva = va;
			return 0;
		}
	}
	return -E_MAX_OPEN;
}

void
fd_close(struct Fd *fd)
{
	sys_mem_unmap(0, (u_int)fd);
}

int
fd_lookup(int fd, struct Fd **pfd)
{
	u_int va;

	if (fd < 0 || fd >= MAXFD) {
		if (debug) printf("[%08x] bad fd %d\n", env->env_id, fd);
		return -E_INVAL;
	}
	va = (u_int)INDEX2FD(fd);
	if(!(vpd[PDX(va)]&PTE_P) || !(vpt[VPN(va)]&PTE_P)) {
		if (debug) printf("[%08x] closed fd %d\n", env->env_id, fd);
		return -E_INVAL;
	}
	*pfd = (struct Fd*)va;
	return 0;
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
	||  (r = dev_lookup(fd->fd_dev, &dev)) < 0)
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
	if ((r = sys_mem_map(0, (u_int)oldfd, 0, (u_int)newfd, vpt[VPN(oldfd)]&PTE_USER)) < 0)
		return r;

	ova = fd2data(oldfd);
	nva = fd2data(newfd);

	if (vpd[PDX(ova)])
		for (i=0; i<PDMAP; i+=BY2PG) {
			pte = vpt[VPN(ova+i)];
			if(pte&PTE_P) {
				// should be no error here -- pd is already allocated
				if ((r = sys_mem_map(0, ova+i, 0, nva+i, pte&PTE_USER)) < 0)
					panic("dup sys_mem_map: %e", r);
			}
		}

	return newfdnum;
}

int
read(int fdnum, void *buf, u_int n)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
	||  (r = dev_lookup(fd->fd_dev, &dev)) < 0)
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
	||  (r = dev_lookup(fd->fd_dev, &dev)) < 0)
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
	||  (r = dev_lookup(fd->fd_dev, &dev)) < 0)
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

#endif /* LAB >= 5 */
