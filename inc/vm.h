#if LAB >= 3
// User-visible virtual memory layout definitions.
// See COPYRIGHT for copyright information.
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

#endif /* !PIOS_INC_VM_H */
#endif // LAB >= 3
