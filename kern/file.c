#if LAB >= 4
// Process management.
// See COPYRIGHT for copyright information.

#include <inc/x86.h>
#include <inc/fcntl.h>

#include <kern/proc.h>
#include <kern/file.h>


// Build a table of files to include in the initial file system.
#define INITFILE(name)	\
	extern char _binary_obj_user_##name##_start[]; \
	extern char _binary_obj_user_##name##_end[];
#include <obj/kern/initfiles.h>
#undef INITFILE

#define INITFILE(name)	\
	{ _binary_obj_user_##name##_start, _binary_obj_user_##name##_end },
char *initfiles[][2] = {
	#include <obj/kern/initfiles.h>
};
#undef INITFILE


void
file_init(proc *root)
{
	// Make sure the root process's page directory is active
	lcr3(root->pdir);

	// Enable read/write access on the file metadata area
	assert(pmap_setperm(root->pdir, FILESVA,
				ROUNDUP(sizeof(filestate), PAGESIZE),
				SYS_READ | SYS_WRITE));
	memset(files, 0, sizeof(*files));

	// Set up the standard I/O descriptors for console I/O
	files->fd[0].ino = FILEINO_CONSIN;
	files->fd[0].flags = FILEDESC_READ;
	files->fd[1].ino = FILEINO_CONSOUT;
	files->fd[1].flags = FILEDESC_WRITE | FILEDESC_APPEND;
	files->fd[2].ino = FILEINO_CONSOUT;
	files->fd[2].flags = FILEDESC_WRITE | FILEDESC_APPEND;

	files->fi[FILEINO_CONSIN].partial = 1;
	files->fi[FILEINO_CONSIN].nlink = 1;	// fd[0]
	files->fi[FILEINO_CONSOUT].nlink = 2;	// fd[1] and fd[2]

	// Set up the root directory.
	files->fi[FILEINO_ROOTDIR].nlink = 1;
}

#endif // LAB >= 4
