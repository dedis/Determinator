#if LAB >= 5
#include <inc/string.h>

#include "fs.h"

struct Super *super;

#if SOL >= 99
static uint32_t nbitmap;	// number of bitmap blocks
#endif
uint32_t *bitmap;		// bitmap blocks mapped in memory

void file_flush(struct File *f);
bool block_is_free(uint32_t blockno);

// Return the virtual address of this disk block.
char*
diskaddr(uint32_t blockno)
{
	if (super && blockno >= super->s_nblocks)
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (vpd[PDX(va)] & PTE_P) && (vpt[VPN(va)] & PTE_P);
}

// Is this disk block mapped?
bool
block_is_mapped(uint32_t blockno)
{
	char *va = diskaddr(blockno);
	return va_is_mapped(va) && va != 0;
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (vpt[VPN(va)] & PTE_D) != 0;
}

// Is this block dirty?
bool
block_is_dirty(uint32_t blockno)
{
	char *va = diskaddr(blockno);
	return va_is_mapped(va) && va_is_dirty(va);
}

// Allocate a page to hold the disk block
int
map_block(uint32_t blockno)
{
	if (block_is_mapped(blockno))
		return 0;
	return sys_page_alloc(0, diskaddr(blockno), PTE_U|PTE_P|PTE_W);
}

// Make sure a particular disk block is loaded into memory.
// Returns 0 on success, or a negative error code on error.
// 
// If blk != 0, set *blk to the address of the block in memory.
//
// Hint: Use diskaddr, block_is_mapped, sys_page_alloc, and ide_read.
// Hint: Use the DISKNO constant when calling ide_read.
// Hint: If you loaded the block from disk, use sys_page_map to clear the
// corresponding page's PTE_D bit.  (This is an optimization.)
int
read_block(uint32_t blockno, char **blk)
{
	int r;
	char *addr;

	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);

#if SOL >= 5
	addr = diskaddr(blockno);

	if (va_is_mapped(addr)) {
		if (blk)
			*blk = (void*)addr;
		return 0;
	}

	if ((r = map_block(blockno)) < 0)
		return r;

	ide_read(DISKNO, blockno * BLKSECTS, (void*) addr, BLKSECTS);
	if (blk)
		*blk = (void*) addr;

	return 0;
#else
	// LAB 5: Your code here.
	panic("read_block not implemented");
	return 0;
#endif
}

// Copy the current contents of the block out to disk.
// Then clear the PTE_D bit using sys_page_map.
// Hint: Use ide_write.
// Hint: Use the PTE_USER constant when calling sys_page_map.
void
write_block(uint32_t blockno)
{
	char *addr;

	if (!block_is_mapped(blockno))
		panic("write unmapped block %08x", blockno);
	
#if SOL >= 5
	addr = diskaddr(blockno);

	ide_write(DISKNO, blockno * BLKSECTS, (void*) addr, BLKSECTS);

	// clear dirty flag in pte
	sys_page_map(0, addr, 0, addr, vpt[VPN(addr)] & PTE_USER);
#else
	// Write the disk block and clear PTE_D.
	// LAB 5: Your code here.
	panic("write_block not implemented");
#endif
}

// Make sure this block is unmapped.
void
unmap_block(uint32_t blockno)
{
	int r;

	if (!block_is_mapped(blockno))
		return;

	assert(block_is_free(blockno) || !block_is_dirty(blockno));

	if ((r = sys_page_unmap(0, diskaddr(blockno))) < 0)
		panic("unmap_block: sys_mem_unmap: %e", r);
	assert(!block_is_mapped(blockno));
}

// Check to see if the block bitmap indicates that block 'blockno' is free.
// Return 1 if the block is free, 0 if not.
bool
block_is_free(uint32_t blockno)
{
	if (super == 0 || blockno >= super->s_nblocks)
		return 0;
	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;
	return 0;
}

// Mark a block free in the bitmap
void
free_block(uint32_t blockno)
{
	// Blockno zero is the null pointer of block numbers.
	if (blockno == 0)
		panic("attempt to free zero block");
	bitmap[blockno/32] |= 1<<(blockno%32);
}

// Search the bitmap for a free block and allocate it.
// 
// Return block number allocated on success,
// -E_NO_DISK if we are out of blocks.
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
			write_block(2 + j/BLKBITSIZE);
			lastalloc = j;
			return j;
		}
	}
#else
	// LAB 5: Your code here.
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
	char *blk;

	if ((r = read_block(1, &blk)) < 0)
		panic("cannot read superblock: %e", r);

	super = (struct Super*) blk;
	if (super->s_magic != FS_MAGIC)
		panic("bad file system magic number");

	if (super->s_nblocks > DISKSIZE/BLKSIZE)
		panic("file system is too large");

	printf("superblock is good\n");
}

