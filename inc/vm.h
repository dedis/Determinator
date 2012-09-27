#if LAB >= 3
/*
 * User-visible virtual memory layout definitions for 64-bit PIOS.
 * Yu Zhang (clara)
 */

#ifndef PIOS_INC_VM_H
#define PIOS_INC_VM_H

//
// The PIOS kernel divides the 2^64B (effectively 2^48B) linear address space
// into four parts:
//
//
// Kernel's linear address map: 	              Permissions
//                                                    kernel/user
//
//    2^47-1---------> +==============================+
//                     |                              | --/--
//                     |    Unused                    | --/--
//                     |                              | --/--
//    VM_USERHI -----> +==============================+ 0x0000800000000000 (128TB)
//                     |                              | RW/RW
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |       (2^17-1) GB            | RW/RW
//                     |User address space (128TB-1GB)| RW/RW
//                     |        (see inc/vm.h)        | RW/RW
//                     |                              | RW/RW
//    VM_USERLO -----> +==============================+ 0x40000000 (1GB)
//                     |                              | RW/--
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/--
//                     |    Physical memory           | RW/--
//                     |    incl. I/O, kernel, ...    | RW/--
//                     |                              | RW/--
//    VM_KERNHI -----> +==============================+ 0x0
//                     |                              | RW/--
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/--
//                     |    Identical mapping         | RW/--
//                     |    of physical memory        | RW/--
//                     |                              | RW/--
//    VM_KERNLO -----> +==============================+ 0xffffff8000000000 (-512GB)
//                     |                              | --/--
//                     |    Unused                    | --/--
//                     |                              | --/--
//    -2^47 --- -----> +==============================+ 0xffff800000000000 (-128TB)
//
#define VM_USERHI	0x0000800000000000
#define VM_USERLO	0x0000000040000000
#define VM_KERNHI	0x0000000000000000
#define VM_KERNLO	0xffffff8000000000
#define ALLVA		((void *)VM_USERLO)
#define ALLSIZE         (VM_USERHI - VM_USERLO)

//
// Within the user-space region, user processes are technically free
// to organize their address space however they see fit.
// However, the following layout reflects conventions
// that PIOS's user-space library infrastructure use
// for communication between parent and child processes
// (and ultimately between the PIOS kernel and the root process).
//
//                     |                              |
//    VM_USERHI, ----> +==============================+ 0x800000000000
#if LAB >= 9
//    VM_PRIVHI        |                              |
//                     |     Thread-private state     |    (1TB)
//                     |                              |
//    VM_PRIVLO,       +------------------------------+ 0x7f0000000000
#endif
//    VM_STACKHI       |                              |
//                     |          User stack          |    (1TB)
//                     |                              |
//    VM_STACKLO,      +------------------------------+ 0x7e0000000000
//    VM_SCRATCHHI     |                              |
//                     |    Scratch address space     |
//                     | for file reconciliation etc. |    (1TB)
//                     |                              |
//    VM_SCRATCHLO, -> +------------------------------+ 0x7d0000000000
//                     |                              |
#if LAB >= 9
//    VM_SPMCHI        +------------------------------+ 0x700000000000
//                     |                              |
//                     |      SPMC Regions            |    
//                     |                              | 
//    VM_SPMCLO,       +------------------------------+ 0x300000000000
#endif
//    VM_FILEHI        |                              |
//                     |      File system and         |    (16TB)
//                     |   process management state   |
//                     |                              |
//    VM_FILELO, ----> +------------------------------+ 0x200000000000
//    VM_SHAREHI       |                              |
//                     |  General-purpose use space   |
//                     | shared between user threads: |    (31TB)
//                     |   program text, data, heap   |
//                     |                              |
//    VM_SHARELO   ---> +==============================+ 0x000040000000(1GB)
//    VM_USERLO, ---> 


#if LAB >= 9
// Standard area for state that is always thread-private,
// regardless of the user-level threading model in use.
#define VM_PRIVHI	0x800000000000
#define VM_PRIVLO	0x7f0000000000

// Standard area for the user-space stack:
// may or may not be thread-private depending on threading model.
#define VM_STACKHI	0x7f0000000000
#else	// LAB < 9 (PIOS) - no thread-private area
#define VM_STACKHI	0x800000000000
#endif	// LAB < 9
#define VM_STACKLO	0x7e0000000000

// Scratch address space region for general use (e.g., by exec)
#define VM_SCRATCHHI	0x7e0000000000
#define VM_SCRATCHLO	0x7d0000000000

// labelled message address space
#define VM_LABELHI	0x7d0000000000
#define VM_LABELLO	0x7c0000000000

#if LAB >= 9
#ifdef PIOS_SPMC
// Address space area for SPMC(single producer multiple-consumer) system
#define VM_SPMCHI	0x300100000000
#define VM_SPMCLO	0x300000000000
#endif
#endif

// Address space area for file system and Unix process state
#define VM_FILEHI	0x300000000000
#define VM_FILELO	0x200000000000

// General-purpose address space shared between "threads"
// created via SYS_SNAP/SYS_MERGE.
#define VM_SHAREHI	(VM_USERLO + 0x000100000000)
#define VM_SHARELO	VM_USERLO
#define SHAREVA         ((void*) VM_SHARELO)
#define SHARESIZE       (VM_SHAREHI - VM_SHARELO)

#endif /* !PIOS_INC_VM_H */
#endif // LAB >= 3
