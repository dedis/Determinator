#if LAB >= 4

#include <inc/assert.h>
#include <inc/stdarg.h>
#include <inc/syscall.h>
#include <inc/dirent.h>

extern void start(void);
extern void exec_start(intptr_t esp);

void exec_readelf(const char *path);
void exec_copyargs(char *const argv[]);

int
execl(const char *path, const char *arg0, ...)
{
	return execv(path, &arg0);
}

int
execv(const char *path, char *const argv[])
{
	// We'll build the new program in child 0,
	// which never represents a forked child since 0 is an invalid pid.
	// First clear out the new program's entire address space.
	sys_put(SYS_ZERO, 0, NULL, NULL, (void*)VM_USERLO, VM_USERHI-VM_USERLO);

	// Load the ELF executable into child 0.
	if (exec_readelf(path) < 0)
		return -1;

	// Setup the child's stack with the argument array.
	intptr_t esp = exec_copyargs(argv);

	XXX ...
}

int
exec_readelf(const char *path)
{
	// We'll load the ELF image into a scratch area in our address space.
	sys_get(SYS_ZERO, 0, NULL, NULL, (void*)VM_SCRATCHLO, PTSIZE);

	// Open the ELF image to load.
	filedesc *fd = filedesc_open(NULL, path, O_RDONLY, 0);
	if (fd == NULL)
		return -1;
	void *imgdata = FILEDATA(fd->ino);
	size_t imgsize = files->fi[fd->ino].size;

	// Make sure it looks like an ELF image.
	elfhdr *eh = imgdata;
	if (imgsize < sizeof(*eh) || eh->e_magic != ELF_MAGIC)
		goto badelf;

	// Load each program segment into the scratch area
	proghdr *ph = imgdata + eh->e_phoff;
	proghdr *eph = ph + eh->e_phnum;
	if (imgsize < (void*)eph - imgdata)
		goto badelf;
	for (; ph < eph; ph++) {
		if (ph->p_type != ELF_PROG_LOAD)
			continue;

		// The executable should fit in the first 4MB of user space.
		intptr_t valo = ph->p_va;
		intptr_t vahi = valo + ph->p_memsz;
		if (valo < VM_USERLO || valo > VM_USERLO+PTSIZE ||
				vahi < valo || vahi > VM_USERLO+PTSIZE)
			goto badelf;

		// Map all pages the segment touches in our scratch region.
		// They've already been zeroed by the SYS_ZERO above.
		intptr_t scratchofs = VM_SCRATCHLO - VM_USERLO;
		intptr_t pagelo = ROUNDDOWN(valo, PAGESIZE);
		intptr_t pagehi = ROUNDUP(vahi, PAGESIZE);
		sys_get(SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL, NULL,
			(void*)pagelo + scratchofs, pagehi - pagelo);

		// Initialize the file-loaded part of the ELF image.
		// (We could use copy-on-write if SYS_COPY
		// supports copying at arbitrary page boundaries.)
		intptr_t filelo = ph->p_offset;
		intptr_t filehi = filelo + ph->p_filesz;
		if (filelo < 0 || filelo > imgsize
				|| filehi < filelo || filehi > imgsize)
			goto badelf;
		memcpy((void*)valo + scratchofs, filelo, filehi - filelo);

		// Finally, remove write permissions on read-only segments.
		if (!(ph->p_flags & ELF_PROG_FLAG_WRITE))
			sys_get(SYS_PERM | SYS_READ, 0, NULL, NULL,
				(void*)pagelo + scratchofs, pagehi - pagelo);
	}

	// Copy the ELF image into its correct position in child 0.
	sys_put(SYS_COPY, 0, NULL, (void*)VM_SCRATCHLO,
		(void*)VM_USERLO, PTSIZE);

	// The new program should have the same entrypoint as we do!
	if (eh->e_entry != (intptr_t)start)
		goto badelf;

	filedesc_close(fd);	// Done with the ELF file
	return 0;

badelf:
	warn("execv: file %s is not an ELF image", path);
err:
	filedesc_close(fd);
	return -1;
}

intptr_t
exec_copyargs(char *const argv[])
{
	// Give the process a nice big 4MB, zero-filled stack.
	sys_get(SYS_ZERO | SYS_PERM | SYS_READ | SYS_WRITE, 0, NULL,
		NULL, (void*)VM_SCRATCHLO, PTSIZE);

	// How many arguments?
	int argc;
	for (int argc = 0; argv[argc] != NULL; argc++)
		;

	// Make room for the argv array
	intptr_t esp = VM_STACKHI;
	intptr_t scratchofs = VM_SCRATCHLO - (VM_STACKHI-PTSIZE);
	esp -= (argc+1) * sizeof(intptr_t);
	intptr_t dargv = esp;

	// Copy the argument strings
	int i;
	for (i = 0; i < argc; i++) {
		int len = strlen(argv[i]);
		esp -= len+1;
		strcpy((void*)esp + scratchofs, argv[i]);
		((intptr_t*)(dargv + scratchofs))[i] = esp;
	}
	esp &= 3;	// keep esp word-aligned

	// Push the arguments to main()
	esp -= 4;	*(intptr_t*)(esp + scratchofs) = dargv;
	esp -= 4;	*(intptr_t*)(esp + scratchofs) = argc;

	// Copy the stack into its correct position in child 0.
	sys_put(SYS_COPY, 0, NULL, (void*)VM_SCRATCHLO,
		(void*)VM_STACKHI-PTSIZE, PTSIZE);

	return esp;
}

#endif /* LAB >= 4 */
