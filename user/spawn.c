#if LAB >= 5
#include "lib.h"
#include <inc/aout.h>

#define TMPSTACKTOP	(2*BY2PG)

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
	strings = (char*)TMPSTACKTOP - tot;
	args = (u_int*)(TMPSTACKTOP - ROUND(tot, 4) - 4*(argc+1));

	if ((r = sys_mem_alloc(0, TMPSTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
		return r;

#if SOL >= 5
	for (i=0; i<argc; i++) {
		args[i] = (u_int)strings + USTACKTOP - TMPSTACKTOP;
		strcpy(strings, argv[i]);
		strings += strlen(argv[i])+1;
	}
	args[i] = 0;
	assert(strings == (char*)TMPSTACKTOP);

	args[-1] = (u_int)args + USTACKTOP - TMPSTACKTOP;
	args[-2] = argc;

	*init_esp = (u_int)&args[-2] + USTACKTOP - TMPSTACKTOP;
#else /* not SOL >= 5 */
	// Replace this with your code to:
	//	- copy the argument strings into the stack page at 'strings'
	//	- initialize args[0..argc-1] to be pointers to these strings
	//	  that will be valid addresses for the child environment
	//	  (for whom this page will be at USTACKTOP-BY2PG!).
	//	- set args[argc] to 0 to null-terminate the args array.
	//	- push two more words onto the child's stack below 'args',
	//	  containing the argc and argv parameters to be passed
	//	  to the child's umain() function.
	//	- set *init_esp to the initial stack pointer for the child
	*init_esp = USTACKTOP;	// Change this!
#endif /* not SOL >= 5 */

	if ((r = sys_mem_map(0, TMPSTACKTOP-BY2PG, child, USTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
		goto error;
	if ((r = sys_mem_unmap(0, TMPSTACKTOP-BY2PG)) < 0)
		goto error;

	return 0;

error:
	sys_mem_unmap(0, TMPSTACKTOP-BY2PG);
	return r;
}

// Spawn a child process from a program image loaded from the file system.
// prog: the pathname of the program to run.
// argv: pointer to null-terminated array of pointers to strings,
// 	 which will be passed to the child as its command-line arguments.
// Returns child envid on success, < 0 on failure.
int
spawn(char *prog, char **argv)
{
	int child, fd, i, r;
	u_int text, data, bss, init_esp;
	void *blk;
	struct Aout aout;
	struct Trapframe tf;

	if ((r = open(prog, O_RDONLY)) < 0)
		return r;
	fd = r;

	// Read a.out header
	if (read(fd, &aout, sizeof(aout)) != sizeof(aout)
			|| aout.a_magic != AOUT_MAGIC) {
		close(fd);
printf("aout magic %08x want %08x\n", aout.a_magic, AOUT_MAGIC);
		return -E_NOT_EXEC;
	}

	// Create new child environment
	if ((r = sys_env_alloc()) < 0)
		return r;
	child = r;

	// Set up stack.
	if ((r = init_stack(child, argv, &init_esp)) < 0)
		return r;

	// Set up program text, data, bss
	text = UTEXT;
	for (i=0; i<aout.a_text; i+=BY2PG) {
		if ((r = read_map(fd, i, &blk)) < 0)
			goto error;
		if ((r = sys_mem_map(0, (u_int)blk, child, text+i, PTE_P|PTE_U)) < 0)
			panic("spawn: sys_mem_map text: %e", r);
	}

	data = text+i;
	for (i=0; i<aout.a_data; i+=BY2PG) {
		if ((r = read_map(fd, aout.a_text+i, &blk)) < 0)
			goto error;
		if ((r = sys_mem_map(0, (u_int)blk, child, data+i, PTE_P|PTE_U|PTE_W)) < 0)
			panic("spawn: sys_mem_map data: %e", r);
	}

	bss = data+i;
	for (i=0; i<aout.a_bss; i+=BY2PG) {
		if ((r = sys_mem_alloc(child, bss+i, PTE_P|PTE_U|PTE_W)) < 0)
			goto error;
	}

	// Set up trap frame.
	tf = envs[ENVX(child)].env_tf;
	tf.tf_eip = aout.a_entry;
	tf.tf_esp = init_esp;

	if ((r = sys_set_trapframe(child, &tf)) < 0)
		panic("sys_set_tf: %e", r);

	if ((r = sys_set_env_status(child, ENV_RUNNABLE)) < 0)
		panic("sys_set_env_status: %e", r);

	close(fd);
	return child;

error:
	sys_env_destroy(child);
	close(fd);
	return r;
}

// Spawn, taking command-line arguments array directly on the stack.
int
spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}

#endif
