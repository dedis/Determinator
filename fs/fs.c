#if LAB >= 5

#include <inc/mmu.h>

#include "lib.h"
#include "fs.h"

struct Super *super;

u_int nbmblocks;	/* number of bitmap blocks */
u_int *bitmap;		/* bitmap blocks mapped in memory */


void *
read_block(u_int blockno)
{
#if SOL >= 5
	u_int va = DISKMAP + blockno*BY2PG;
	int rc;

	/* check if already loaded */
	if (vpd[PDX(va)] && vpt[VPN(va)])
		return (void*)va;

	/* Allocate a page to hold the disk block */
	rc = sys_mem_alloc(0, va, PTE_U|PTE_P|PTE_W);
	if (rc < 0)
		return 0;

	read_sectors(DISKNO, blockno*SECT2PG, (void*)va, SECT2PG);

	return (void*)va;
#endif
}

void
write_block(u_int blockno)
{
#if SOL >= 5
	u_int va = DISKMAP + blockno*BY2PG;

	write_sectors(DISKNO, blockno*SECT2PG, (void*)va, SECT2PG);
#endif
}


// Check to see if the block bitmap indicates that block 'blockno' is free.
// Returns 1 if the block is free, 0 if not.
int test_block(u_int blockno)
{
	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;
	return 0;
}

// Search the bitmap for a free block and allocate it.
// Return block number allocated on success, < 0 on failure.
int
alloc_block(void)
{
#if SOL >= 5
	int i = 0; 

	for (i = 0; i < super->nblocks/32; i++) {
		if (bitmap[i] != 0) {
			/* Some bit in this word is set - find it */
			int j;
			for (j = 0; j < 32; j++) {
				if (bitmap[i] & (1 << j)) {
					/* allocate it and return */
					bitmap[i] &= ~(1 << j);
					return i*32+j;
				}
			}
			panic("couldn't find set bit!");
		}
	}

	return -E_NO_SPACE;
#else
	// Your code here
#endif
}

// Mark a block free in the bitmap
void
free_block(u_int blockno)
{
#if SOL >= 5
	bitmap[blockno / 32] |= 1 << (blockno % 32);
#else
	// Your code here
#endif
}


// Read and validate the file system super-block.
void
read_super(void)
{
	super = read_block(1);
	if (super == 0)
		panic("Can't read superblock!");

	if (super->magic != FS_MAGIC)
		panic("Bad file system magic number!");

	if (super->nblocks > DISKMAX/BY2BLK)
		panic("File system is too large!");
}

// Read and validate the file system bitmap.
void
read_bitmap(void)
{
	u_int i;

	for (i = 0; i < super->nblocks; i += BIT2BLK) {
		bmblk = read_block(2+i/BIT2BLK);
		if (bmblk == 0)
			panic("Can't read bitmap block %d", i);

		/* Make sure all bitmap blocks are marked in-use */
		assert(!test_block(2+i/BIT2BLK));
	}

	/* Make sure the reserved and root blocks are marked in-use */
	assert(!test_block(0));
	assert(!test_block(1));
}


int
traverse(u_char *path, struct File **out_dir, u_char **out_filename)
{
	...
}

int
file_open(u_char *path, struct File **out_file)
{
	int rc;
	File *dir;
	u_char *filename;

	// Traverse the hierarchy to the directory containing the file
	rc = traverse(path, &dir, &filename);
	if (rc < 0)
		return rc;

	...
}

int
file_getblk(struct File *f, u_int blockno, void **out_blk)
{
}

int
file_resize(struct File *f, u_int newsize)
{
}

void
file_close(struct File *f)
{
}

int
file_delete(u_char *path)
{
}


#endif /* LAB >= 5 */
