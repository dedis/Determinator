
#include <inc/fs.h>


/* IDE disk number to look on for our file system */
#define DISKNO		1

#define BY2SECT		512	/* Bytes per disk sector */
#define SECT2PG		(BY2PG/BY2SECT)	/* sectors to a page */


/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP+(n*BY2PG). */
#define DISKMAP		0x10000000

/* Maximum disk size we can handle (3GB) */
#define DISKMAX		0xc0000000



/* ide.c */
void read_sectors(u_int diskno, u_int secno, void *dst, u_int nsecs);
void write_sectors(u_int diskno, u_int secno, void *src, u_int nsecs);

/* fs.c */
int file_open(u_char *path, struct File **out_file);
int file_getblk(struct File *f, u_int blockno, void **out_blk);
int file_resize(struct File *f, u_int newsize);
void file_close(struct File *f);
int file_delete(u_char *path);

