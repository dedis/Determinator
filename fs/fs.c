#if LAB >= 5
#include "fs.h"

struct Super *super;

u_int nbitmap;		// number of bitmap blocks
u_int *bitmap;		// bitmap blocks mapped in memory

void file_flush(struct File*);
int block_is_free(u_int);

// Return the virtual address of this disk block.
u_int
diskaddr(u_int blockno)
{
	if(super && blockno >= super->s_nblocks)
		panic("bad block number %08x in diskaddr", blockno);
	return DISKMAP+blockno*BY2BLK;
}

// Is this virtual address mapped?
u_int
va_is_mapped(u_int va)
{
	return (vpd[PDX(va)]&PTE_P) && (vpt[VPN(va)]&PTE_P);
}

// Is this disk block mapped?
u_int
block_is_mapped(u_int blockno)
{
	u_int va;

	va = diskaddr(blockno);
	if (va_is_mapped(va))
		return va;
	return 0;
}

// Is this virtual address dirty?
u_int
va_is_dirty(u_int va)
{
	return vpt[VPN(va)]&PTE_D;
}

// Is this block dirty?
u_int
block_is_dirty(u_int blockno)
{
	u_int va;

	va = diskaddr(blockno);
	return va_is_mapped(va) && va_is_dirty(va);
}

// Allocate a page to hold the disk block
int
map_block(u_int blockno)
{
	if (block_is_mapped(blockno))
		return 0;
	return sys_mem_alloc(0, diskaddr(blockno), PTE_U|PTE_P|PTE_W);
}

// Make sure a particular disk block is loaded into memory.
// Return 0 on success, or a negative error code on error.
// 
// If blk!=0, set *blk to the address of the block in memory.
//
// If isnew!=0, set *isnew to 0 if the block was already in memory,
//	or to 1 if the block was loaded off disk to satisfy this request.
//
// (Isnew lets callers like file_get_block clear any memory-only
// fields from the disk blocks when they come in off disk.)
//
// Hint: use diskaddr, block_is_mapped, sys_mem_alloc, and ide_read.
int
read_block(u_int blockno, void **blk, u_int *isnew)
{
	int r;
	u_int va;

	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);

#if SOL >= 5
	va = diskaddr(blockno);

	if (va_is_mapped(va)) {
		if (isnew)
			*isnew = 0;
		if (blk)
			*blk = (void*)va;
		return 0;
	}

	if ((r = map_block(blockno)) < 0)
		return r;

	ide_read(DISKNO, blockno*SECT2BLK, (void*)va, SECT2BLK);
	if (isnew)
		*isnew = 1;
	if (blk)
		*blk = (void*)va;

	return 0;
#else
	// Your code here
	panic("read_block not implemented");
	return 0;
#endif
}

// Copy the current contents of the block out to disk.
// Then clear the PTE_D bit using sys_mem_map.
// The PTE_USER constant in inc/mmu.h may be useful!
void
write_block(u_int blockno)
{
	u_int va;

	if (!block_is_mapped(blockno))
		panic("write unmapped block %08x", blockno);
#if SOL >= 5
	va = diskaddr(blockno);

	ide_write(DISKNO, blockno*SECT2BLK, (void*)va, SECT2BLK);

	// clear dirty flag in pte
	sys_mem_map(0, va, 0, va, vpt[VPN(va)]&PTE_USER);
#else
	// Your code here to write disk block and clear PTE_D.
	panic("write_block not implemented");
#endif
}

// Make sure this block is unmapped.
void
unmap_block(u_int blockno)
{
	int r;

	if(!block_is_mapped(blockno))
		return;

	assert(block_is_free(blockno) || !block_is_dirty(blockno));

	if ((r = sys_mem_unmap(0, diskaddr(blockno))) < 0)
		panic("unmap_block: sys_mem_unmap: %e", r);
	assert(!block_is_mapped(blockno));
}

// Check to see if the block bitmap indicates that block 'blockno' is free.
// Return 1 if the block is free, 0 if not.
int
block_is_free(u_int blockno)
{
	if (super == 0 || blockno >= super->s_nblocks)
		return 0;
	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;
	return 0;
}

