#if LAB >= 4
// More-or-less Unix-compatible fork and wait functions

#include <inc/file.h>
#include <inc/stat.h>
#include <inc/unistd.h>
#include <inc/sys/wait.h>
#include <inc/string.h>
#include <inc/syscall.h>
#include <inc/assert.h>
#include <inc/errno.h>
#include <inc/mmu.h>
#include <inc/vm.h>


#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

static bool reconcile(pid_t pid, filestate *cfiles);
static bool recinode(pid_t pid, filestate *src, int srci, filestate *dst,
			int *imap);
static bool recappends(pid_t pid, filestate *src, int srci,
			filestate *dst, int dsti, int *rlenp);

pid_t fork(void)
{
	// Find a free child process slot.
	// We just use child process slot numbers as Unix PIDs,
	// even though child slots are process-local in PIOS
	// whereas PIDs are global in Unix.
	// This means that commands like 'ps' and 'kill'
	// have to be shell-builtin commands under PIOS.
	pid_t pid;
	for (pid = 1; pid < 256; pid++)
		if (files->child[pid].state == PROC_FREE)
			break;
	if (pid == 256) {
		warn("fork: no child process available");
		errno = EAGAIN;
		return -1;
	}

	// Set up the register state for the child
	struct cpustate cs;
	memset(&cs, 0, sizeof(cs));

	// Use some assembly magic to propagate registers to child
	// and generate an appropriate starting eip
	int isparent;
	asm volatile(
		"	movl	%%esi,%0;"
		"	movl	%%edi,%1;"
		"	movl	%%ebp,%2;"
		"	movl	%%esp,%3;"
		"	movl	$1f,%4;"
		"	movl	$1,%5;"
		"1:	"
		: "=m" (cs.tf.tf_regs.reg_esi),
		  "=m" (cs.tf.tf_regs.reg_edi),
		  "=m" (cs.tf.tf_regs.reg_ebp),
		  "=m" (cs.tf.tf_esp),
		  "=m" (cs.tf.tf_eip),
		  "=a" (isparent)
		:
		: "ebx", "ecx", "edx");
	if (!isparent) {
		// Clear our child state array, since we have no children yet.
		memset(&files->child, 0, sizeof(files->child));
		files->child[0].state = PROC_RESERVED;

		return 0;	// indicate that we're the child.
	}

	// Copy our entire user address space into the child and start it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(SYS_REGS | SYS_COPY | SYS_START, pid, &cs,
		ALLVA, ALLVA, ALLSIZE);

	// Record the inode generation numbers of all inodes at fork time,
	// so that we can reconcile them later when we synchronize with it.
	memset(&files->child[pid], 0, sizeof(files->child[pid]));
	files->child[pid].state = PROC_FORKED;
	files->child[pid].ip2c[FILEINO_CONSIN] = FILEINO_CONSIN;
	files->child[pid].ic2p[FILEINO_CONSIN] = FILEINO_CONSIN;
	files->child[pid].ip2c[FILEINO_CONSOUT] = FILEINO_CONSOUT;
	files->child[pid].ic2p[FILEINO_CONSOUT] = FILEINO_CONSOUT;
	files->child[pid].ip2c[FILEINO_ROOTDIR] = FILEINO_ROOTDIR;
	files->child[pid].ic2p[FILEINO_ROOTDIR] = FILEINO_ROOTDIR;
	int i;
	for (i = 1; i < FILE_INODES; i++) {
		files->child[pid].rver[i] = files->fi[i].ver;
		files->child[pid].rlen[i] = files->fi[i].size;
	}

	return pid;
}

pid_t
wait(int *status)
{
	return waitpid(-1, status, 0);
}

pid_t
waitpid(pid_t pid, int *status, int options)
{
	assert(pid >= -1 && pid < 256);

	// Find a process to wait for.
	// Of course for interactive or load-balancing purposes
	// we would like to have a way to wait for
	// whichever child process happens to finish first -
	// that requires a (nondeterministic) kernel API extension.
	if (pid <= 0)
		for (pid = 1; pid < 256; pid++)
			if (files->child[pid].state == PROC_FORKED)
				break;
	if (pid == 256 || files->child[pid].state != PROC_FORKED) {
		errno = ECHILD;
		return -1;
	}

	// Repeatedly synchronize with the chosen child until it exits.
	while (1) {
		// Wait for the child to finish whatever it's doing,
		// and extract its CPU and process/file state.
		struct cpustate cs;
		sys_get(SYS_COPY | SYS_REGS, pid, &cs,
			(void*)FILESVA, (void*)VM_SCRATCHLO, PTSIZE);
		filestate *cfiles = (filestate*)VM_SCRATCHLO;

		// Did the child take a trap?
		if (cs.tf.tf_trapno != T_SYSCALL) {
			// Yes - terminate the child WITHOUT reconciling,
			// since the child's results are probably invalid.
			if (status != NULL)
				*status = WSIGNALED | cs.tf.tf_trapno;

			done:
			// Clear out the child's address space.
			sys_put(SYS_ZERO, pid, NULL, ALLVA, ALLVA, ALLSIZE);
			files->child[pid].state = PROC_FREE;
			return pid;
		}

		// Reconcile our file system state with the child's.
		bool didio = reconcile(pid, cfiles);

		// Has the child exited gracefully?
		if (cfiles->exited) {
			if (status != NULL)
				*status = WEXITED | (cfiles->status & 0xff);
			goto done;
		}

		// If the child is waiting for new input
		// and the reconciliation above didn't provide anything new,
		// then wait for something new from OUR parent in turn.
		if (!didio)
			sys_ret();

		// Reconcile again, to forward any new I/O to the child.
		(void)reconcile(pid, cfiles);

		// Push the child's updated file state back into the child.
		sys_put(SYS_COPY | SYS_START, pid, NULL,
			(void*)VM_SCRATCHLO, (void*)FILESVA, PTSIZE);
	}
}