// Read and validate the file system bitmap.
//
// Read all the bitmap blocks into memory.
// Set the "bitmap" pointer to point at the beginning of the first
// bitmap block.
// 
// Check that all reserved blocks -- 0, 1, and the bitmap blocks themselves --
// are all marked as in-use
// (for each block i, assert(!block_is_free(i))).
//
// Hint: Assume that the superblock has already been loaded into
// memory (in variable 'super').  Check out super->s_nblocks.
void
read_bitmap(void)
{
	int r;
	uint32_t i;
	char *blk;

#if SOL >= 5
	for (i = 0; i * BLKBITSIZE < super->s_nblocks; i++) {
		if ((r = read_block(2+i, &blk)) < 0)
			panic("cannot read bitmap block %d: %e", i, r);
		if (i == 0)
			bitmap = (uint32_t*) blk;
		// Make sure all bitmap blocks are marked in-use
		assert(!block_is_free(2+i));
	}
#else
	// Read the bitmap into memory.
	// The bitmap consists of one or more blocks.  A single bitmap block
	// contains the in-use bits for BLKBITSIZE blocks.  There are
	// super->s_nblocks blocks in the disk altogether.
	// Set 'bitmap' to point to the first address in the bitmap.
	// Hint: Use read_block.

	// LAB 5: Your code here.
	panic("read_bitmap not implemented");
#endif

	// Make sure the reserved and root blocks are marked in-use.
	assert(!block_is_free(0));
	assert(!block_is_free(1));
	assert(bitmap);

#if SOL >= 5
#else
	// Make sure that the bitmap blocks are marked in-use.
	// LAB 5: Your code here.

#endif
	printf("read_bitmap is good\n");
}

// Test that write_block works, by smashing the superblock and reading it back.
void
check_write_block(void)
{
	super = 0;

	// back up super block
	read_block(0, 0);
	memcpy(diskaddr(0), diskaddr(1), PGSIZE);

	// smash it 
	strcpy(diskaddr(1), "OOPS!\n");
	write_block(1);
	assert(block_is_mapped(1));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!block_is_mapped(1));

	// read it back in
	read_block(1, 0);
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memcpy(diskaddr(1), diskaddr(0), PGSIZE);
	write_block(1);
	super = (struct Super*)diskaddr(1);

	printf("write_block is good\n");
}

// Initialize the file system
void
fs_init(void)
{
	static_assert(sizeof(struct File) == 256);

	read_super();
	check_write_block();
	read_bitmap();
}

// Find the disk block number slot for the 'filebno'th block in file 'f'.
// Set '*ppdiskbno' to point to that slot.
// The slot will be one of the f->f_direct[] entries,
// or an entry in the indirect block.
// When 'alloc' is set, this function will allocate an indirect block
// if necessary.
//
// Returns:
//	0 on success (but note that *ppdiskbno might equal 0).
//	-E_NOT_FOUND if the function needed to allocate an indirect block, but
//		alloc was 0.
//	-E_NO_DISK if there's no space on the disk for an indirect block.
//	-E_NO_MEM if there's no space in memory for an indirect block.
//	-E_INVAL if filebno is out of range (it's >= NINDIRECT).
//
// Analogy: This is like pgdir_walk for files.  
int
file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
{
	int r;
	uint32_t *ptr;
	char *blk;

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
		if ((r = read_block(f->f_indirect, &blk)) < 0)
			return r;
		assert(blk != 0);
		ptr = (uint32_t*)blk + filebno;
	} else
		return -E_INVAL;

	*ppdiskbno = ptr;
	return 0;
}

// Set '*diskbno' to the disk block number for the 'filebno'th block
// in file 'f'.
// If 'alloc' is set and the block does not exist, allocate it.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_NOT_FOUND if alloc was 0 but the block did not exist.
//	-E_NO_DISK if a block needed to be allocated but the disk is full.
//	-E_NO_MEM if we're out of memory.
//	-E_INVAL if filebno is out of range.
int
file_map_block(struct File *f, uint32_t filebno, uint32_t *diskbno, bool alloc)
{
	int r;
	uint32_t *ptr;

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
// Returns 0 on success, < 0 on error.
int
file_clear_block(struct File *f, uint32_t filebno)
{
	int r;
	uint32_t *ptr;

	if ((r = file_block_walk(f, filebno, &ptr, 0)) < 0)
		return r;
	if (*ptr) {
		free_block(*ptr);
		*ptr = 0;
	}
	return 0;
}

// Set *blk to point at the filebno'th block in file 'f'.
// Allocate the block if it doesn't yet exist.
// Returns 0 on success, < 0 on error.
int
file_get_block(struct File *f, uint32_t filebno, char **blk)
{
	int r;
	uint32_t diskbno;

#if SOL >= 5
	if ((r = file_map_block(f, filebno, &diskbno, 1)) < 0)
		return r;
	if ((r = read_block(diskbno, blk)) < 0)
		return r;
#else
	// Read in the block, leaving the pointer in *blk.
	// Hint: Use file_map_block and read_block.
	// LAB 5: Your code here.
	panic("file_get_block not implemented");
#endif
	return 0;
}

// Mark the offset/BLKSIZE'th block dirty in file f
// by writing its first word to itself.  
int
file_dirty(struct File *f, off_t offset)
{
	int r;
	char *blk;

	if ((r = file_get_block(f, offset/BLKSIZE, &blk)) < 0)
		return r;
	*(volatile char*)blk = *(volatile char*)blk;
	return 0;
}

// Try to find a file named "name" in dir.  If so, set *file to it.
int
dir_lookup(struct File *dir, const char *name, struct File **file)
{
	int r;
	uint32_t i, j, nblock;
	char *blk;
	struct File *f;

	// Search dir for name.
	// We maintain the invariant that the size of a directory-file
	// is always a multiple of the file system's block size.
	assert((dir->f_size % BLKSIZE) == 0);
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
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
	uint32_t nblock, i, j;
	char *blk;
	struct File *f;

	assert((dir->f_size % BLKSIZE) == 0);
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
			if (f[j].f_name[0] == '\0') {
				*file = &f[j];
				return 0;
			}
	}
	dir->f_size += BLKSIZE;
	if ((r = file_get_block(dir, i, &blk)) < 0)
		return r;
	f = (struct File*) blk;
	*file = &f[0];
	return 0;
}

