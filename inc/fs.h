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
#define NDIRECT		10
#define NINDIRECT	(BY2BLK/4)

#define MAXFILESIZE	(NINDIRECT*BY2BLK)

struct File {
	u_char name[MAXNAMELEN];	/* filename */
	u_int size;			/* file size in bytes */
	u_int type;			/* file type */
	u_int direct[NDIRECT];
	u_int indirect;

	/* The remaining fields are only valid in memory, not on disk. */
	u_int ref;			/* number of current opens */
	struct File *dir;
};

#define FILE2BLK	(BY2BLK/sizeof(struct File))

/* File types */
#define FTYPE_REG		0	/* Regular file */
#define FTYPE_DIR		1	/* Directory */


/*** File system super-block (both in-memory and on-disk) ***/

#define FS_MAGIC	6828	/* Everyone's favorite OS class */

struct Super {
	u_int magic;		/* Magic number: FS_MAGIC */
	u_int nblocks;		/* Total number of blocks on disk */
	struct File root;	/* Root directory node */
};


/*** Definitions for requests from clients to file system ***/

#define FSREQ_OPEN	1
#define FSREQ_MAP	2
#define FSREQ_SET_SIZE	3
#define FSREQ_CLOSE	4
#define FSREQ_DIRTY	5
#define FSREQ_REMOVE	6
#define FSREQ_SYNC	7


struct Fsreq_open {
	char req_path[MAXPATHLEN];
	u_int req_omode;
	u_int req_fileid;
	u_int req_size;
};

struct Fsreq_map {
	int req_fileid;
	u_int req_offset;
};

struct Fsreq_set_size {
	int req_fileid;
	u_int req_size;
};

struct Fsreq_close {
	int req_fileid;
};

struct Fsreq_dirty {
	int req_fileid;
	u_int req_offset;
};

struct Fsreq_remove {
	u_char req_path[MAXPATHLEN];
};

#endif /* _FS_H_ */
#endif
