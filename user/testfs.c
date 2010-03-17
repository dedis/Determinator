#if LAB >= 4

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/file.h>
#include <inc/stat.h>
#include <inc/elf.h>

void
initfilecheck()
{
	// Manually go through the inodes looking for populated files
	int ino, count = 0, shino = 0, lsino = 0;
	for (ino = 1; ino < FILE_INODES; ino++) {
		if (files->fi[ino].de.d_name[0] == 0)
			break;		// first unused entry

		cprintf("initfilecheck: file '%s' mode 0x%x size %d\n",
			files->fi[ino].de.d_name, files->fi[ino].mode,
			files->fi[ino].size);

		// Make sure general properties are as we expect
		assert(files->fi[ino].ver == 0);
		if (ino >= FILEINO_GENERAL) {
			// initfiles are all in the root directory
			assert(files->fi[ino].dino == FILEINO_ROOTDIR);
			// initfiles are all regular files
			assert(files->fi[ino].mode == S_IFREG);
			// and should all contain some file data
			assert(files->fi[ino].size > 0);
		}

		// Make sure a couple specific files we're expecting show up
		if (strcmp(files->fi[ino].de.d_name, "sh") == 0) {
			// contents should be an ELF executable!
			assert(*(int*)FILEDATA(ino) == ELF_MAGIC);
			shino = ino;
		}
		if (strcmp(files->fi[ino].de.d_name, "ls") == 0) {
			// contents should be an ELF executable!
			assert(*(int*)FILEDATA(ino) == ELF_MAGIC);
			lsino = ino;
		}

		count++;
	}
	for (; ino < FILE_INODES; ino++) {
		// all the rest of the inodes should be empty
		assert(files->fi[ino].dino == FILEINO_NULL);
		assert(files->fi[ino].de.d_name[0] == 0);
		assert(files->fi[ino].ver == 0);
		assert(files->fi[ino].mode == 0);
		assert(files->fi[ino].size == 0);
	}

	// Make sure we found a "reasonable" number of populated entries
	assert(count >= 5);
	assert(shino != 0);
	assert(lsino != 0);

	cprintf("initfilecheck passed\n");
}

int
main()
{
	cprintf("testfs: in main()\n");

	initfilecheck();

	cprintf("testfs: all tests completed successfully!\n");
	return 0;
}

#endif // LAB >= 4
