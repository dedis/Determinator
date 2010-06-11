#if LAB >= 3
/*
 * User-visible virtual memory layout definitions for PIOS.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#ifndef PIOS_INC_VM_H
#define PIOS_INC_VM_H

//
// The PIOS kernel divides the 4GB linear (post-segmentation) address space
// into three parts:
//
// - The low 1GB contains fixed direct mappings of physical memory,
//   representing the address space in which the kernel operates.
//   This way the kernel's address space effectively remains the same
//   both before and after it initializes the MMU and enables paging.
//   (It also means we can use at most 1GB of physical memory!)
//
// - The next 2.75GB contains the running process's user-level address space.
//   This is the only address range user-mode processes can access or map.
//
// - The top 256MB once again contains direct mappings of physical memory,
//   giving the kernel access to the high I/O region, e.g., the local APIC.
//
// Kernel's linear address map: 	              Permissions
//                                                    kernel/user
//
//    4 Gig ---------> +==============================+
//                     |                              | RW/--
//                     |    High 32-bit I/O region    | RW/--
//                     |                              | RW/--
//    VM_USERHI -----> +==============================+ 0xf0000000
//                     |                              | RW/RW
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/RW
//                     |  User address space (2.75GB) | RW/RW
//                     |        (see inc/vm.h)        | RW/RW
//                     |                              | RW/RW
//    VM_USERLO -----> +==============================+ 0x40000000
//                     |                              | RW/--
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/--
//                     |    Physical memory (1GB)     | RW/--
//                     |    incl. I/O, kernel, ...    | RW/--
//                     |                              | RW/--
//    0 -------------> +==============================+
//
#define	VM_USERHI	0xf0000000
#define	VM_USERLO	0x40000000


//
// Within the user-space region, user processes are technically free
// to organize their address space however they see fit.
// However, the following layout reflects conventions
// that PIOS's user-space library infrastructure use
// for communication between parent and child processes
// (and ultimately between the PIOS kernel and the root process).
//
//                     |                              |
//    VM_USERHI, ----> +==============================+ 0xf0000000
//    VM_STACKHI       |                              |
//                     |     Per-thread user stack    |
//                     |                              |
//    VM_STACKLO,      +------------------------------+ 0xd0000000
//    VM_SCRATCHHI     |                              |
//                     |    Scratch address space     |
//                     | for file reconciliation etc. |
//                     |                              |
//    VM_SCRATCHLO, -> +------------------------------+ 0xc0000000
//    VM_FILEHI        |                              |
//                     |      File system and         |
//                     |   process management state   |
//                     |                              |
//    VM_FILELO, ----> +------------------------------+ 0x80000000
//    VM_SHAREHI       |                              |
//                     |  General-purpose use space   |
//                     | shared between user threads: |
//                     |   program text, data, heap   |
//                     |                              |
//    VM_SHARELO, ---> +==============================+ 0x40000000
//    VM_USERLO        |                              |

// Standard area for the user-space stack (thread-private)
#define VM_STACKHI	0xf0000000
#define VM_STACKLO	0xd0000000

// Scratch address space region for general use (e.g., by exec)
#define VM_SCRATCHHI	0xd0000000
#define VM_SCRATCHLO	0xc0000000

// Address space area for file system and Unix process state
#define VM_FILEHI	0xc0000000
#define VM_FILELO	0x80000000

// General-purpose address space shared between "threads"
// created via SYS_SNAP/SYS_MERGE.
#define VM_SHAREHI	0x80000000
#define VM_SHARELO	0x40000000


#endif /* !PIOS_INC_VM_H */
#endif // LAB >= 3
