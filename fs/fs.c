#if LAB >= 5
#include "fs.h"

struct Super *super;

u_int nbitmap;		/* number of bitmap blocks */
u_int *bitmap;		/* bitmap blocks mapped in memory */

void file_decref(struct File*);
void file_flush(struct File*);
int block_is_free(u_int);

u_int
diskaddr(u_int blockno)
{
	if(super && blockno >= super->nblocks)
		panic("bad block number %08x in diskaddr", blockno);
	return DISKMAP+blockno*BY2BLK;
}

u_int
va_is_mapped(u_int va)
{
	return (vpd[PDX(va)]&PTE_P) && (vpt[VPN(va)]&PTE_P);
}

u_int
block_is_mapped(u_int blockno)
{
	u_int va;

	va = diskaddr(blockno);
	if (va_is_mapped(va))
		return va;
	return 0;
}

u_int
va_is_dirty(u_int va)
{
	return vpt[VPN(va)]&PTE_D;
}

int
read_block(u_int blockno, void **pblk, u_int *pisnew)
{
	int r;
	u_int va;

	va = diskaddr(blockno);

	if (va_is_mapped(va)) {
		if(pisnew)
			*pisnew = 0;
		*pblk = (void*)va;
		return 0;
	}

	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);

	/* Allocate a page to hold the disk block */
	if ((r = sys_mem_alloc(0, va, PTE_U|PTE_P|PTE_W)) < 0)
		return r;

	ide_read(DISKNO, blockno*SECT2BLK, (void*)va, SECT2BLK);
	if(pisnew)
		*pisnew = 1;
	*pblk = (void*)va;
	return 0;
}

void
write_block(u_int blockno)
{
	u_int va;

	va = diskaddr(blockno);
	if (!va_is_mapped(va))
		panic("write unmapped block %08x", blockno);

	ide_write(DISKNO, blockno*SECT2BLK, (void*)va, SECT2BLK);

	// clear dirty flag in pte
	sys_mem_map(0, va, 0, va, vpt[VPN(va)]&PTE_USER);
}


// Check to see if the block bitmap indicates that block 'blockno' is free.
// Returns 1 if the block is free, 0 if not.
int
block_is_free(u_int blockno)
{
	if (blockno >= super->nblocks)
		return 0;
	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;
	return 0;
}

// Search the bitmap for a free block and allocate it.
// Return block number allocated on success, < 0 on failure.
int
alloc_block_num(void)
{
	int i = 0; 

	for (i = 0; i < super->nblocks/32; i++) {
		if (bitmap[i] != 0) {
			/* Some bit in this word is set - find it */
			int j;
			for (j = 0; j < 32; j++) {
				if (bitmap[i] & (1<<j)) {
					/* allocate it and return */
					bitmap[i] &= ~(1<<j);
					return i*32+j;
				}
			}
			panic("couldn't find set bit!");
		}
	}

	return -E_NO_DISK;
}

int
alloc_block(void)
{
	int r;
	void *blk;

	if ((r = alloc_block_num()) < 0)
		return r;
	if ((r = read_block(r, &blk, 0)) < 0)
		return r;
	bzero(blk, BY2BLK);
	return r;
}

// Mark a block free in the bitmap
void
free_block(u_int blockno)
{
	bitmap[blockno/32] |= 1<<(blockno%32);
}

// Read and validate the file system super-block.
void
read_super(void)
{
	int r;
	void *blk;

	if ((r = read_block(1, &blk, 0)) < 0)
		panic("cannot read superblock: %e", r);

	super = blk;
	if (super->magic != FS_MAGIC)
		panic("bad file system magic number");

	if (super->nblocks > DISKMAX/BY2BLK)
		panic("file system is too large");
}

// Read and validate the file system bitmap.
void
read_bitmap(void)
{
	int r;
	u_int i;
	void *blk;

	for (i=0; i*BIT2BLK < super->nblocks; i++) {
		if ((r = read_block(2+i, &blk, 0)) < 0)
			panic("cannot read bitmap block %d: %e", i, r);
		if (i == 0)
			bitmap = blk;
		/* Make sure all bitmap blocks are marked in-use */
		assert(!block_is_free(2+i));
	}

	/* Make sure the reserved and root blocks are marked in-use */
	assert(!block_is_free(0));
	assert(!block_is_free(1));
}