static bool
reconcile(pid_t pid, filestate *cfiles)
{
	bool didio = 0;
	int i;

	procinfo *pi = &files->child[pid];
	for (i = 1; i < FILE_INODES; i++) {
		// Merge from child into parent, then from parent into child.
		didio |= recinode(pid, cfiles, i, files, pi->ic2p);
		didio |= recinode(pid, files, i, cfiles, pi->ip2c);
#if LAB >= 99

		// Make temporary copies of both inodes,
		// and then reconcile each into the other.
		const fileinode fi = files->fi[i];
		const fileinode cfi = cfiles->fi[i];
		didio |= reconcile_inode(pid, i, &fi, &cfiles->fi[i], 1);
		didio |= reconcile_inode(pid, i, &cfi, &files->fi[i], 0);
#if LAB >= 99
		fileinode *fi = &files->fi[i];
		fileinode *cfi = &cfiles->fi[i];

		// Handle exclusive modifications via the generation count.
		int refgen = files->child[pid].igen[i];
		if (fi->gen > refgen) {
			if (cfi->gen > refgen) {
				// Changed in both parent and child - conflict!
				warn("reconcile: conflict in inode %d", i);
				fi->mode |= S_IFCONF;
				cfi->mode |= S_IFCONF;
				fi->gen = cfi->gen = MAX(fi->gen, cfi->gen);
				continue;
			}

			// Changed in parent but not child: copy to child.
		}
#endif

		// Reset our reference point generation number
		// for the next time we need to reconcile.
		files->child[pid].igen[i] = files->fi[i].gen;
#endif
	}

	return didio;
}

static bool
recinode(pid_t pid, filestate *src, int srci, filestate *dst, int *imap)
{
	if (src->fi[srci].de.d_name[0] == 0)
		return 0;	// Not allocated to a name yet

	int dsti = imap[srci];	// Map src inode to dst inode number
	if (dsti == 0) {	// No mapping yet?  Create one.
		int srcdi = src->fi[srci].dino;
		assert(srcdi > 0 && srcdi < FILE_INODES);
		int dstdi = imap[srcdi];
		assert(dstdi > 0 && dstdi < FILE_INODES); // should be mapped!

		for (dsti = FILEINO_GENERAL; dsti < FILE_INODES; dsti++)
			if (dst->fi[dsti].dino == dstdi &&
					strcmp(src->fi[srci].de.d_name,
						dst->fi[dsti].de.d_name) == 0)
				break;
		if (dsti == FILE_INODES) {	// Doesn't yet exist in dst
			for (dsti = FILEINO_GENERAL; dsti < FILE_INODES; dsti++)
				if (dst->fi[dsti].de.d_name[0] == 0)
					break;
			if (dsti == FILE_INODES) {
				warn("recinode: out of inodes");
				// Mark conflict in src?
				return 0;
			}

			strcpy(dst->fi[dsti].de.d_name,
				src->fi[srci].de.d_name);
			dst->fi[dsti].dino = dstdi;
			dst->fi[dsti].ver = 0;	// always start at ver 0
		}
		assert(dsti > 0 && dsti < FILE_INODES);
		imap[srci] = dsti;
	}
	assert(src->fi[srci].dino); assert(dst->fi[dsti].dino);
	assert(imap[src->fi[srci].dino] == dst->fi[dsti].dino);
	assert(strcmp(src->fi[srci].de.d_name, dst->fi[dsti].de.d_name) == 0);

	// Now figure out the reference version number for reconciliation
	assert(src == files || dst == files);
	int parentino = (src == files) ? srci : dsti;
	int *rverp = &files->child[pid].rver[parentino];
	int *rlenp = &files->child[pid].rlen[parentino];
	int rver = *rverp;
	int rlen = *rlenp;

	assert(src->fi[srci].ver >= rver);	// version # only increases
	if (dst->fi[dsti].ver == rver)	// within a version, length only grows
		assert(dst->fi[dsti].size >= rlen);

	// If no exclusive changes made in either parent or child,
	// then just merge any non-exclusive, append-only updates.
	if (src->fi[srci].ver == rver && dst->fi[dsti].ver == rver)
		return recappends(pid, src, srci, dst, dsti, rlenp);

	// Nothing to propagate from src to dst?  If so, done for now.
	if (src->fi[srci].ver == rver)
		return 0;

	// Destination also changed since reference version or size?  Conflict!
	if (dst->fi[dsti].ver != rver || dst->fi[dsti].size > rlen) {
		dst->fi[dsti].ver = src->fi[srci].ver;
		dst->fi[dsti].mode |= S_IFCONF;
		return 1;	// I/O of sorts did occur
	}

	// XXX validity-check src inode before propagating!

	// No conflict - bring dst inode up-to-date with respect to src
	assert(dst->fi[dsti].ver == rver);
	dst->fi[dsti].ver = src->fi[srci].ver;
	dst->fi[dsti].mode = src->fi[srci].mode;
	dst->fi[dsti].size = src->fi[srci].size;
	*rverp = src->fi[srci].ver;
	*rlenp = src->fi[srci].size;

	// ...and copy the entire file data area into the destination
	if (src == files)	// updating from parent to child
		sys_put(SYS_COPY, pid, NULL, FILEDATA(srci), FILEDATA(dsti),
			PTSIZE);
	else			// updating from child to parent
		sys_get(SYS_COPY, pid, NULL, FILEDATA(srci), FILEDATA(dsti),
			PTSIZE);
	return 1;	// update occurred
}

