#if LAB >= 5
#include "lib.h"
#include <inc/fs.h>

// maximum number of open files
#define MAXFD	32
#define FILEBASE 0xd0000000

// open file structure on client side
struct Fileinfo
{
	u_int fi_va;		// virtual address of file contents
	u_int fi_busy;		// is the fd in use?
	u_int fi_size;		// how many bytes in the fd?
	u_int fi_omode;		// file open mode (O_RDONLY etc.)
	u_int fi_fileid;	// file id on server
	u_int fi_offset;	// r/w offset
};

// local information about open files
static struct Fileinfo filetab[MAXFD];

// find an available struct Fileinfo
static int
fd_alloc(struct Fileinfo **fi)
{
	int i;

	for (i=0; i<MAXFD; i++)
		if (!filetab[i].fi_busy) {
			if (fi)
				*fi = &filetab[i];
			filetab[i].fi_busy = 1;
			filetab[i].fi_va = FILEBASE+PDMAP*i;
			
			return i;
		}
	return -E_MAX_OPEN;
}

static int
fd_lookup(int fd, struct Fileinfo **fi)
{
	if (fd < 0 || fd >= MAXFD || !filetab[fd].fi_busy)
		return -E_INVAL;
	*fi = &filetab[fd];
	return 0;
}

int
open(const char *path, int mode)
{
	int i, fd, r;
	struct Fileinfo *fi;

	if ((r = fd_alloc(&fi)) < 0)
		return r;
	fd = r;

	if ((r = fsipc_open(path, mode, &fi->fi_fileid, &fi->fi_size)) < 0)
		return r;
	for (i=0; i < fi->fi_size; i+=BY2PG)
		if ((r = fsipc_map(fi->fi_fileid, i, fi->fi_va+i)) < 0) {
			fi->fi_size = i-BY2PG;
			fi->fi_omode = 0;
			close(fd);
			return r;
		}
	fi->fi_omode = mode;
	fi->fi_offset = 0;

	return fd;
}

int
close(int fd)
{
	int i, r, ret;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	ret = 0;
	for (i=0; i < fi->fi_size; i+=BY2PG) {
		if ((vpt[VPN(fi->fi_va+i)]&(PTE_P|PTE_D)) == (PTE_P|PTE_D))
			if ((r = fsipc_dirty(fi->fi_fileid, i)) < 0)
				ret = r;
		if ((r = sys_mem_unmap(0, fi->fi_va+i)) < 0)
			panic("close: sys_mem_unmap: %e", r);
	}
	if ((r = fsipc_close(fi->fi_fileid)) < 0)
		ret = r;
	fi->fi_busy = 0;
	return r;
}

int
read(int fd, void *buf, u_int n)
{
	int r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;
	if ((fi->fi_omode & O_ACCMODE) == O_WRONLY)
		return -E_INVAL;

	if (fi->fi_offset > fi->fi_size)
		return 0;
	if (fi->fi_offset+n > fi->fi_size)
		n = fi->fi_size - fi->fi_offset;
	bcopy((char*)fi->fi_va + fi->fi_offset, buf, n);
	fi->fi_offset += n;
	return n;
}

int
read_map(int fd, u_int offset, void **blk)
{
	int r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;
	if (offset > MAXFILESIZE)
		return -E_NO_DISK;
	*blk = (void*)(fi->fi_va + offset);
	return 0;
}

int
write(int fd, const void *buf, u_int n)
{
	int i, r;
	u_int tot;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	if ((fi->fi_omode & O_ACCMODE) == O_RDONLY)
		return -E_INVAL;

	tot = fi->fi_offset + n;
	if (tot > MAXFILESIZE)
		return -E_NO_DISK;

	if (tot > fi->fi_size) {
		if ((r = fsipc_set_size(fi->fi_fileid, tot)) < 0)
			return r;
		for (i = ROUND(fi->fi_size, BY2PG); i < ROUND(tot, BY2PG); i += BY2PG) {
			if ((r = fsipc_map(fi->fi_fileid, i, fi->fi_va+i)) < 0) {
				fsipc_set_size(fi->fi_fileid, fi->fi_size);
				return r;
			}
		}
		fi->fi_size = tot;
	}

	bcopy(buf, (char*)fi->fi_va+fi->fi_offset, n);
	return n;
}

int
seek(int fd, u_int offset)
{
	int r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	fi->fi_offset = offset;
	return 0;
}

int
ftruncate(int fd, u_int size)
{
	int i, r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	if ((r = fsipc_set_size(fi->fi_fileid, size)) < 0)
		return r;

	for (i=ROUND(size, BY2PG); i<fi->fi_size; i+=BY2PG)
		if ((r = sys_mem_unmap(0, fi->fi_va+i)) < 0)
			panic("ftruncate: sys_mem_unmap %08x: %e", fi->fi_va+i, r);

	fi->fi_size = size;
	return 0;
}

int
remove(const char *path)
{
	return fsipc_remove(path);
}

int
sync(void)
{
	return fsipc_sync();
}
#endif