void
fs_init(void)
{
	read_super();
	read_bitmap();
}

int
file_walk(struct File *f, u_int filebno, u_int **ppdiskbno, u_int alloc)
{
	int r;
	u_int *ptr;
	void *blk;

	if (filebno < NDIRECT)
		ptr = &f->direct[filebno];
	else if ((filebno -= NDIRECT) < NINDIRECT) {
		if (f->indirect == 0) {
			if (alloc == 0)
				return -E_NOT_FOUND;
			if ((r = alloc_block()) < 0)
				return r;
			f->indirect = r;
		}
		if ((r=read_block(f->indirect, &blk, 0)) < 0)
			return r;
		ptr = (u_int*)blk + filebno;
	} else
		return -E_INVAL;

	*ppdiskbno = ptr;
	return 0;
}

int
file_map_block(struct File *f, u_int filebno, u_int *pdiskbno, u_int alloc)
{
	int r;
	u_int *ptr;

	if ((r = file_walk(f, filebno, &ptr, alloc)) < 0)
		return r;
	if (*ptr == 0) {
		if (alloc == 0)
			return -E_NOT_FOUND;
		if ((r = alloc_block()) < 0)
			return r;
		*ptr = r;
	}
	*pdiskbno = *ptr;
	return 0;
}

int
file_clear_block(struct File *f, u_int filebno)
{
	int r;
	u_int *ptr;

	if ((r = file_walk(f, filebno, &ptr, 0)) < 0)
		return r;
	if (*ptr) {
		free_block(*ptr);
		*ptr = 0;
	}
	return 0;
}

int
file_get_block(struct File *f, u_int filebno, void **pblk)
{
	int i, r, isnew;
	u_int diskbno;
	void *blk;
	struct File *ff;

	if ((r = file_map_block(f, filebno, &diskbno, 1)) < 0)
		return r;

	if ((r=read_block(diskbno, &blk, &isnew)) < 0)
		return r;
	if (isnew && f->type == FTYPE_DIR) {
		// just read some File structures off disk - initialize the memory-only parts
		ff = blk;
		for (i=0; i<FILE2BLK; i++) {
			ff[i].ref = 0;
			ff[i].dir = f;
			f->ref++;
		}
	}

	*pblk = blk;
	return 0;
}

int
file_dirty(struct File *f, u_int offset)
{
	int r;
	void *blk;

	if ((r = file_get_block(f, offset/BY2BLK, &blk)) < 0)
		return r;
	*(volatile char*)blk = *(volatile char*)blk;
	return 0;
}

int
dir_lookup(struct File *dir, char *name, struct File **pfile)
{
	int r;
	u_int i, j, nblock;
	void *blk;
	struct File *file;

	// search dir for name
	nblock = dir->size / BY2BLK;
	for (i=0; i<nblock; i++) {
		if ((r=file_get_block(dir, i, &blk)) < 0)
			return r;
		file = blk;
		for (j=0; j<FILE2BLK; j++)
			if (strcmp(file[j].name, name) == 0) {
				*pfile = &file[j];
				file[j].ref++;
				return 0;
			}
	}
	return -E_NOT_FOUND;	
}

int
dir_alloc_file(struct File *dir, struct File **pfile)
{
	int r;
	u_int nblock, i , j;
	void *blk;
	struct File *file;

	nblock = dir->size / BY2BLK;
	for (i=0; i<nblock; i++) {
		if ((r=file_get_block(dir, i, &blk)) < 0)
			return r;
		file = blk;
		for (j=0; j<FILE2BLK; j++)
			if (file[j].name[0] == '\0') {
				*pfile = &file[j];
				file[j].ref++;
				return 0;
			}
	}
	dir->size += BY2BLK;
	if ((r = file_get_block(dir, i, &blk)) < 0)
		return r;
	file = blk;
	*pfile = &file[0];
	file[0].ref++;
	return 0;		
}

char*
skip_slash(char *p)
{
	while (*p == '/')
		p++;
	return p;
}

