#if LAB >= 5
/*
 * File system server main loop -
 * serves IPC requests from other environments.
 */

#include "fs.h"
#include <inc/x86.h>

#define debug 0

struct Open {
	struct File *o_file;	// mapped descriptor for open file
	u_int o_envid;		// envid of client
	int o_mode;		// open mode
};

// Max number of open files in the file system at once
#define MAXOPEN	1024

// initialize to force into data section
struct Open opentab[MAXOPEN] = { { 0, 0, 1 } };

// Virtual address at which to receive page mappings containing client requests.
#define REQVA	0x0ffff000

// Allocate an open file.
int
open_alloc(struct Open **o)
{
	int fileid;

	// Find an available open-file table entry
	for (fileid = 0; fileid < MAXOPEN; fileid++) {
		if (opentab[fileid].o_file == 0) {
			*o = &opentab[fileid];
			return fileid;
		}
	}
	return -E_MAX_OPEN;
}

// Look up an open file for envid.
int
open_lookup(u_int envid, u_int fileid, struct Open **o)
{
	if (fileid >= MAXOPEN || opentab[fileid].o_envid != envid)
		return -E_INVAL;
	*o = &opentab[fileid];
	return 0;
}

// Serve requests, sending responses back to envid.
// To send a result back, ipc_send(envid, r, 0, 0).
// To include a page, ipc_send(envid, r, srcva, perm).

void
serve_open(u_int envid, struct Fsreq_open *rq)
{
	if (debug) printf("serve_open %08x %s 0x%x\n", envid, rq->req_path, rq->req_omode);

	u_char path[MAXPATHLEN];
	struct File *f;
	int fileid;
	int r;
	struct Open *o;

	// Copy in the path, making sure it's null-terminated
	bcopy(rq->req_path, path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;

	// Find a file id.
	if ((r = open_alloc(&o)) < 0)
		goto out;
	fileid = r;

	// Open the file.
	if ((r = file_open(path, &f)) < 0)
		goto out;

	o->o_file = f;
	o->o_envid = envid;
	o->o_mode = rq->req_omode;

	// Fill out return params and return success
	r = 0;
	rq->req_size = f->f_size;
	rq->req_fileid = fileid;

out:
	ipc_send(envid, r, 0, 0);
}

void
serve_map(u_int envid, struct Fsreq_map *rq)
{
	if (debug) printf("serve_map %08x %08x %08x\n", envid, rq->req_fileid, rq->req_offset);

#if SOL >= 5
	int r;
	void *blk;
	struct Open *o;
	u_int perm;

	if ((r = open_lookup(envid, rq->req_fileid, &o)) < 0)
		goto out;

	if ((r = file_get_block(o->o_file, rq->req_offset/BY2BLK, &blk)) < 0)
		goto out;

	perm = PTE_U|PTE_P;
	if ((o->o_mode & O_ACCMODE) != O_RDONLY)
		perm |= PTE_W;
	ipc_send(envid, 0, (u_int)blk, perm);
	return;

out:
	ipc_send(envid, r, 0, 0);
#else
	// Your code here
	panic("serve_map not implemented");
#endif
}

void
serve_set_size(u_int envid, struct Fsreq_set_size *rq)
{
	if (debug) printf("serve_set_size %08x %08x %08x\n", envid, rq->req_fileid, rq->req_size);

#if SOL >= 5
	int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];
	int rc;

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->o_file == 0 || o->o_envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Resize the file as requested
	rc = file_set_size(o->o_file, rq->req_size);

	// Reply to the client
	out:
	ipc_send(envid, rc, 0, 0);
#else
	// Your code here
	panic("serve_set_size not implemented");
#endif
}

void
serve_close(u_int envid, struct Fsreq_close *rq)
{
	if (debug) printf("serve_close %08x %08x\n", envid, rq->req_fileid);

#if SOL >= 5
	int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];
	int rc;

	// Validate fileid passed by user
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->o_file == 0 || o->o_envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Close the file and clear the open table entry
	file_close(o->o_file);
	o->o_file = 0;
	o->o_envid = 0;
	rc = 0;

	// Reply to the client
out:
	ipc_send(envid, rc, 0, 0);
#else
	// Your code here
	panic("serve_close not implemented");
#endif
}

void
serve_remove(u_int envid, struct Fsreq_remove *rq)
{
	if (debug) printf("serve_map %08x %s\n", envid, rq->req_path);

#if SOL >= 5
	u_char path[MAXPATHLEN];
	int rc;

	// Copy in the path, making sure it's null-terminated
	bcopy(rq->req_path, path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;

	// Delete the specified file
	rc = file_remove(path);
	ipc_send(envid, rc, 0, 0);
#else
	// Your code here
	panic("serve_remove not implemented");
#endif
}

void
serve_dirty(u_int envid, struct Fsreq_dirty *rq)
{
	if (debug) printf("serve_dirty %08x %08x %08x\n", envid, rq->req_fileid, rq->req_offset);

#if SOL >= 5
	int rc;
	u_int fileid = rq->req_fileid;
	struct Open *o = &opentab[fileid];

	// Validate fileid passed by user
	
	if (fileid < 0 || fileid >= MAXOPEN ||
	    o->o_file == 0 || o->o_envid != envid) {
		rc = E_INVAL;
		goto out;
	}

	// Resize the file as requested
	rc = file_dirty(o->o_file, rq->req_offset);

	// Reply to the client
	out:
	ipc_send(envid, rc, 0, 0);
#else
	// Your code here
	panic("serve_dirty not implemented");
#endif
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

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			printf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
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
	assert(sizeof(struct File)==256);
	printf("FS is running\n");

	// Check that we are able to do I/O
	outw(0x8A00, 0x8A00);
	printf("FS can do I/O\n");

	fs_init();

	fs_test();

	serve();
}

#endif
