#if LAB >= 5
/*
 * File system server main loop -
 * serves IPC requests from other environments.
 */

#include "fs.h"

#define debug 0

struct Open {
	struct File *file;	/* mapped descriptor for open file */
	u_int envid;		/* envid of client */
	int mode;		/* open mode */
};

/* Max number of open files in the file system at once */
#define MAXOPEN	1024

struct Open opentab[MAXOPEN];

/* Virtual address at which to receive page mappings
 * containing client requests. */
#define REQVA	0x0ffff000


void
serve_open(u_int envid, struct Fsreq_open *rq)
{
	u_char path[MAXPATHLEN];
	struct File *f;
	int fileid;
	int rc;

	if (debug) printf("serve_open %08x %s 0x%x\n", envid, rq->req_path, rq->req_omode);
	// Copy in the path, making sure it's null-terminated
	bcopy(rq->req_path, path, MAXPATHLEN);
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
	opentab[fileid].mode = rq->req_omode;
	rc = 0;

	rq->req_size = f->size;
	rq->req_fileid = fileid;

	out:
	ipc_send(envid, rc, 0, 0);
}

void
serve_map(u_int envid, struct Fsreq_map *rq)
{
	int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];
	void *blk;
	int perm;
	int rc;

	if (debug) printf("serve_map %08x %08x %08x\n", envid, rq->req_fileid, rq->req_offset);

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Locate and read (if necessary) the appropriate block/page.
	rc = file_get_block(o->file, rq->req_offset/BY2BLK, &blk);
	if (rc < 0)
		goto out;

	// Send the page containing this block to the user via IPC,
	// with appropriate permissions according to the file open mode
	perm = PTE_U | PTE_P;
	if ((o->mode & O_ACCMODE) != O_RDONLY)
		perm |= PTE_W;
	ipc_send(envid, 0, (u_int)blk, perm);
	return;

	out:
	ipc_send(envid, rc, 0, 0);
}

void
serve_set_size(u_int envid, struct Fsreq_set_size *rq)
{
	int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];
	int rc;

	if (debug) printf("serve_set_size %08x %08x %08x\n", envid, rq->req_fileid, rq->req_size);

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Resize the file as requested
	rc = file_set_size(o->file, rq->req_size);

	// Reply to the client
	out:
	ipc_send(envid, rc, 0, 0);
}

void
serve_close(u_int envid, struct Fsreq_close *rq)
{
	int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];
	int rc;

	if (debug) printf("serve_close %08x %08x\n", envid, rq->req_fileid);

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Close the file and clear the open table entry
	file_close(o->file);
	o->file = 0;
	rc = 0;

	// Reply to the client
out:
	ipc_send(envid, rc, 0, 0);
}

void
serve_remove(u_int envid, struct Fsreq_remove *rq)
{
	u_char path[MAXPATHLEN];
	int rc;

	if (debug) printf("serve_map %08x %s\n", envid, rq->req_path);

	// Copy in the path, making sure it's null-terminated
	bcopy(rq->req_path, path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;

	// Delete the specified file
	rc = file_remove(path);
	ipc_send(envid, rc, 0, 0);
}

void
serve_dirty(u_int envid, struct Fsreq_dirty *rq)
{
	int rc;
	u_int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];

	if (debug) printf("serve_dirty %08x %08x %08x\n", envid, rq->req_fileid, rq->req_offset);

	// Validate fileid passed by user
	
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->file == 0 || o->envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Resize the file as requested
	rc = file_dirty(o->file, rq->req_offset);

	// Reply to the client
	out:
	ipc_send(envid, rc, 0, 0);
}

void
serve_sync(u_int envid)
{
	fs_sync();
	ipc_send(envid, 0, 0, 0);
}

void
serve(void)
{
	u_int req, whom, perm;

	for(;;) {
		perm = 0;
		req = ipc_recv(&whom, REQVA, &perm);
		if (debug)
			printf("fs req %d from %08x [page %08x: %s]\n",
				req, whom, vpt[VPN(REQVA)], REQVA);

		/* All requests must contain an argument page */
		if (!(perm & PTE_P)) {
			printf("Invalid request from %08x: no argument page\n",
				whom);
			continue; /* just leave it hanging... */
		}

		switch (req) {
		case FSREQ_OPEN:
			serve_open(whom, (struct Fsreq_open*)REQVA);
			break;
		case FSREQ_MAP:
			serve_map(whom, (struct Fsreq_map*)REQVA);
			break;
		case FSREQ_SET_SIZE:
			serve_set_size(whom, (struct Fsreq_set_size*)REQVA);
			break;
		case FSREQ_CLOSE:
			serve_close(whom, (struct Fsreq_close*)REQVA);
			break;
		case FSREQ_DIRTY:
			serve_dirty(whom, (struct Fsreq_dirty*)REQVA);
			break;
		case FSREQ_REMOVE:
			serve_remove(whom, (struct Fsreq_remove*)REQVA);
			break;
		case FSREQ_SYNC:
			serve_sync(whom);
			break;
		default:
			printf("Invalid request code %d from %08x\n", whom, req);
			break;
		}
		sys_mem_unmap(0, REQVA);
	}
}

void
umain(void)
{
	fs_init();
	serve();
}

#endif
