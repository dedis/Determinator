#if LAB >= 1
// User-visible virtual memory layout and related definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_INC_VM_H
#define PIOS_INC_VM_H

/*
 * This file contains definitions for memory management in our OS,
 * which are relevant to both the kernel and user-mode software.
 *
 * User-visible virtual memory map:                    user
 *                                                  permissions  
 *    4 Gig ---------> +==============================+
 *                     |                              | --
 *                     |     Reserved for kernel:     | --
 *                     |      always inaccessible     | --
 *                     |       to user processes      | --
 *                     |        (see kern/vm.h)       | --
 *    VM_KERNLO,       |                              | --
 *    VM_USERHI, ----> +==============================+ 0x80000000
 *    VM_USTACKHI      |                              | RW
 *                     | User process grow-down stack | RW
 *                     |                              | RW
 *    VM_USTACKLO, --> +------------------------------+ 0x70000000      --+
 *                     |                              | RW
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     :              .               :
 *                     :              .               :
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     |                              | RW
 *                     |      Dynamic user heap       | RW
 *                     |                              | RW
 *                     | - - - - - - - - - - - - - - -|
 *                     |    Static user data, bss     | RW
 *                     | - - - - - - - - - - - - - - -|
 *                     |          User code           | RO
 *    VM_UTEXT ------> +------------------------------+ 0x00800000
 *                     |                              |
 *                     | - - - - - - - - - - - - - - -|
 *                     |  User STAB Data (optional)   | RO
 *    VM_USTABS -----> +------------------------------+ 0x00200000
 *                     |     Unmapped Memory (*)      |
 *    0 -------------> +==============================+
 *
 * (*) Note: "Unmapped Memory" is normally unmapped, but user programs may
 *     map pages there if desired.
 */


// All virtual address space above the 2GB mark is kernel-accessible only;
// user processes cannot map anything there and always fault if they touch it.
// Virtual address space below this mark may be user-accessible if mapped.
#define VM_USERHI	0x80000000
#define VM_KERNLO	0x80000000


// The user stack grows downward from the top of user-accessible memory.
// This region allows the user stack to grow up to 256MB in size.
//
// The kernel needs to know the boundary between user stack
// and normal user code/data/bss/heap because the stack is thread-private:
// when a group of forked child threads synchronize and rejoin their parent,
// the kernel thread does not attempt to reconcile the user memory
// from VM_USTACKLO up to the parent thread's ESP at the time of fork,
// because that area will contain different private garbage for each thread.
//
#define VM_USTACKHI	0x80000000
#define VM_USTACKLO	0x70000000


// We link user-level programs so as to start at this address.
#define VM_UTEXT	0x00800000

// The location of the user-level STABS debugging structure, if loaded
#define VM_USTABS	0x00200000


#endif /* !PIOS_INC_VM_H */
#endif // LAB >= 1
