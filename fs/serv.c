#if LAB >= 5
/*
 * File system server main loop -
 * accepts and services IPC requests from other environments.
 */

#include "lib.h"
#include "fs.h"


struct Open {
	struct File *file;	/* mapped descriptor for open file */
	u_int envid;		/* envid of client */
	int mode;		/* open mode */
};

/* Max number of open files in the file system at once */
#define MAXOPEN	1024

struct Open *opentab[MAXOPEN];


/* Virtual address at which to receive page mappings
 * containing client requests. */
#define REQVA	0x0ffff000


// Service an "open" request from a client.
void
serve_open(u_int envid, struct fsrq_open *rq)
{
#if SOL >= 5
	u_char path[MAXPATHLEN];
	struct File *f;
	int fileid;
	int rc;

	// Copy in the path, making sure it's null-terminated
	memcpy(path, rq->path, MAXPATHLEN-1);
	path[MAXPATHLEN-1] = 0;

	// Find an available open-file table entry
	for (fileid = 0; fileid < MAXOPEN; fileid++) {
		if (opentab[fileid].file == 0)
			break;
	}
	if (fileid == MAXOPEN) {
		rc = -E_MAX_OPEN;
		goto out;
	}

	// Find and open the file
	rc = file_open(path, &f);
	if (rc < 0)
		goto out;

	// Fill out the open-table entry we just allocated
	opentab[fileid].file = f;
	opentab[fileid].envid = envid;
	opentab[fileid].mode = rq->mode;
	rc = fileid;

	out:
	ipc_send(envid, rc, 0, 0);
#endif /* SOL >= 5 */
}

// Service a "map" request from a client.
void
serve_map(u_int envid, struct fsrq_map *rq)
{
#if SOL >= 5
	int fileid = rq->fileid;
	struct Open *o = &opentab[fileid];
	void *blk;
	int perm;
	int rc;

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Locate and read (if necessary) the appropriate block/page.
	rc = file_getblk(o->file, VPN(rq->ofs), &blk);
	if (rc < 0)
		goto out;

	// Send the page containing this block to the user via IPC,
	// with appropriate permissions according to the file open mode
	perm = PTE_U | PTE_P;
	if ((o->mode & O_ACCMODE) != O_RDONLY)
		perm |= PTE_W;
	ipc_send(envid, 0, blk, perm);
	return;

	out:
	ipc_send(envid, rc, 0, 0);
#endif /* SOL >= 5 */
}

// Service a "resize" request from a client.
void
serve_resize(u_int envid, struct fsrq_resize *rq)
{
#if SOL >= 5
	int fileid = rq->fileid;
	struct Open *o = &opentab[fileid];
	int rc;

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Resize the file as requested
	rc = file_resize(o->file, rq->newsize);

	// Reply to the client
	out:
	ipc_send(envid, rc, 0, 0);
#endif /* SOL >= 5 */
}

// Service a "close" request from a client.
void
serve_close(u_int envid, struct fsrq_close *rq)
{
#if SOL >= 5
	int fileid = rq->fileid;
	struct Open *o = &opentab[fileid];
	int rc;

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto err;
	}

	// Close the file and clear the open table entry
	file_close(o->file);
	o->file = 0;
	rc = 0;

	// Reply to the client
	out:
	ipc_send(envid, rc, 0, 0);
#endif /* SOL >= 5 */
}

// Service a "delete" request from the client
void serve_delete(void)
{
#if SOL >= 5
	u_char path[MAXPATHLEN];
	int rc;

	// Copy in the path, making sure it's null-terminated
	memcpy(path, rq->path, MAXPATHLEN-1);
	path[MAXPATHLEN-1] = 0;

	// Delete the specified file
	rc = file_delete(path);
	ipc_send(envid, rc, 0, 0);
#endif /* SOL >= 5 */
}

void
serve_requests(void)
{
	u_int req, whom, perm;

	while (1) {
		req = ipc_recv(&whom, REQVA, &perm);

		/* All requests must contain an argument page */
		if (!(perm & PTE_P)) {
			printf("Invalid request from %08x: no argument page\n",
				whom);
			continue; /* just leave it hanging... */
		}

		switch (req) {
		case FSRQ_OPEN:
			serve_open(whom, (struct fsrq_open*)REQVA);
			break;
		case FSRQ_MAP:
			serve_close(whom, (struct fsrq_map*)REQVA);
			break;
		case FSRQ_RESIZE:
			serve_close(whom, (struct fsrq_resize*)REQVA);
			break;
		case FSRQ_CLOSE:
			serve_close(whom, (struct fsrq_close*)REQVA);
			break;
		case FSRQ_DELETE:
			serve_close(whom, (struct fsrq_delete*)REQVA);
			break;
		default:
			printf("Invalid request code %d from %08x\n", whom, req);
			break;
		}
	}
}

void
umain(void)
{
	int child;

	assert(DIRENT_SIZE >= sizeof(struct File));

	read_super();
	read_bitmap();

	/* Fork off a client to use the file system */
	child = fork();
	if (child < 0)
		panic("can't fork first child environment");
	if (child == 0) {
		/* we're the child */
		panic("XXX exec");
	}

	serve_requests();
}

#endif /* LAB >= 5 */