// Skip over slashes.
static inline const char*
skip_slash(const char *p)
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
static int
walk_path(const char *path, struct File **pdir, struct File **pfile, char *lastelem)
{
	const char *p;
	char name[MAXNAMELEN];
	struct File *dir, *file;
	int r;

	// if (*path != '/')
	//	return -E_BAD_PATH;
	path = skip_slash(path);
	file = &super->s_root;
	dir = 0;
	name[0] = 0;

	if (pdir)
		*pdir = 0;
	*pfile = 0;
	while (*path != '\0') {
		dir = file;
		p = path;
		while (*path != '/' && *path != '\0')
			path++;
		if (path - p >= MAXNAMELEN)
			return -E_BAD_PATH;
		memcpy(name, p, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (dir->f_type != FTYPE_DIR)
			return -E_NOT_FOUND;

		if ((r = dir_lookup(dir, name, &file)) < 0) {
			if (r == -E_NOT_FOUND && *path == '\0') {
				if (pdir)
					*pdir = dir;
				if (lastelem)
					strcpy(lastelem, name);
				*pfile = 0;
			}
			return r;
		}
	}

	if (pdir)
		*pdir = dir;
	*pfile = file;
	return 0;
}

// Create "path".  On success set *file to point at the file and return 0.
// On error return < 0.
int
file_create(const char *path, struct File **file)
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

// Open "path".  On success set *pfile to point at the file and return 0.
// On error return < 0.
int
file_open(const char *path, struct File **file)
{
#if SOL >= 5
	return walk_path(path, 0, file, 0);
#else
	// Hint: Use walk_path.
	// LAB 5: Your code here.
	panic("file_open not implemented");
	return 0;
#endif
}

// Remove any blocks currently used by file 'f',
// but not necessary for a file of size 'newsize'.
// For both the old and new sizes, figure out the number of blocks required,
// and then clear the blocks from new_nblocks to old_nblocks.
// If the new_nblocks is no more than NDIRECT, and the indirect block has
// been allocated (f->f_indirect != 0), then free the indirect block too.
// (Remember to clear the f->f_indirect pointer so you'll know
// whether it's valid!)
// Do not change f->f_size.
static void
file_truncate_blocks(struct File *f, off_t newsize)
{
	int r;
	uint32_t bno, old_nblocks, new_nblocks;

#if SOL >= 5
	old_nblocks = (f->f_size + BLKSIZE - 1) / BLKSIZE;
	new_nblocks = (newsize + BLKSIZE - 1) / BLKSIZE;
	for (bno = new_nblocks; bno < old_nblocks; bno++)
		if ((r = file_clear_block(f, bno)) < 0)
			printf("warning: file_clear_block: %e", r);

	if (new_nblocks <= NDIRECT && f->f_indirect) {
		free_block(f->f_indirect);
		f->f_indirect = 0;
	}
#else
	// Hint: Use file_clear_block and/or free_block.
	// LAB 5: Your code here.
	panic("file_truncate_blocks not implemented");
#endif
}

int
file_set_size(struct File *f, off_t newsize)
{
	if (f->f_size > newsize)
		file_truncate_blocks(f, newsize);
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
	uint32_t diskbno;

	for (i = 0; i < (f->f_size + BLKSIZE - 1) / BLKSIZE; i++) {
		if (file_map_block(f, i, &diskbno, 0) < 0)
			continue;
		if (block_is_dirty(diskbno))
			write_block(diskbno);
	}	
#else
	// LAB 5: Your code here.
	panic("file_flush not implemented");
#endif
}

// Sync the entire file system.  A big hammer.
void
fs_sync(void)
{
	int i;
	for (i = 0; i < super->s_nblocks; i++)
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
file_remove(const char *path)
{
	int r;
	struct File *f;

	if ((r = walk_path(path, 0, &f, 0)) < 0)
		return r;

	file_truncate_blocks(f, 0);
	f->f_name[0] = '\0';
	file_flush(f);
	if (f->f_dir)
		file_flush(f->f_dir);

	return 0;
}

#endif
