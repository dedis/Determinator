#if LAB >= 5
#include "lib.h"
#include <inc/fs.h>

#define debug 0

extern u_char fsipcbuf[BY2PG];		// page-aligned, declared in entry.S

static int
fsipc(u_int type, void *fsreq, u_int dstva, u_int *perm)
{
	u_int whom;

	if (debug) printf("[%08x] fsipc %d %08x\n", env->env_id, type, fsipcbuf);

	ipc_send(envs[1].env_id, type, (u_int)fsreq, PTE_P|PTE_W|PTE_U);
	return ipc_recv(&whom, dstva, perm);
}

int
fsipc_open(const char *path, u_int omode, u_int *fileid, u_int *size)
{
	int r;
	struct Fsreq_open *req;

	req = (struct Fsreq_open*)fsipcbuf;
	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;
	strcpy(req->req_path, path);
	req->req_omode = omode;

	if ((r = fsipc(FSREQ_OPEN, req, 0, 0)) < 0)
		return r;

	if (fileid)
		*fileid = req->req_fileid;
	if (size)
		*size = req->req_size;
	return 0;
}

int
fsipc_map(u_int fileid, u_int offset, u_int dstva)
{
	int r;
	u_int perm;
	struct Fsreq_map *req;

	req = (struct Fsreq_map*)fsipcbuf;
	req->req_fileid = fileid;
	req->req_offset = offset;
	if ((r=fsipc(FSREQ_MAP, req, dstva, &perm)) < 0)
		return r;
	if ((perm&~PTE_W) != (PTE_U|PTE_P))
		panic("fsipc_map: unexpected permissions %08x", perm);
	return 0;
}

int
fsipc_set_size(u_int fileid, u_int size)
{
	struct Fsreq_set_size *req;

	req = (struct Fsreq_set_size*)fsipcbuf;
	req->req_fileid = fileid;
	req->req_size = size;
	return fsipc(FSREQ_SET_SIZE, req, 0, 0);
}

int
fsipc_close(u_int fileid)
{
	struct Fsreq_close *req;

	req = (struct Fsreq_close*)fsipcbuf;
	req->req_fileid = fileid;
	return fsipc(FSREQ_CLOSE, req, 0, 0);
}

int
fsipc_dirty(u_int fileid, u_int offset)
{
	struct Fsreq_dirty *req;

	req = (struct Fsreq_dirty*)fsipcbuf;
	req->req_fileid = fileid;
	req->req_offset = offset;
	return fsipc(FSREQ_DIRTY, req, 0, 0);
}

int
fsipc_remove(const char *path)
{
	struct Fsreq_remove *req;

	req = (struct Fsreq_remove*)fsipcbuf;
	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;
	strcpy(req->req_path, path);
	return fsipc(FSREQ_REMOVE, req, 0, 0);
}

int
fsipc_sync(void)
{
	return fsipc(FSREQ_SYNC, fsipcbuf, 0, 0);
}

#endif