int
walk_path(char *path, struct File **pdir, struct File **pfile, char *lastelem)
{
	char *p;
	char name[MAXNAMELEN];
	struct File *dir, *file;
	int r;

	if (*path != '/')
		return -E_BAD_PATH;
	path = skip_slash(path);
	file = &super->root;
	file->ref++;
	dir = 0;
	name[0] = 0;

	if(pdir)
		*pdir = 0;
	*pfile = 0;
	while (*path != '\0') {
		dir = file;
		p = path;
		while (*path != '/' && *path != '\0')
			path++;
		if (path - p >= MAXNAMELEN)
			return -E_BAD_PATH;
		bcopy(p, name, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (dir->type != FTYPE_DIR)
			return -E_NOT_FOUND;

		if ((r=dir_lookup(dir, name, &file)) < 0) {
			if(r == -E_NOT_FOUND && *path == '\0') {
				if(pdir)
					*pdir = dir;
				else
					file_decref(dir);
				if (lastelem)
					strcpy(lastelem, name);
				*pfile = 0;
			}
			return r;
		}
	}

	if(pdir)
		*pdir = dir;
	else
		file_decref(dir);
	*pfile = file;
	return 0;
}

int
file_open(char *path, struct File **pfile)
{
	int r;
	struct File *file;

	if ((r = walk_path(path, 0, &file, 0)) < 0)
		return r;

	*pfile = file;
	return 0;
}

int
file_create(char *path, struct File **pfile)
{
	char name[MAXNAMELEN];
	int r;
	struct File *dir, *file;

	if ((r = walk_path(path, &dir, &file, name)) == 0) {
		file_decref(dir);
		file_decref(file);
		return -E_FILE_EXISTS;
	}
	if (r != -E_NOT_FOUND || dir == 0)
		return r;
	if (dir_alloc_file(dir, &file) < 0) {
		file_decref(dir);
		return r;
	}
	file_decref(dir);
	strcpy(file->name, name);
	*pfile = file;
	return 0;
}

void
file_truncate(struct File *f, u_int newsize)
{
	int r;
	u_int bno, old_last_bno, new_last_bno;

	old_last_bno = (f->size+BY2BLK-1)/BY2BLK;
	new_last_bno = (newsize+BY2BLK-1)/BY2BLK;
	for (bno=old_last_bno; bno>new_last_bno; bno--)
		if ((r = file_clear_block(f, bno)) < 0)
			printf("warning: file_clear_block: %e", r);

	if (new_last_bno < NDIRECT && f->indirect) {
		free_block(f->indirect);
		f->indirect = 0;
	}
}

int
file_set_size(struct File *f, u_int newsize)
{
	if (f->size > newsize)
		file_truncate(f, newsize);
	f->size = newsize;
	return 0;
}

void
file_decref(struct File *f)
{
	if (f == &super->root)
		return;

	if (--f->ref == 0) {
		file_flush(f);
		file_decref(f->dir);
	}
}

void
file_flush(struct File *f)
{
	int i;
	u_int diskbno;
	u_int va;

	for (i=0; i < (f->size+BY2BLK-1)/BY2BLK; i++) {
		if (file_map_block(f, i, &diskbno, 0) < 0)
			continue;
		if ((va=block_is_mapped(diskbno)) && va_is_dirty(va))
			write_block(diskbno);
	}	
}

void
fs_sync(void)
{
	int i;
	u_int va;

	for (i=0; i<super->nblocks; i++) {
		va = diskaddr(i);
		if(!(vpd[PDX(va)]&PTE_P)){
			i += PTE2PT-1;
			continue;
		}
		if(!(vpt[VPN(va)]&PTE_P) || !(vpt[VPN(va)]&PTE_D))
			continue;
		write_block(i);
	}
}
void
file_close(struct File *f)
{
	file_decref(f);
}

int
file_remove(char *path)
{
	int r;
	struct File *file;

	if ((r = walk_path(path, 0, &file, 0)) < 0)
		return r;

	file_truncate(file, 0);
	file->name[0] = '\0';
	file_decref(file);
	return 0;
}

#endif
