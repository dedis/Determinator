#if LAB >= 5
/* See COPYRIGHT for copyright information. */

#ifndef _FS_H_
#define _FS_H_

#include <inc/types.h>


/*** File nodes (both in-memory and on-disk) ***/

/* Bytes per file system block - same as page size */
#define BY2BLK		BY2PG
#define BIT2BLK		(BY2BLK*8)

/* Maximum size of a filename (a single path component), including null */
#define MAXNAMELEN	128

/* Maximum size of a complete pathname, including null */
#define MAXPATHLEN	1024

/* Number of (direct) block pointers in a File descriptor */
#define NFILEBLOCKS	10

struct File {
	u_char name[MAXNAMELEN];	/* filename */
	u_int size;			/* file size in bytes */
	u_int type;			/* file type */
	u_int block[NFILEBLOCKS];	/* direct block pointers */
	u_int bindir;			/* pointer to (single) indirect block */

	/* The remaining fields are only valid in memory, not on disk. */
	u_int nopen;			/* number of current opens */
	void *pages[NFILEBLOCKS];	/* in-memory pages */
	void **pindir;			/* pointer to indirect page table */
};

/* Amount of space reserved for each entry in a directory:
 * DIRENT_SIZE >= sizeof(struct File), and is a power of two */
#define DIRENT_SIZE	128

/* File types */
#define FT_REG		0	/* Regular file */
#define FT_DIR		1	/* Directory */


/*** File system super-block (both in-memory and on-disk) ***/

#define FS_MAGIC	6828	/* Everyone's favorite OS class */

struct Super {
	u_int magic;		/* Magic number: FS_MAGIC */
	u_int nblocks;		/* Total number of blocks on disk */
	struct File root;	/* Root directory node */
};


/*** Definitions for requests from clients to file system ***/

#define FSRQ_OPEN	1
#define FSRQ_MAP	2
#define FSRQ_RESIZE	3
#define FSRQ_CLOSE	4
#define FSRQ_DELETE	6

struct fsrq_open {
	u_char path[MAXPATHLEN];
	int mode;
};

struct fsrq_close {
	int fileid;
};

struct fsrq_resize {
	int fileid;
	u_int newsize;
};

struct fsrq_map {
	int fileid;
	u_int ofs;
};

struct fsrq_delete {
	u_char path[MAXPATHLEN];
};


#endif /* _FS_H_ */
#endif /* LAB >= 5 */
