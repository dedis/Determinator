#if LAB >= 5

#include <inc/fs.h>
#include <inc/string.h>
#include <inc/lib.h>

#define debug 0

static int file_close(struct Fd *fd);
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);
static int file_stat(struct Fd *fd, struct Stat *stat);

struct Dev devfile =
{
.dev_id=	'f',
.dev_name=	"file",
.dev_read=	file_read,
.dev_write=	file_write,
.dev_close=	file_close,
.dev_stat=	file_stat,
};

#if SOL >= 5
static int fmap(struct Filefd *f, u_int oldsize, u_int newsize);
static int funmap(struct Filefd *f, u_int oldsize, u_int newsize, u_int dirty);
#endif

// Open a file (or directory),
// returning the file descriptor on success, < 0 on failure.
int
open(const char *path, int mode)
{
#if SOL >= 5
	int r;
	struct Fd *fd;
	struct Filefd *f;

	if ((r = fd_alloc(&fd)) < 0
	||  (r = fsipc_open(path, mode, fd)) < 0)
		return r;
	f = (struct Filefd*)fd;

	if ((r = fmap(f, 0, f->f_file.f_size)) < 0) {
		funmap(f, 0, f->f_file.f_size, 0);
		sys_mem_unmap(0, (u_int)fd);
		return r;
	}
	return fd2num(fd);
#else
	// Your code here.
	panic("open() unimplemented!");
#endif
}

// Close a file descriptor
int
file_close(struct Fd *fd)
{
#if SOL >= 5
	int r, ret;
	u_int va;
	struct Filefd *f;

	f = (struct Filefd*)fd;

	va = fd2data(fd);
	ret = funmap(f, 0, f->f_file.f_size, 1);
	if ((r = fsipc_close(f->f_fileid)) < 0 && ret == 0)
		ret = r;
	return ret;
#else
	// Your code here.
	panic("close() unimplemented!");
#endif
}

// Read 'n' bytes from 'fd' at the current seek position into 'buf'.
// Since files are memory-mapped, this amounts to a memcpy()
// surrounded by a little red tape to handle the file size and seek pointer.
static int
file_read(struct Fd *fd, void *buf, u_int n, u_int offset)
{
	u_int size;
	struct Filefd *f;

	f = (struct Filefd*)fd;

	// avoid reading past the end of file
	size = f->f_file.f_size;
	if (offset > size)
		return 0;
	if (offset+n > size)
		n = size - offset;

	// read the data by copying from the file mapping
	memcpy(buf, (char*)fd2data(fd)+offset, n);
	return n;
}

// Find the virtual address of the page
// that maps the file block starting at 'offset'.
int
read_map(int fdnum, u_int offset, void **blk)
{
	int r;
	u_int va;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0)
		return r;
	if (fd->fd_dev_id != devfile.dev_id)
		return -E_INVAL;
	va = fd2data(fd) + offset;
	if (offset >= MAXFILESIZE)
		return -E_NO_DISK;
	if (!(vpd[PDX(va)]&PTE_P) || !(vpt[VPN(va)]&PTE_P))
		return -E_NO_DISK;
	*blk = (void*)va;
	return 0;
}

// Write 'n' bytes from 'buf' to 'fd' at the current seek position.
static int
file_write(struct Fd *fd, const void *buf, u_int n, u_int offset)
{
	int r;
	u_int tot;
	struct Filefd *f;

	f = (struct Filefd*)fd;

	// don't write past the maximum file size
	tot = offset + n;
	if (tot > MAXFILESIZE)
		return -E_NO_DISK;

	// increase the file's size if necessary
	if (tot > f->f_file.f_size) {
		if ((r = ftruncate(fd2num(fd), tot)) < 0)
			return r;
	}

	// write the data
	memcpy((char*)fd2data(fd)+offset, buf, n);
	return n;
}

static int
file_stat(struct Fd *fd, struct Stat *st)
{
	struct Filefd *f;

	f = (struct Filefd*)fd;

	strcpy(st->st_name, f->f_file.f_name);
	st->st_size = f->f_file.f_size;
	st->st_isdir = f->f_file.f_type==FTYPE_DIR;
	return 0;
}

// Truncate or extend an open file to 'size' bytes
int
ftruncate(int fdnum, u_int size)
{
	int i, r;
	struct Fd *fd;
	struct Filefd *f;
	u_int oldsize, va, fileid;

	if (size > MAXFILESIZE)
		return -E_NO_DISK;

	if ((r = fd_lookup(fdnum, &fd)) < 0)
		return r;
	if (fd->fd_dev_id != devfile.dev_id)
		return -E_INVAL;

	f = (struct Filefd*)fd;
	fileid = f->f_fileid;
	oldsize = f->f_file.f_size;
	if ((r = fsipc_set_size(fileid, size)) < 0)
		return r;

	va = fd2data(fd);
#if SOL >= 6
	if ((r = fmap(f, oldsize, size)) < 0)
		return r;
	funmap(f, oldsize, size, 0);
#else
	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, BY2PG); i < ROUND(size, BY2PG); i += BY2PG) {
		if ((r = fsipc_map(fileid, i, va+i)) < 0) {
			fsipc_set_size(fileid, oldsize);
			return r;
		}
	}

	// Unmap pages if truncating the file
	for (i = ROUND(size, BY2PG); i < ROUND(oldsize, BY2PG); i+=BY2PG)
		if ((r = sys_mem_unmap(0, va+i)) < 0)
			panic("ftruncate: sys_mem_unmap %08x: %e", va+i, r);
#endif
	return 0;
}

#if SOL >= 5
static int
fmap(struct Filefd *f, u_int oldsize, u_int newsize)
{
	u_int i, va;
	int r;

	va = fd2data((struct Fd*)f);
	for (i=ROUND(oldsize, BY2PG); i<newsize; i+=BY2PG) {
		if ((r = fsipc_map(f->f_fileid, i, va+i)) < 0) {
			fsipc_set_size(f->f_fileid, oldsize);
			return r;
		}
	}
	return 0;
}

static int
funmap(struct Filefd *f, u_int oldsize, u_int newsize, u_int dirty)
{
	u_int i, va;
	int r, ret;

	ret = 0;
	va = fd2data((struct Fd*)f);
	for (i=ROUND(newsize, BY2PG); i < oldsize; i+=BY2PG) {
		if (dirty && (vpt[VPN(va+i)]&(PTE_P|PTE_D)) == (PTE_P|PTE_D))
			if ((r = fsipc_dirty(f->f_fileid, i)) < 0)
				ret = r;
		sys_mem_unmap(0, va+i);
	}
	return ret;
}

#endif
// Delete a file
int
remove(const char *path)
{
	return fsipc_remove(path);
}

// Synchronize disk with buffer cache
int
sync(void)
{
	return fsipc_sync();
}

#endif /* LAB >= 5 */
