#include "lib.h"

int
strecmp(char *a, char *b)
{
	while (*b)
		if(*a++ != *b++)
			return 1;
	return 0;
}

char *msg = "This is the NEW message of the day!\n\n";

#define FVA 0xCCCCC000

void
umain(void)
{
	int r;
	u_int fileid;
	struct Filefd *ff;

	if ((r = fsipc_open("/not-found", O_RDONLY, FVA)) < 0 && r != -E_NOT_FOUND)
		panic("serve_open /not-found: %e", r);
	else if (r == 0)
		panic("serve_open /not-found succeeded!");

	if ((r = fsipc_open("/newmotd", O_RDONLY, FVA)) < 0)
		panic("serve_open /newmotd: %e", r);
	ff = (struct Filefd*)FVA;
	if (strlen(msg) != ff->f_file.f_size)
		panic("serve_open returned size %d wanted %d\n", ff->f_file.f_size, strlen(msg));
	printf("serve_open is good\n");

	if ((r = fsipc_map(ff->f_fileid, 0, BY2PG)) < 0)
		panic("serve_map: %e", r);
	if(strecmp((char*)BY2PG, msg) != 0)
		panic("serve_map returned wrong data");
	printf("serve_map is good\n");

	if ((r = fsipc_close(ff->f_fileid)) < 0)
		panic("serve_close: %e", r);
	printf("serve_close is good\n");
	fileid = ff->f_fileid;
	sys_mem_unmap(0, FVA);

	if ((r = fsipc_map(fileid, 0, BY2PG)) != -E_INVAL)
		panic("serve_map does not handle stale fileids correctly");
	printf("stale fileid is good\n");
}