static bool
recappends(pid_t pid, filestate *src, int srci, filestate *dst, int dsti,
		int *rlenp)
{
	assert(src == files || dst == files);
	assert(src->fi[srci].ver == dst->fi[dsti].ver);

	// How much did the file grow in the src & dst since last reconcile?
	int rlen = *rlenp;
	int srclen = src->fi[srci].size;
	int dstlen = src->fi[dsti].size;
	int srcgrow = srclen - rlen;
	int dstgrow = dstlen - rlen;
	assert(srcgrow >= 0 && srcgrow <= FILE_MAXSIZE);
	assert(dstgrow >= 0 && dstgrow <= FILE_MAXSIZE);

	// Map if necessary and find src & dst file data areas.
	// The child's inode table is sitting at VM_SCRATCHLO,
	// so map the file data temporarily at VM_SCRATCHLO+PTSIZE.
	void *srcp, *dstp;
	if (src == files) {	// updating from parent to child
		sys_get(SYS_COPY, pid, NULL,
			FILEDATA(dsti), (void*)VM_SCRATCHLO+PTSIZE, PTSIZE);
		srcp = FILEDATA(srci);
		dstp = (void*)VM_SCRATCHLO+PTSIZE;
	} else {		// updating from child to parent
		sys_get(SYS_COPY, pid, NULL,
			FILEDATA(srci), (void*)VM_SCRATCHLO+PTSIZE, PTSIZE);
		srcp = (void*)FILEDATA(srci);
		srcp = (void*)VM_SCRATCHLO+PTSIZE;
	}

	// Would the new file size be too big after reconcile?  Conflict!
	int newlen = rlen + srcgrow + dstgrow;
	assert(newlen == srclen + dstgrow);
	assert(newlen == dstlen + srcgrow);
	if (newlen > FILE_MAXSIZE) {
		dst->fi[dsti].mode |= S_IFCONF;
		return 1;	// I/O of sorts did occur
	}

	// Make sure the perms are adequate on both src and dst
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		srcp, ROUNDUP(newlen, PAGESIZE));
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		dstp, ROUNDUP(newlen, PAGESIZE));

	// Copy the newly-added parts of the file in both directions.
	// Note that if both parent and child appended simultaneously,
	// the appended chunks will be left in opposite order in the two!
	// This is unavoidable if we want to allow simultaneous appends
	// and don't want to change already-written portions:
	// it's a price we pay for this relaxed consistency model.
	memcpy(dstp + dstlen, srcp + rlen, srcgrow);
	memcpy(srcp + srclen, dstp + rlen, dstgrow);
	src->fi[srci].size = newlen; assert(newlen == srclen + dstgrow);
	dst->fi[dsti].size = newlen; assert(newlen == dstlen + srcgrow);
	*rlenp = newlen; assert(newlen == rlen + srcgrow + dstgrow);

	// Copy updated file data back into the child
	if (src == files) {	// updating from parent to child
		assert(dstp == (void*)VM_SCRATCHLO+PTSIZE);
		sys_put(SYS_COPY, pid, NULL, dstp, FILEDATA(dsti), PTSIZE);
	} else {		// updating from child to parent
		assert(srcp == (void*)VM_SCRATCHLO+PTSIZE);
		sys_put(SYS_COPY, pid, NULL, srcp, FILEDATA(srci), PTSIZE);
	}

	// File merged!
	return 1;
}

#endif	// LAB >= 4