// Mark a block free in the bitmap
void
free_block(u_int blockno)
{
	// Blockno zero is the null pointer of block numbers.
	if (blockno == 0)
		panic("attempt to free zero block");
	bitmap[blockno/32] |= 1<<(blockno%32);
}

// Search the bitmap for a free block and allocate it.
// 
// Return block number allocated on success, -E_NO_DISK if we are out of blocks.
int
alloc_block_num(void)
{
#if SOL >= 5
	int i = 0, j;
	static int lastalloc;

	for (i = 0; i < super->s_nblocks; i++) {
		j = (lastalloc+i)%super->s_nblocks;
		if (block_is_free(j)) {
			bitmap[j/32] &= ~(1<<(j%32));
			write_block(2+j/BIT2BLK);
			lastalloc = j;
			return j;
		}
	}
#else
	// Your code here.
	panic("alloc_block_num not implemented");
#endif
	return -E_NO_DISK;
}

// Allocate a block -- first find a free block in the bitmap,
// then map it into memory.
int
alloc_block(void)
{
	int r, bno;

	if ((r = alloc_block_num()) < 0)
		return r;
	bno = r;

	if ((r = map_block(bno)) < 0) {
		free_block(bno);
		return r;
	}
	return bno;
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
	if (super->s_magic != FS_MAGIC)
		panic("bad file system magic number");

	if (super->s_nblocks > DISKMAX/BY2BLK)
		panic("file system is too large");

	printf("superblock is good\n");
}

// Read and validate the file system bitmap.
//
// Read all the bitmap blocks into memory.
// Set the "bitmap" pointer to point at the beginning of the first bitmap block.
// 
// Check that they're all marked as inuse
// (for each block i, assert(!block_is_free(i))).
void
read_bitmap(void)
{
	int r;
	u_int i;
	void *blk;

#if SOL >= 5
	for (i=0; i*BIT2BLK < super->s_nblocks; i++) {
		if ((r = read_block(2+i, &blk, 0)) < 0)
			panic("cannot read bitmap block %d: %e", i, r);
		if (i == 0)
			bitmap = blk;
		// Make sure all bitmap blocks are marked in-use
		assert(!block_is_free(2+i));
	}
#else
	// Your code here
	panic("read_bitmap not implemented");
#endif

	// Make sure the reserved and root blocks are marked in-use
	assert(!block_is_free(0));
	assert(!block_is_free(1));
	assert(bitmap);

	printf("read_bitmap is good\n");
}

// Test that write_block works, by smashing the superblock and reading it back.
void
check_write_block(void)
{
	super = 0;

	// back up super block
	read_block(0, 0, 0);
	bcopy((char*)diskaddr(1), (char*)diskaddr(0), BY2PG);

	// smash it 
	strcpy((char*)diskaddr(1), "OOPS!\n");
	write_block(1);
	assert(block_is_mapped(1));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_mem_unmap(0, diskaddr(1));
	assert(!block_is_mapped(1));

	// read it back in
	read_block(1, 0, 0);
	assert(strcmp((char*)diskaddr(1), "OOPS!\n") == 0);

	// fix it
	bcopy((char*)diskaddr(0), (char*)diskaddr(1), BY2PG);
	write_block(1);
	super = (struct Super*)diskaddr(1);

	printf("write_block is good\n");
}

// Initialize the file system
void
fs_init(void)
{
	read_super();
	check_write_block();
	read_bitmap();
}

// Like pgdir_walk but for files.  Set *ppdiskbno to a pointer to the disk block number
// for the filebno'th block in file f.  If alloc is set and we need an indirect
// block, allocate it.  
int
file_block_walk(struct File *f, u_int filebno, u_int **ppdiskbno, u_int alloc)
{
	int r;
	u_int *ptr;
	void *blk;

	if (filebno < NDIRECT)
		ptr = &f->f_direct[filebno];
	else if (filebno < NINDIRECT) {
		if (f->f_indirect == 0) {
			if (alloc == 0)
				return -E_NOT_FOUND;
			if ((r = alloc_block()) < 0)
				return r;
			f->f_indirect = r;
		}
		if ((r=read_block(f->f_indirect, &blk, 0)) < 0)
			return r;
		assert(blk != 0);
		ptr = (u_int*)blk + filebno;
	} else
		return -E_INVAL;

	*ppdiskbno = ptr;
	return 0;
}

