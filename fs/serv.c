#if LAB >= 5
/*
 * File system server main loop -
 * serves IPC requests from other environments.
 */

#include <inc/x86.h>
#include <inc/string.h>

#include "fs.h"


#define debug 0

struct Open {
	struct File *o_file;	// mapped descriptor for open file
	u_int o_fileid;		// file id
	int o_mode;		// open mode
	struct Filefd *o_ff;	// va of filefd page
};

// Max number of open files in the file system at once
#define MAXOPEN	1024
#define FILEVA 0xD0000000

// initialize to force into data section
struct Open opentab[MAXOPEN] = { { 0, 0, 1 } };

// Virtual address at which to receive page mappings containing client requests.
#define REQVA	0x0ffff000

void
serve_init(void)
{
	int i;
	u_int va;

	va = FILEVA;
	for (i=0; i<MAXOPEN; i++) {
		opentab[i].o_fileid = i;
		opentab[i].o_ff = (struct Filefd*)va;
		va += BY2PG;
	}
}

// Allocate an open file.
int
open_alloc(struct Open **o)
{
	int i, r;

	// Find an available open-file table entry
	for (i = 0; i < MAXOPEN; i++) {
		switch (pageref(opentab[i].o_ff)) {
		case 0:
			if ((r = sys_mem_alloc(0, (u_int)opentab[i].o_ff, PTE_P|PTE_U|PTE_W)) < 0)
				return r;
			/* fall through */
		case 1:
			opentab[i].o_fileid += MAXOPEN;
			*o = &opentab[i];
			memset((void*)opentab[i].o_ff, 0, BY2PG);
			return (*o)->o_fileid;
		}
	}
	return -E_MAX_OPEN;
}

// Look up an open file for envid.
int
open_lookup(u_int envid, u_int fileid, struct Open **po)
{
	struct Open *o;

	o = &opentab[fileid%MAXOPEN];
	if (pageref(o->o_ff) == 1 || o->o_fileid != fileid)
		return -E_INVAL;
	*po = o;
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
	struct Filefd *ff;
	int fileid;
	int r;
	struct Open *o;

	// Copy in the path, making sure it's null-terminated
	memcpy(path, rq->req_path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;

	// Find a file id.
	if ((r = open_alloc(&o)) < 0) {
		if (debug) printf("open_alloc failed: %e", r);
		goto out;
	}
	fileid = r;

	// Open the file.
	if ((r = file_open(path, &f)) < 0) {
		if (debug) printf("file_open failed: %e", r);
		goto out;
	}

	// Save the file pointer.
	o->o_file = f;

	// Fill out the Filefd structure
	ff = (struct Filefd*)o->o_ff;
	ff->f_file = *f;
	ff->f_fileid = o->o_fileid;
	o->o_mode = rq->req_omode;
	ff->f_fd.fd_omode = o->o_mode;
	ff->f_fd.fd_dev_id = devfile.dev_id;

	if (debug) printf("sending success, page %08x\n", (u_int)o->o_ff);
#if SOL >= 6
	ipc_send(envid, 0, (u_int)o->o_ff, PTE_P|PTE_U|PTE_W|PTE_LIBRARY);
#else
	ipc_send(envid, 0, (u_int)o->o_ff, PTE_P|PTE_U|PTE_W);
#endif
	return;
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

#if SOL >= 6
	perm = PTE_U|PTE_P|PTE_LIBRARY;
#else
	perm = PTE_U|PTE_P;
#endif
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
	struct Open *o;
	int r;

	if ((r = open_lookup(envid, rq->req_fileid, &o)) < 0)
		goto out;
	r = file_set_size(o->o_file, rq->req_size);
out:
	ipc_send(envid, r, 0, 0);
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
	struct Open *o;
	int r;

	if ((r = open_lookup(envid, rq->req_fileid, &o)) < 0)
		goto out;
	file_close(o->o_file);
	if (pageref(o->o_ff) == 1)
		o->o_file = 0;
	r = 0;

out:
	ipc_send(envid, r, 0, 0);
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
	memcpy(path, rq->req_path, MAXPATHLEN);
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
	struct Open *o;
	int r;

	if ((r = open_lookup(envid, rq->req_fileid, &o)) < 0)
		goto out;
	r = file_dirty(o->o_file, rq->req_offset);

out:
	ipc_send(envid, r, 0, 0);
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

	serve_init();
	fs_init();
	fs_test();

	serve();
}

#endif
