#if LAB >= 5

#include <inc/lib.h>
#include <inc/elf.h>

#define TMPPAGE		(BY2PG)
#define TMPPAGETOP	(TMPPAGE+BY2PG)

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
	if (ROUND(tot, 4)+4*(argc+3) > BY2PG)
		return -E_NO_MEM;

	// Determine where to place the strings and the args array
	strings = (char*)TMPPAGETOP - tot;
	args = (u_int*)(TMPPAGETOP - ROUND(tot, 4) - 4*(argc+1));

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

	if ((r = sys_mem_map(0, TMPPAGE, child, USTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
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
		for (j=0; j<PTE2PT; j++) {
			pn = i*PTE2PT+j;
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

	printf("map_segment %x+%x\n", va, memsz);

	if ((i = (va&(BY2PG-1))) != 0) {
		va -= i;
		memsz += i;
		filesz += i;
		fileoffset -= i;
	}

	for (i = 0; i < memsz; i+=BY2PG) {
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
				if ((r = read(fd, (void*)TMPPAGE, MIN(BY2PG, filesz-i))) < 0)
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

	// Read a.out header
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

	if ((r = sys_set_env_status(child, ENV_RUNNABLE)) < 0)
		panic("sys_set_env_status: %e", r);

	return child;

error:
	sys_env_destroy(child);
	close(fd);
	return r;
#else /* not SOL >= 5 */
	// Insert your code, following approximately this procedure:
	//
	//	- Open the program file and read the a.out header (see inc/aout.h).
	//
	//	- Use sys_env_alloc() to create a new environment.
	//
	//	- Call the init_stack() function above to set up
	//	  the initial stack page for the child environment.
	//
	//	- Map the program's text segment, from file offset 0
	//	  up to (but not including) file offset 0x20+aout.a_text, starting at
	//	  virtual address UTEXT in the child (0x20 is the size of the a.out header).
	//	  Use read_map() and map the pages it returns directly
	//	  into the child so that multiple instances of the
	//	  same program will share the same copy of the program text.
	//	  Be sure to map the program text read-only in the child.
	//	  Read_map is like read but returns a pointer to the data
	//	  in *blk rather than copying the data into another buffer.
	//
	//	  The text segment will end on a page boundary.
	//	  That is, (0x20+aout.a_text) % BY2PG == 0.
	//
	//	- Set up the child process's data segment.  For each page,
	//	  allocate a page in the parent temporarily at TMPPAGE,
	//	  read() the appropriate block of the file into that page,
	//	  and then insert that page mapping into the child.
	//	  Look at init_stack() for inspiration.
	//	  Be sure you understand why you can't use read_map() here.
	//
	//	  The data segment starts where the text segment left off,
	//	  so it starts on a page boundary.  The data size will always be a multiple
	//	  of the page size, so it will end on a page boundary too.
	//	  The data comes from the aout.a_data bytes in the file starting
	//	  at offset (0x20+aout.a_text).
	//
	//	- Set up the child process's bss segment.
	//	  All you need to do here is sys_mem_alloc() the pages
	//	  directly into the child's address space, because
	//	  sys_mem_alloc() automatically zeroes the pages it allocates.
	//
	//	  The bss will start page aligned (since it picks up where the
	//	  data segment left off), but it's length may not be a multiple
	//	  of the page size, so it may not end on a page boundary.
	//	  Be sure to map the last page.  (It's okay to map the whole last page
	//	  even though the program will only need part of it.)
	//
	//	  The bss is not read from the binary file.  It is simply 
	//	  allocated as zeroed memory.  There are bits in the file at
	//	  offset 0x20+aout.a_text+aout.a_data, but they are not the
	//	  bss.  They are the symbol table, which is only for debuggers.
	//	  Do not use them.
	//
	//	  The exact location of the bss is a bit confusing, because
	//	  the linker lies to the loader about where it is.  
	//	  For example, in the copy of user/init that we have (yours
	//	  will depend on the size of your implementation of open and close),
	//	  i386-osclass-aout-nm claims that the bss starts at 0x8067c0
	//	  and ends at 0x807f40 (file offsets 0x67c0 to 0x7f40).
	//	  However, since this is not page aligned,
	//	  it lies to the loader, inserting some extra zeros at the end
	//	  of the data section to page-align the end, and then claims
	//	  that the data (which starts at 0x2000) is 0x5000 long, ending
	//	  at 0x7000, and that the bss is 0xf40 long, making it run from
	//	  0x7000 to 0x7f40.  This has the same effect as far as the
	//	  loading of the program.  Offsets 0x8067c0 to 0x807f40 
	//	  end up being filled with zeros, but they come from different
	//	  places -- the ones in the 0x806 page come from the binary file
	//	  as part of the data segment, but the ones in the 0x807 page
	//	  are just fresh zeroed pages not read from anywhere.
	//
	//	  If you are confused by the last paragraph, don't worry too much.
	//	  Just remember that the symbol table, should you choose to look at it,
	//	  is not likely to match what's in the a.out header.  Use the a.out
	//	  header alone.
	// 
	//	- Use the new sys_set_trapframe() call to set up the
	//	  correct initial eip and esp register values in the child.
	//	  You can use envs[ENVX(child)].env_tf as a template trapframe
	//	  in order to get the initial segment registers and such.
	//
	//	- Start the child process running with sys_set_env_status().
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
