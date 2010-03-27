#if LAB >= 4

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/unistd.h>
#include <inc/dirent.h>
#include <inc/syscall.h>
#include <inc/errno.h>
#include <inc/file.h>
#include <inc/stat.h>
#include <inc/elf.h>


int initfilecheck_count;

void
initfilecheck()
{
	// Manually go through the inodes looking for populated files
	int ino, count = 0, shino = 0, lsino = 0;
	for (ino = 1; ino < FILE_INODES; ino++) {
		if (files->fi[ino].de.d_name[0] == 0)
			break;		// first unused entry

		cprintf("initfilecheck: found file '%s' mode 0x%x size %d\n",
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
	initfilecheck_count = count;	// save for readdircheck

	cprintf("initfilecheck passed\n");
}

void
readwritecheck()
{
	static char buf[2048];	// a buffer to use for reading/writing data
	static char buf2[2048];	// a buffer to use for reading/writing data
	static const char zeros[1024];	// a buffer of all zeros

	// Get the initial file size etc.
	struct stat st;
	int rc = stat("ls", &st); assert(rc >= 0);
	assert(S_ISREG(st.st_mode));
	assert(st.st_size > 0);

	// Read the first 1KB of one of the initial files,
	// make sure it looks reasonable.
	int fd = open("ls", O_RDONLY); assert(fd > 0);
	ssize_t act = read(fd, buf, 2048);
	assert(act == 2048);
	elfhdr *eh = (elfhdr*) buf;
	assert(eh->e_magic == ELF_MAGIC); // should be an ELF file
	close(fd);

	// Overwrite the first 1K with zeros.
	fd = open("ls", O_WRONLY); assert(fd > 0);
	act = write(fd, zeros, 1024);
	assert(act == 1024);
	close(fd);

	// Re-read the first 2KB, make sure the right thing happened
	fd = open("ls", O_RDONLY); assert(fd > 0);
	act = read(fd, buf2, 2048);
	assert(act == 2048);
	assert(memcmp(buf2, zeros, 1024) == 0); // first 1K should be all zero
	assert(memcmp(buf2+1024, buf+1024, 1024) == 0); // rest is untouched
	close(fd);

	// Restore the first 1K of the file to its initial condition
	fd = open("ls", O_WRONLY); assert(fd > 0);
	act = write(fd, buf, 1024);
	assert(act == 1024);
	close(fd);

	// Make sure the restoration was successful
	fd = open("ls", O_RDONLY); assert(fd > 0);
	act = read(fd, buf2, 2048);
	assert(act == 2048);
	assert(memcmp(buf, buf2, 2048) == 0);
	close(fd);

	// File size and such shouldn't have changed
	struct stat st2;
	rc = stat("ls", &st2); assert(rc >= 0);
	assert(memcmp(&st, &st2, sizeof(st)) == 0);

	cprintf("readwritecheck passed\n");
}

void
seekcheck()
{
	static char buf[2048];	// a buffer to use for reading/writing data
	static char buf2[2048];	// a buffer to use for reading/writing data
	static char buf3[2048];	// a buffer to use for reading/writing data
	static const char zeros[1024];	// a buffer of all zeros
	int i, rc;
	ssize_t act;

	int fd = open("sh", O_RDWR); assert(fd > 0);

	// Get the file's original size etc.
	struct stat st;
	rc = fstat(fd, &st); assert(rc >= 0);
	assert(S_ISREG(st.st_mode));
	assert(st.st_size > 65536);

	// We should be at the beginning of the file
	assert(lseek(fd, 0, SEEK_CUR) == 0);
	assert(lseek(fd, 0, SEEK_CUR) == 0); // ...and should have stayed there

	// Do some seeking and check the seek pointer arithmetic.
	// Note that it's not an error to seek past the end of file;
	// it's just that there's nothing to read there.
	rc = lseek(fd, 65536, SEEK_SET); assert(rc == 65536);
	rc = lseek(fd, 65536, SEEK_SET); assert(rc == 65536);
	rc = lseek(fd, 1024*1024, SEEK_CUR); assert(rc == 65536+1024*1024);
	rc = lseek(fd, -1024*1024, SEEK_CUR); assert(rc == 65536);
	rc = lseek(fd, 0, SEEK_END); assert(rc == st.st_size);
	rc = lseek(fd, -1024, SEEK_END); assert(rc == st.st_size-1024);
	rc = lseek(fd, 12345, SEEK_END); assert(rc == st.st_size+12345);

	// Read some blocks sequentially from the beginning of the file,
	// and compare against what we get if we directly seek to a block
	rc = lseek(fd, -st.st_size, SEEK_END); assert(rc == 0);
	act = read(fd, buf, 2048); assert(act == 2048);
	elfhdr *eh = (elfhdr*) buf;
	assert(eh->e_magic == ELF_MAGIC); // should be an ELF file
	for (i = 0; i < 32; i++) { // read next 32 2KB chunks
		act = read(fd, buf, 2048); assert(act == 2048);
	}
	// should leave file bytes 64KB thru 66KB in buf; verify...
	rc = lseek(fd, 0, SEEK_CUR); assert(rc == 66*1024);
	rc = lseek(fd, 65536, SEEK_SET); assert(rc == 65536);
	act = read(fd, buf2, 2048); assert(act == 2048);
	rc = lseek(fd, 0, SEEK_CUR); assert(rc == 66*1024);
	assert(memcmp(buf, buf2, 2048) == 0);

	// overwrite part of this area
	rc = lseek(fd, -1024-512, SEEK_CUR); assert(rc == 65536+512);
	act = write(fd, zeros, 1024); assert(act == 1024);
	rc = lseek(fd, 65536, SEEK_SET); assert(rc == 65536);
	act = read(fd, buf2, 2048); assert(act == 2048);
	assert(memcmp(buf2, buf, 512) == 0);
	assert(memcmp(buf2+512, zeros, 1024) == 0);
	assert(memcmp(buf2+1024+512, buf+1024+512, 512) == 0);

	// try reading past the end of the file
	rc = lseek(fd, -1024, SEEK_END); assert(rc == st.st_size - 1024);
	act = read(fd, buf2, 2048); assert(act == 1024); // that's all there is

	// file size shouldn't have changed so far
	struct stat st2;
	rc = fstat(fd, &st2); assert(rc >= 0);
	assert(memcmp(&st, &st2, sizeof(st)) == 0);

	// overwrite and extend the last part of the file
	rc = lseek(fd, -1024, SEEK_END); assert(rc == st.st_size - 1024);
	act = write(fd, zeros, 2048); assert(act == 2048);

	// The file should have grown by 1KB
	rc = fstat(fd, &st2); assert(rc >= 0);
	assert(st2.st_size == st.st_size + 1024);

	// try to read way beyond end-of-file
	rc = lseek(fd, 1234567, SEEK_END); assert(rc == st2.st_size + 1234567);
	memcpy(buf3, buf2, 2048);
	act = read(fd, buf3, 2048); assert(act == 0);
	assert(memcmp(buf3, buf2, 2048) == 0); // shouldn't have touched buf3

	// try to grow a file too big for PIOS's file system
	memcpy(buf3, FILEDATA(files->fd[fd].ino+1), 2048); // corruption check
	rc = lseek(fd, FILE_MAXSIZE, SEEK_SET); assert(rc == FILE_MAXSIZE);
	act = write(fd, buf, 2048); assert(act < 0); assert(errno == EFBIG);
	assert(memcmp(buf3, FILEDATA(files->fd[fd].ino+1), 2048) == 0);

	// The file should still be 1KB larger than its original size
	rc = fstat(fd, &st2); assert(rc >= 0);
	assert(st2.st_size == st.st_size + 1024);

	// Restore the parts of the file we mucked with
	rc = lseek(fd, 65536+512, SEEK_SET); assert(rc == 65536+512);
	act = write(fd, buf+512, 1024); assert(act == 1024);
	rc = lseek(fd, st.st_size-1024, SEEK_SET);
	assert(rc == st.st_size-1024);
	act = write(fd, buf2, 2048); assert(act == 2048);
	rc = ftruncate(fd, st.st_size); assert(rc == 0);

	// The file should now be back to its original size
	rc = fstat(fd, &st2); assert(rc >= 0);
	assert(memcmp(&st, &st2, sizeof(st)) == 0);

	// Check that the restorations happened properly
	rc = lseek(fd, 65536, SEEK_SET); assert(rc == 65536);
	act = read(fd, buf3, 2048); assert(act == 2048);
	assert(memcmp(buf, buf3, 2048) == 0);
	rc = lseek(fd, st.st_size-1024, SEEK_SET);
	assert(rc == st.st_size-1024);
	memset(buf3, 0, 2048);
	act = read(fd, buf3, 2048); assert(act == 1024);
	assert(memcmp(buf3, buf2, 1024) == 0);
	assert(memcmp(buf3+1024, zeros, 1024) == 0);

	cprintf("seekcheck passed\n");
}

void
readdircheck()
{
	// Do basically the same thing as initfilecheck(),
	// but this time using the "proper" Unix-like opendir/readdir API.
	DIR *d = opendir("/"); assert(d != NULL);
	struct dirent *de;
	int count = 0, shfound = 0, lsfound = 0;
	while ((de = readdir(d)) != NULL) {
		struct stat st;
		int rc = stat(de->d_name, &st); assert(rc == 0);

		cprintf("readdircheck: found file '%s' mode 0x%x size %d\n",
			de->d_name, st.st_mode, st.st_size);
		count++;

		// Make sure general properties are as we expect
		if (strcmp(de->d_name, "consin") == 0) {
			assert(st.st_ino == FILEINO_CONSIN);
			continue;
		}
		if (strcmp(de->d_name, "consout") == 0) {
			assert(st.st_ino == FILEINO_CONSOUT);
			continue;
		}
		if (strcmp(de->d_name, "/") == 0) {
			assert(st.st_ino == FILEINO_ROOTDIR);
			assert(st.st_mode == S_IFDIR);
			continue;
		}

		// everything else should be a regular file
		assert(st.st_ino >= FILEINO_GENERAL);
		assert(st.st_ino < FILE_INODES);
		assert(st.st_mode == S_IFREG);
		assert(st.st_size > 0);

		// Make sure a couple specific files we're expecting show up
		if (strcmp(de->d_name, "sh") == 0)
			shfound = 1;
		if (strcmp(de->d_name, "ls") == 0)
			lsfound = 1;
	}

	// Make sure we found a "reasonable" number of populated entries
	assert(count >= 5);
	assert(shfound != 0);
	assert(lsfound != 0);
	assert(count == initfilecheck_count);

	cprintf("readdircheck passed\n");
}

void
consoutcheck()
{
	// Write some text to our 'consout' special file in a few ways
	const char outstr[] = "conscheck: write() to STDOUT_FILENO\n";
	write(STDOUT_FILENO, outstr, strlen(outstr));
	fprintf(stdout, "conscheck: fprintf() to 'stdout'\n");
	FILE *f = fopen("consout", "a"); assert(f != NULL);
	fprintf(f, "conscheck: fprintf() to 'consout' file\n");
	fclose(f);

	cprintf("Buffered console output should NOT have appeared yet\n");
	sys_ret();	// Synchronize with the kernel, deliver console output
	cprintf("Buffered console output SHOULD have appeared now\n");

	// More of the same, just all on one line for easy checking...
	write(STDOUT_FILENO, "456", 3);
	cprintf("123");
	sys_ret();
	write(STDOUT_FILENO, "\n", 1);
	cprintf("789");
	sys_ret();

	cprintf("consoutcheck done\n");
}

void
consincheck()
{
	char *str = readline("Enter something: ");
	printf("You typed: %s\n", str);
	sys_ret();

	cprintf("consincheck done\n");
}

pid_t
spawn(const char *arg0, ...)
{
	pid_t pid = fork();
	if (pid == 0) {		// We're the child.
		execv(arg0, (char *const *)&arg0);
		panic("execl() failed: %s\n", strerror(errno));
	}
	assert(pid > 0);	// We're the parent.
	return pid;
}

void
waitcheckstatus(pid_t pid, int statexpect)
{
	// Wait for the child to finish executing, and collect its status.
	int status = 0xdeadbeef;
	waitpid(pid, &status, 0);
	assert(WIFEXITED(status));
	assert(WEXITSTATUS(status) == statexpect);
}
#define waitcheck(pid) waitcheckstatus(pid, 0)

void
execcheck()
{
	waitcheck(spawn("echo", "-c", "called", "by", "execcheck", NULL));

	cprintf("execcheck done\n");
}

pid_t forkwrite(const char *filename)
{
	pid_t pid = fork();
	if (pid == 0) {		// We're the child.
		FILE *f = fopen(filename, "w"); assert(f != NULL);
		fprintf(f, "forkwrite: %s\n", filename);
		fclose(f);
		exit(0);
	}
	assert(pid > 0);	// We're the parent.
	return pid;
}

void
reconcilecheck()
{
	// First fork off a child process that just writes one file,
	// and make sure it appears when we subsequently 'cat' it.
	waitcheck(forkwrite("reconcilefile0"));
	waitcheck(spawn("cat", "reconcilefile0", NULL));

	// Now try two concurrent, non-conflicting writes.
	pid_t p1 = forkwrite("reconcilefile1");
	pid_t p2 = forkwrite("reconcilefile2");
	waitcheck(p1);
	waitcheck(p2);
	waitcheck(spawn("cat", "reconcilefile1", NULL));
	waitcheck(spawn("cat", "reconcilefile2", NULL));

	// Now try two concurrent, conflicting writes.
	p1 = forkwrite("reconcilefileC");
	p2 = forkwrite("reconcilefileC");
	waitcheck(p1);
	waitcheck(p2);
	waitcheckstatus(spawn("cat", "reconcilefileC", NULL), 1); // fails!
	waitcheck(spawn("ls", "-l", NULL));

	cprintf("reconcilecheck: basic file reconciliation successful\n");

	// Reconcile append-only console output
	printf("reconcilecheck: running echo\n");
	pid_t pid = spawn("echo", "called", "by", "reconcilecheck", NULL);
	printf("reconcilecheck: echo running\n");
	waitcheck(pid);
	printf("reconcilecheck: echo finished\n");
	sys_ret();	// flush this output to the real console

	cprintf("reconcilecheck done\n");
}

int
main()
{
	cprintf("testfs: in main()\n");

	initfilecheck();
	readwritecheck();
	seekcheck();
	readdircheck();

	consoutcheck();
	consincheck();

	execcheck();

	reconcilecheck();

	cprintf("testfs: all tests completed; starting shell...\n");
	execl("sh", "sh", NULL);
	panic("execl failed: %s", strerror(errno));
}

#endif // LAB >= 4