// Set *diskbno to the disk block number for the filebno'th block in file f.
// If alloc is set and the block does not exist, allocate it.
int
file_map_block(struct File *f, u_int filebno, u_int *diskbno, u_int alloc)
{
	int r;
	u_int *ptr;

	if ((r = file_block_walk(f, filebno, &ptr, alloc)) < 0)
		return r;
	if (*ptr == 0) {
		if (alloc == 0)
			return -E_NOT_FOUND;
		if ((r = alloc_block()) < 0)
			return r;
		*ptr = r;
	}
	*diskbno = *ptr;
	return 0;
}

// Remove a block from file f.  If it's not there, just silently succeed.
int
file_clear_block(struct File *f, u_int filebno)
{
	int r;
	u_int *ptr;

	if ((r = file_block_walk(f, filebno, &ptr, 0)) < 0)
		return r;
	if (*ptr) {
		free_block(*ptr);
		*ptr = 0;
	}
	return 0;
}

// Set *blk to point at the filebno'th block in file f.
// 
// Hint: use file_map_block and read_block.
int
file_get_block(struct File *f, u_int filebno, void **blk)
{
	int r, isnew;
	u_int diskbno;

#if SOL >= 5
	if ((r = file_map_block(f, filebno, &diskbno, 1)) < 0)
		return r;

	if ((r=read_block(diskbno, blk, &isnew)) < 0)
		return r;
#else
	// Your code here -- read in the block, leaving the pointer in *blk.
	panic("file_get_block not implemented");
#endif

	// Don't need to maintain reference counts anymore.

	return 0;
}

// Mark the offset/BY2BLK'th block dirty in file f
// by writing its first word to itself.  
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

// Try to find a file named "name" in dir.  If so, set *file to it.
int
dir_lookup(struct File *dir, char *name, struct File **file)
{
	int r;
	u_int i, j, nblock;
	void *blk;
	struct File *f;

	// search dir for name
	nblock = dir->f_size / BY2BLK;
	for (i=0; i<nblock; i++) {
		if ((r=file_get_block(dir, i, &blk)) < 0)
			return r;
		f = blk;
		for (j=0; j<FILE2BLK; j++)
			if (strcmp(f[j].f_name, name) == 0) {
				*file = &f[j];
				f[j].f_dir = dir;
				return 0;
			}
	}
	return -E_NOT_FOUND;	
}

// Set *file to point at a free File structure in dir.
int
dir_alloc_file(struct File *dir, struct File **file)
{
	int r;
	u_int nblock, i , j;
	void *blk;
	struct File *f;

	nblock = dir->f_size / BY2BLK;
	for (i=0; i<nblock; i++) {
		if ((r=file_get_block(dir, i, &blk)) < 0)
			return r;
		f = blk;
		for (j=0; j<FILE2BLK; j++)
			if (f[j].f_name[0] == '\0') {
				*file = &f[j];
				return 0;
			}
	}
	dir->f_size += BY2BLK;
	if ((r = file_get_block(dir, i, &blk)) < 0)
		return r;
	f = blk;
	*file = &f[0];
	return 0;		
}

// Skip over slashes.
char*
skip_slash(char *p)
{
	while (*p == '/')
		p++;
	return p;
}

// Evaluate a path name, starting at the root.
// On success, set *pfile to the file we found
// and set *pdir to the directory the file is in.
// If we cannot find the file but find the directory
// it should be in, set *pdir and copy the final path
// element into lastelem.
int
walk_path(char *path, struct File **pdir, struct File **pfile, char *lastelem)
{
	char *p;
	char name[MAXNAMELEN];
	struct File *dir, *file;
	int r;

	// if (*path != '/')
	//	return -E_BAD_PATH;
	path = skip_slash(path);
	file = &super->s_root;
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

		if (dir->f_type != FTYPE_DIR)
			return -E_NOT_FOUND;

		if ((r=dir_lookup(dir, name, &file)) < 0) {
			if(r == -E_NOT_FOUND && *path == '\0') {
				if(pdir)
					*pdir = dir;
				if (lastelem)
					strcpy(lastelem, name);
				*pfile = 0;
			}
			return r;
		}
	}

	if(pdir)
		*pdir = dir;
	*pfile = file;
	return 0;
}

