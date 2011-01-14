#if LAB >= 4
/*
 * More-or-less Unix-compatible process fork and wait functions,
 * which PIOS implements completely in the user space C library.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#include <inc/file.h>
#include <inc/stat.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/syscall.h>
#include <inc/assert.h>
#include <inc/errno.h>
#include <inc/mmu.h>
#include <inc/vm.h>


#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

bool reconcile(pid_t pid, filestate *cfiles);
bool reconcile_inode(pid_t pid, filestate *cfiles, int pino, int cino);
bool reconcile_merge(pid_t pid, filestate *cfiles, int pino, int cino);

pid_t fork(void)
{
	int i;

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
		: "=m" (cs.tf.regs.esi),
		  "=m" (cs.tf.regs.edi),
		  "=m" (cs.tf.regs.ebp),
		  "=m" (cs.tf.esp),
		  "=m" (cs.tf.eip),
		  "=a" (isparent)
		:
		: "ebx", "ecx", "edx");
	if (!isparent) {
		// Clear our child state array, since we have no children yet.
		memset(&files->child, 0, sizeof(files->child));
		files->child[0].state = PROC_RESERVED;
		for (i = 1; i < FILE_INODES; i++)
			if (fileino_alloced(i)) {
				files->fi[i].rino = i;	// 1-to-1 mapping
				files->fi[i].rver = files->fi[i].ver;
				files->fi[i].rlen = files->fi[i].size;
			}

		return 0;	// indicate that we're the child.
	}

	// Copy our entire user address space into the child and start it.
	cs.tf.regs.eax = 0;	// isparent == 0 in the child
	sys_put(SYS_REGS | SYS_COPY | SYS_START, pid, &cs,
		ALLVA, ALLVA, ALLSIZE);

	// Record the inode generation numbers of all inodes at fork time,
	// so that we can reconcile them later when we synchronize with it.
	memset(&files->child[pid], 0, sizeof(files->child[pid]));
	files->child[pid].state = PROC_FORKED;

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
		if (cs.tf.trapno != T_SYSCALL) {
			// Yes - terminate the child WITHOUT reconciling,
			// since the child's results are probably invalid.
			warn("child %d took trap %d, eip %x\n",
				pid, cs.tf.trapno, cs.tf.eip);
			if (status != NULL)
				*status = WSIGNALED | cs.tf.trapno;

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

// Reconcile our file system state, whose metadata is in 'files',
// with the file system state of child 'pid', whose metadata is in 'cfiles'.
// Returns nonzero if any changes were propagated, false otherwise.
bool
reconcile(pid_t pid, filestate *cfiles)
{
	bool didio = 0;
	int i;

	// Compute a parent-to-child and child-to-parent inode mapping table.
	int p2c[FILE_INODES], c2p[FILE_INODES];
	memset(p2c, 0, sizeof(p2c)); memset(c2p, 0, sizeof(c2p));
	p2c[FILEINO_CONSIN] = c2p[FILEINO_CONSIN] = FILEINO_CONSIN;
	p2c[FILEINO_CONSOUT] = c2p[FILEINO_CONSOUT] = FILEINO_CONSOUT;
	p2c[FILEINO_ROOTDIR] = c2p[FILEINO_ROOTDIR] = FILEINO_ROOTDIR;

	// First make sure all the child's allocated inodes
	// have a mapping in the parent, creating mappings as needed.
	// Also keep track of the parent inodes we find mappings for.
	int cino;
	for (cino = 1; cino < FILE_INODES; cino++) {
		fileinode *cfi = &cfiles->fi[cino];
		if (cfi->de.d_name[0] == 0)
			continue;	// not allocated in the child
		if (cfi->mode == 0 && cfi->rino == 0)
			continue;	// existed only ephemerally in child
		if (cfi->rino == 0) {
			// No corresponding parent inode known: find/create one.
			// The parent directory should already have a mapping.
			if (cfi->dino <= 0 || cfi->dino >= FILE_INODES
				|| c2p[cfi->dino] == 0) {
				warn("reconcile: cino %d has invalid parent",
					cino);
				continue;	// don't reconcile it
			}
			cfi->rino = fileino_create(files, c2p[cfi->dino],
							cfi->de.d_name);
			if (cfi->rino <= 0)
				continue;	// no free inodes!
		}

		// Check the validity of the child's existing mapping.
		// If something's fishy, just don't reconcile it,
		// since we don't want the child to kill the parent this way.
		int pino = cfi->rino;
		fileinode *pfi = &files->fi[pino];
		if (pino <= 0 || pino >= FILE_INODES
				|| p2c[pfi->dino] != cfi->dino
				|| strcmp(pfi->de.d_name, cfi->de.d_name) != 0
				|| cfi->rver > pfi->ver
				|| cfi->rver > cfi->ver) {
			warn("reconcile: mapping %d/%d: "
				"dir %d/%d name %s/%s ver %d/%d(%d)",
				pino, cino, pfi->dino, cfi->dino,
				pfi->de.d_name, cfi->de.d_name,
				pfi->ver, cfi->ver, cfi->rver);
			continue;
		}

		// Record the mapping.
		p2c[pino] = cino;
		c2p[cino] = pino;
	}

	// Now make sure all the parent's allocated inodes
	// have a mapping in the child, creating mappings as needed.
	int pino;
	for (pino = 1; pino < FILE_INODES; pino++) {
		fileinode *pfi = &files->fi[pino];
		if (pfi->de.d_name[0] == 0 || pfi->mode == 0)
			continue; // not in use or already deleted
		if (p2c[pino] != 0)
			continue; // already mapped
		cino = fileino_create(cfiles, p2c[pfi->dino], pfi->de.d_name);
		if (cino <= 0)
			continue;	// no free inodes!
		cfiles->fi[cino].rino = pino;
		p2c[pino] = cino;
		c2p[cino] = pino;
	}

	// Finally, reconcile each corresponding pair of inodes.
	for (pino = 1; pino < FILE_INODES; pino++) {
		if (!p2c[pino])
			continue;	// no corresponding inode in child
		cino = p2c[pino];
		assert(c2p[cino] == pino);

		didio |= reconcile_inode(pid, cfiles, pino, cino);
	}

	return didio;
}

bool
reconcile_inode(pid_t pid, filestate *cfiles, int pino, int cino)
{
	assert(pino > 0 && pino < FILE_INODES);
	assert(cino > 0 && cino < FILE_INODES);
	fileinode *pfi = &files->fi[pino];
	fileinode *cfi = &cfiles->fi[cino];

	// Find the reference version number and length for reconciliation
	int rver = cfi->rver;
	int rlen = cfi->rlen;

	// Check some invariants that should hold between
	// the parent's and child's current version numbers and lengths
	// and the reference version number and length stored in the child.
	// XXX should protect the parent better from state corruption by child.
	assert(cfi->ver >= rver);	// version # only increases
	assert(pfi->ver >= rver);
	if (cfi->ver == rver)		// within a version, length only grows
		assert(cfi->size >= rlen);
	if (pfi->ver == rver)
		assert(pfi->size >= rlen);

#if SOL >= 4
	// If no exclusive changes made in either parent or child,
	// then just merge any non-exclusive, append-only updates.
	if (pfi->ver == rver && cfi->ver == rver)
		return reconcile_merge(pid, cfiles, pino, cino);

	//cprintf("reconcile %s %d/%d: ver %d/%d(%d) len %d/%d(%d)\n",
	//	pfi->de.d_name, pino, cino,
	//	pfi->ver, cfi->ver, rver,
	//	pfi->size, cfi->size, rlen);

	// If both inodes have been changed and at least one was exclusive,
	// then we have a conflict - just mark both inodes conflicted.
	if ((pfi->ver > rver || pfi->size > rlen)
			&& (cfi->ver > rver || cfi->size > rlen)) {
		warn("reconcile_inode: parent/child conflict: %s (%d/%d)",
			pfi->de.d_name, pino, cino);
		pfi->mode |= S_IFCONF;
		cfi->mode |= S_IFCONF;
		pfi->ver = cfi->ver = cfi->rver = MAX(pfi->ver, cfi->ver);
		return 1;	// Changes of sorts were "propagated"
	}

	// No conflict: copy the latest version to the other.
	if (pfi->ver > rver || pfi->size > rlen) {
		// Parent's version is newer: copy to child.
		cfi->ver = pfi->ver;
		cfi->mode = pfi->mode;
		cfi->size = pfi->size;

		sys_put(SYS_COPY, pid, NULL, FILEDATA(pino), FILEDATA(cino),
			PTSIZE);
	} else {
		// Child's version is newer: copy to parent.
		pfi->ver = cfi->ver;
		pfi->mode = cfi->mode;
		pfi->size = cfi->size;

		sys_get(SYS_COPY, pid, NULL, FILEDATA(cino), FILEDATA(pino),
			PTSIZE);
	}

	// Reset child's reconciliation state.
	cfi->rver = pfi->ver;
	cfi->rlen = pfi->size;

	return 1;
#else	// ! SOL >= 4
	// Lab 4: insert your code here to reconcile the two inodes:
	// copy the parent's file to the child if only the parent's has changed,
	// copy the child's file to the parent if only the child's has changed,
	// and mark both files conflicted if both have been modified.
	// Then be sure to update the reconciliation state
	// so that the next reconciliation will start from this point.
	//
	// Note: if only one process has made an exclusive modification
	// that bumps the inode's version number,
	// and the other process has NOT bumped its inode's version number
	// but has performed append-only writes increasing the file's length,
	// that situation still constitutes a conflict
	// because we don't have a clean way to resolve it automatically.
	warn("reconcile_inode not implemented");
	return 0;
#endif	// ! SOL >= 4
}

bool
reconcile_merge(pid_t pid, filestate *cfiles, int pino, int cino)
{
	fileinode *pfi = &files->fi[pino];
	fileinode *cfi = &cfiles->fi[cino];
	assert(pino > 0 && pino < FILE_INODES);
	assert(cino > 0 && cino < FILE_INODES);
	assert(pfi->ver == cfi->ver);
	assert(pfi->mode == cfi->mode);

	if (!S_ISREG(pfi->mode))
		return 0;	// only regular files have data to merge

#if SOL >= 4
	// How much did the file grow in the src & dst since last reconcile?
	int rlen = cfi->rlen;
	int plen = pfi->size;
	int clen = cfi->size;
	int pgrow = plen - rlen;
	int cgrow = clen - rlen;
	assert(pgrow >= 0 && pgrow <= FILE_MAXSIZE);
	assert(cgrow >= 0 && cgrow <= FILE_MAXSIZE);

	if (pgrow == 0 && cgrow == 0)
		return 0;	// nothing to merge

	// Map if necessary and find src & dst file data areas.
	// The child's inode table is sitting at VM_SCRATCHLO,
	// so map the file data temporarily at VM_SCRATCHLO+PTSIZE.
	void *pp = FILEDATA(pino);
	void *cp = (void*)VM_SCRATCHLO+PTSIZE;
	sys_get(SYS_COPY, pid, NULL, FILEDATA(cino), cp, PTSIZE);

	// Would the new file size be too big after reconcile?  Conflict!
	int newlen = rlen + pgrow + cgrow;
	assert(newlen == plen + cgrow);
	assert(newlen == clen + pgrow);
	if (newlen > FILE_MAXSIZE) {
		pfi->mode |= S_IFCONF;
		cfi->mode |= S_IFCONF;
		return 1;	// I/O of sorts did occur
	}

	// Make sure the perms are adequate in both copies of file
	int pagelen = ROUNDUP(newlen, PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, pp, pagelen);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, cp, pagelen);

	// Copy the newly-added parts of the file in both directions.
	// Note that if both parent and child appended simultaneously,
	// the appended chunks will be left in opposite order in the two!
	// This is unavoidable if we want to allow simultaneous appends
	// and don't want to change already-written portions:
	// it's a price we pay for this relaxed consistency model.
	memcpy(pp + plen, cp + rlen, cgrow);
	memcpy(cp + clen, pp + rlen, pgrow);
	pfi->size = newlen; assert(newlen == plen + cgrow);
	cfi->size = newlen; assert(newlen == clen + pgrow);
	cfi->rlen = newlen; assert(newlen == rlen + pgrow + cgrow);

	// Copy child's updated file data back into the child
	sys_put(SYS_COPY, pid, NULL, cp, FILEDATA(cino), PTSIZE);

	// File merged!
	return 1;
#else	// ! SOL >= 4
	// Lab 4: insert your code here to merge inclusive appends:
	// copy the parent's appends since last reconciliation into the child,
	// and the child's appends since last reconciliation into the parent.
	// Parent and child should be left with files of the same size,
	// although the writes they contain may be in a different order.
	warn("reconcile_merge not implemented");
	return 0;
#endif	// ! SOL >= 4
}

#endif	// LAB >= 4
