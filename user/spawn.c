#if LAB >= 5
#include "lib.h"
#include <inc/aout.h>

#define TMPSTACKTOP	(2*BY2PG)

int
spawn(char *prog, char **argv)
{
	int argc, child, fd, i, r, tot;
	char *strings;
	u_int *args, text, data, bss;
	void *blk;
	struct Aout aout;
	struct Trapframe tf;

	if ((r = open(prog, O_RDONLY)) < 0)
		return r;
	fd = r;

	if (read(fd, &aout, sizeof(aout)) != sizeof(aout)
			|| aout.a_magic != AOUT_MAGIC) {
		close(fd);
printf("aout magic %08x want %08x\n", aout.a_magic, AOUT_MAGIC);
		return -E_NOT_EXEC;
	}

	if ((r = sys_env_alloc()) < 0)
		return r;
	child = r;

	// Set up stack.
	tot = 0;
	for (argc=0; argv[argc]; argc++)
		tot += strlen(argv[argc])+1;
	
	if (ROUND(tot, 4)+4*(argc+3) > BY2PG)
		return -E_NO_MEM;

	if ((r = sys_mem_alloc(0, TMPSTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
		goto error;

	strings = (char*)TMPSTACKTOP - tot;
	args = (u_int*)(TMPSTACKTOP - ROUND(tot, 4) - 4*(argc+1));

	for (i=0; i<argc; i++) {
		args[i] = (u_int)strings + USTACKTOP - TMPSTACKTOP;
		strcpy(strings, argv[i]);
		strings += strlen(argv[i])+1;
	}
	args[i] = 0;
	assert(strings == (char*)TMPSTACKTOP);

	args[-1] = (u_int)args + USTACKTOP - TMPSTACKTOP;
	args[-2] = argc;

	if ((r = sys_mem_map(0, TMPSTACKTOP-BY2PG, child, USTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
		panic("spawn: sys_mem_map stack: %e", r);
	if ((r = sys_mem_unmap(0, TMPSTACKTOP-BY2PG)) < 0)
		panic("spawn: sys_mem_unmap tmpstack: %e", r);

	// Set up text, data, bss
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
	tf.tf_esp = (u_int)&args[-2] + USTACKTOP - TMPSTACKTOP;

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

int
spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}

#endif