// Open "path".  On success set *pfile to point at the file and return 0.
// On error return < 0.
int
file_open(char *path, struct File **file)
{
#if SOL >= 5
	return walk_path(path, 0, file, 0);
#else
	// Your code here
	panic("file_open not implemented");
	return 0;
#endif
}

// Create "path".  On success set *file to point at the file and return 0.
// On error return < 0.
int
file_create(char *path, struct File **file)
{
	char name[MAXNAMELEN];
	int r;
	struct File *dir, *f;

	if ((r = walk_path(path, &dir, &f, name)) == 0)
		return -E_FILE_EXISTS;
	if (r != -E_NOT_FOUND || dir == 0)
		return r;
	if (dir_alloc_file(dir, &f) < 0)
		return r;
	strcpy(f->f_name, name);
	*file = f;
	return 0;
}

// Truncate file down to newsize bytes.  
// Since the file is shorter, we can free the blocks
// that were used by the old bigger version but not
// by our new smaller self.  For both the old and new sizes,
// figure out the number of blocks required.
// and then clear the blocks from new_nblocks to old_nblocks.
// If the new_nblocks is no more than NDIRECT, free the indirect
// block too.  (Remember to clear the f->f_indirect pointer so
// you'll know whether it's valid!)
//
// Hint: use file_clear_block.
void
file_truncate(struct File *f, u_int newsize)
{
	int r;
	u_int bno, old_nblocks, new_nblocks;

#if SOL >= 5
	old_nblocks = (f->f_size+BY2BLK-1)/BY2BLK;
	new_nblocks = (newsize+BY2BLK-1)/BY2BLK;
	for (bno=new_nblocks; bno<old_nblocks; bno++)
		if ((r = file_clear_block(f, bno)) < 0)
			printf("warning: file_clear_block: %e", r);

	if (new_nblocks <= NDIRECT && f->f_indirect) {
		free_block(f->f_indirect);
		f->f_indirect = 0;
	}
#else
	// Your code here
	panic("file_truncate not implemented");
#endif
}

int
file_set_size(struct File *f, u_int newsize)
{
	if (f->f_size > newsize)
		file_truncate(f, newsize);
	f->f_size = newsize;
	if (f->f_dir)
		file_flush(f->f_dir);
	return 0;
}

// Flush the contents of file f out to disk.
// Loop over all the blocks in file.
// Translate the file block number into a disk block number
// and then check whether that disk block is dirty.  If so, write it out.
//
// Hint: use file_map_block, block_is_dirty, and write_block.
void
file_flush(struct File *f)
{
#if SOL >= 5
	int i;
	u_int diskbno;

	for (i=0; i < (f->f_size+BY2BLK-1)/BY2BLK; i++) {
		if (file_map_block(f, i, &diskbno, 0) < 0)
			continue;
		if (block_is_dirty(diskbno))
			write_block(diskbno);
	}	
#else
	// Your code here
	panic("file_flush not implemented");
#endif
}

// Sync the entire file system.  A big hammer.
void
fs_sync(void)
{
	int i;

	for (i=0; i<super->s_nblocks; i++)
		if (block_is_dirty(i))
			write_block(i);
}

// Close a file.
void
file_close(struct File *f)
{
	file_flush(f);
	if (f->f_dir)
		file_flush(f->f_dir);
}

// Remove a file by truncating it and then zeroing the name.
int
file_remove(char *path)
{
	int r;
	struct File *f;

	if ((r = walk_path(path, 0, &f, 0)) < 0)
		return r;

	file_truncate(f, 0);
	f->f_name[0] = '\0';
	file_flush(f);
	if (f->f_dir)
		file_flush(f->f_dir);

	return 0;
}

#endif
