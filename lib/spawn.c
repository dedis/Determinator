#if LAB >= 5

#include <inc/lib.h>
#include <inc/elf.h>

#define TMPPAGE		(PGSIZE)
#define TMPPAGETOP	(TMPPAGE+PGSIZE)

// Set up the initial stack page for the new child process with envid 'child',
// using the arguments array pointed to by 'argv',
// which is a null-terminated array of pointers to null-terminated strings.
//
// On success, returns 0 and sets *init_esp
// to the initial stack pointer with which the child should start.
// Returns < 0 on failure.
//
static int
init_stack(u_int child, char **argv, u_int *init_esp)
{
	int argc, i, r, tot;
	char *strings;
	u_int *args;

	// Count the number of arguments (argc)
	// and the total amount of space needed for strings (tot)
	tot = 0;
	for (argc=0; argv[argc]; argc++)
		tot += strlen(argv[argc])+1;

	// Make sure everything will fit in the initial stack page
	if (ROUNDUP(tot, 4)+4*(argc+3) > PGSIZE)
		return -E_NO_MEM;

	// Determine where to place the strings and the args array
	strings = (char*)TMPPAGETOP - tot;
	args = (u_int*)(TMPPAGETOP - ROUNDUP(tot, 4) - 4*(argc+1));

	if ((r = sys_mem_alloc(0, TMPPAGE, PTE_P|PTE_U|PTE_W)) < 0)
		return r;

#if SOL >= 5
	for (i=0; i<argc; i++) {
		args[i] = (u_int)strings + USTACKTOP - TMPPAGETOP;
		strcpy(strings, argv[i]);
		strings += strlen(argv[i])+1;
	}
	args[i] = 0;
	assert(strings == (char*)TMPPAGETOP);

	args[-1] = (u_int)args + USTACKTOP - TMPPAGETOP;
	args[-2] = argc;

	*init_esp = (u_int)&args[-2] + USTACKTOP - TMPPAGETOP;
#else // not SOL >= 5
	// Replace this with your code to:
	//
	//	- copy the argument strings into the stack page at 'strings'
	//
	//	- initialize args[0..argc-1] to be pointers to these strings
	//	  that will be valid addresses for the child environment
	//	  (for whom this page will be at USTACKTOP-BY2PG!).
	//
	//	- set args[argc] to 0 to null-terminate the args array.
	//
	//	- push two more words onto the child's stack below 'args',
	//	  containing the argc and argv parameters to be passed
	//	  to the child's umain() function.
	//
	//	- set *init_esp to the initial stack pointer for the child
	//
	*init_esp = USTACKTOP;	// Change this!
#endif // not SOL >= 5

	if ((r = sys_mem_map(0, TMPPAGE, child, USTACKTOP-PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
		goto error;
	if ((r = sys_mem_unmap(0, TMPPAGE)) < 0)
		goto error;

	return 0;

error:
	sys_mem_unmap(0, TMPPAGE);
	return r;
}

#if SOL >= 6
// Copy the mappings for library pages into the child address space.
static void
copy_library(u_int child)
{
	u_int i, j, pn, va;
	int r;

	for (i=0; i<PDX(UTOP); i++) {
		if ((vpd[i]&PTE_P) == 0)
			continue;
		for (j=0; j<NPTENTRIES; j++) {
			pn = i*NPTENTRIES+j;
			if ((vpt[pn]&(PTE_P|PTE_LIBRARY)) == (PTE_P|PTE_LIBRARY)) {
				va = pn<<PGSHIFT;
				if ((r = sys_mem_map(0, va, child, va, vpt[pn]&PTE_USER)) < 0)
					panic("sys_mem_map: %e", r);
			}
		}
	}
}
#endif

int
map_segment(int child, u_int va, u_int memsz, 
	int fd, u_int filesz, u_int fileoffset, u_int perm)
{
#if SOL >= 5
	int i, r;
	void *blk;

	//printf("map_segment %x+%x\n", va, memsz);

	if ((i = (va&(PGSIZE-1))) != 0) {
		va -= i;
		memsz += i;
		filesz += i;
		fileoffset -= i;
	}

	for (i = 0; i < memsz; i+=PGSIZE) {
		if (i >= filesz) {
			// allocate a blank page
			if ((r = sys_mem_alloc(child, va+i, perm)) < 0)
				return r;
		} else {
			// from file
			if (perm&PTE_W) {
				// must make a copy so it can be writable
				if ((r = sys_mem_alloc(0, TMPPAGE, PTE_P|PTE_U|PTE_W)) < 0)
					return r;
				if ((r = seek(fd, fileoffset+i)) < 0)
					return r;
				if ((r = read(fd, (void*)TMPPAGE, MIN(PGSIZE, filesz-i))) < 0)
					return r;
				if ((r = sys_mem_map(0, TMPPAGE, child, va+i, perm)) < 0)
					panic("spawn: sys_mem_map data: %e", r);
				sys_mem_unmap(0, TMPPAGE);
			} else {
				// can map buffer cache read only
				if ((r = read_map(fd, fileoffset+i, &blk)) < 0)
					return r;
				if ((r = sys_mem_map(0, (u_int)blk, child, va+i, perm)) < 0)
					panic("spawn: sys_mem_map text: %e", r);
			}
		}
	}
	return 0;
#endif
}


// Spawn a child process from a program image loaded from the file system.
// prog: the pathname of the program to run.
// argv: pointer to null-terminated array of pointers to strings,
// 	 which will be passed to the child as its command-line arguments.
// Returns child envid on success, < 0 on failure.
int
spawn(char *prog, char **argv)
{
#if SOL >= 5
	int child, fd, i, r;
	u_int init_esp;
	struct Trapframe tf;
	u_char hdr[512];
	struct Elf *elf;
	struct Proghdr *ph;
	int perm;

	if ((r = open(prog, O_RDONLY)) < 0)
		return r;
	fd = r;

	// Read elf header
	elf = (struct Elf*)hdr;
	if (read(fd, hdr, sizeof(hdr)) != sizeof(hdr)
			|| elf->e_magic != ELF_MAGIC) {
		close(fd);
		printf("elf magic %08x want %08x\n", elf->e_magic, ELF_MAGIC);
		return -E_NOT_EXEC;
	}

	// Create new child environment
	if ((r = sys_env_alloc()) < 0)
		return r;
	child = r;

	// Set up stack.
	if ((r = init_stack(child, argv, &init_esp)) < 0)
		return r;

	// Set up program segments as defined in ELF header.
	ph = (struct Proghdr*)(hdr+elf->e_phoff);
	for (i=0; i<elf->e_phnum; i++, ph++) {
		if (ph->p_type != ELF_PROG_LOAD)
			continue;
		perm = PTE_P|PTE_U;
		if(ph->p_flags & ELF_PROG_FLAG_WRITE)
			perm |= PTE_W;
		if ((r = map_segment(child, ph->p_va, ph->p_memsz, 
				fd, ph->p_filesz, ph->p_offset, perm)) < 0)
			goto error;
	}
	close(fd);
	fd = -1;

#if SOL >= 6
	// Copy shared library state.
	copy_library(child);

#endif
	// Set up trap frame.
	tf = envs[ENVX(child)].env_tf;
	tf.tf_eip = elf->e_entry;
	tf.tf_esp = init_esp;

	if ((r = sys_set_trapframe(child, &tf)) < 0)
		panic("sys_set_tf: %e", r);

	if ((r = sys_set_status(child, ENV_RUNNABLE)) < 0)
		panic("sys_set_status: %e", r);

	return child;

error:
	sys_env_destroy(child);
	close(fd);
	return r;
#else /* not SOL >= 5 */
	// Insert your code, following approximately this procedure:
	//
	//   - Open the program file and read the elf header (see inc/elf.h).
	//
	//   - Use sys_env_alloc() to create a new environment.
	//
	//   - Call the init_stack() function above to set up
	//     the initial stack page for the child environment.
	//
	//   - Map all of the program's segments that are of p_type
	//     ELF_PROG_LOAD into the new environment's address space.
	//     Use the p_flags field in the Proghdr for each segment
	//     to determine how to map the segment:
	//
	//	* If the ELF flags do not include ELF_PROG_FLAG_WRITE,
	//	  then the segment contains text and read-only data.
	//	  Use read_map() to read the contents of this segment,
	//	  and map the pages it returns directly into the child
	//        so that multiple instances of the same program
	//	  will share the same copy of the program text.
	//        Be sure to map the program text read-only in the child.
	//        Read_map is like read but returns a pointer to the data in
	//        *blk rather than copying the data into another buffer.
	//
	//	* If the ELF segment flags DO include ELF_PROG_FLAG_WRITE,
	//	  then the segment contains read/write data and bss.
	//	  As with load_icode() in Lab 3, such an ELF segment
	//	  occupies p_memsz bytes in memory, but only the FIRST
	//	  p_filesz bytes of the segment are actually loaded
	//	  from the executable file - you must clear the rest to zero.
	//        For each page to be mapped for a read/write segment,
	//        allocate a page in the parent temporarily at TMPPAGE,
	//        read() the appropriate portion of the file into that page
	//	  and/or use memset() to zero non-loaded portions,
	//        and then insert the page mapping into the child.
	//        Look at init_stack() for inspiration.
	//        Be sure you understand why you can't use read_map() here.
	//
	//     Note: None of the segment addresses or lengths above
	//     are guaranteed to be page-aligned, so you must deal with
	//     these non-page-aligned values appropriately.
	//     The ELF linker does, however, guarantee that no two segments
	//     will overlap on the same page.
	//
	//   - Use the new sys_set_trapframe() call to set up the
	//     correct initial eip and esp register values in the child.
	//     You can use envs[ENVX(child)].env_tf as a template trapframe
	//     in order to get the initial segment registers and such.
	//
	//   - Start the child process running with sys_set_status().
	//
	panic("spawn unimplemented!");
#endif /* not SOL >= 5 */
}

// Spawn, taking command-line arguments array directly on the stack.
int
spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}

#endif
