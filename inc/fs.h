#if LAB >= 5
/* See COPYRIGHT for copyright information. */

#ifndef _FS_H_
#define _FS_H_

#include <inc/types.h>


/*** File nodes (both in-memory and on-disk) ***/

/* Maximum size of a filename (a single path component), including null */
#define MAXNAMELEN	128

/* Number of (direct) block pointers in a File descriptor */
#define NFILEBLOCKS	10

struct File {
	u_char name[MAXNAMELEN];	/* filename */
	u_int size;			/* file size in bytes */
	u_int mode;			/* file type and permissions */
	u_int block[NFILEBLOCKS];	/* direct block pointers */
	u_int indir;			/* pointer to (single) indirect block */

	/* The remaining fields are only valid in memory, not on disk. */
	u_int nopen;			/* number of current opens */
	void *pages[NFILEBLOCKS];	/* in-memory pages */
	void **indir;			/* pointer to indirect page table */
};

#define FM_DIR		0x0008	/* is directory */
#define FM_READ		0x0004	/* read permission */
#define FM_WRITE	0x0002	/* write permission */
#define FM_EXEC		0x0001	/* execute permission */


/*** File system super-block (both in-memory and on-disk) ***/

#define FS_MAGIC	6828	/* Everyone's favorite OS class */

struct Super {
	u_int magic;		/* Magic number: FS_MAGIC */
	u_int nblocks;		/* Total number of blocks on disk */
	struct File root;	/* Root directory node */
};


/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP+(n*BY2PG). */
#define DISKMAP		0x10000000

/* Maximum disk size we can handle (3GB) */
#define DISKMAX		0xc0000000


#endif /* _FS_H_ */
#endif /* LAB >= 5 */
