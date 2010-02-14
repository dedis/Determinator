#if LAB >= 2
#ifndef PIOS_INC_SYSCALL_H
#define PIOS_INC_SYSCALL_H

#include <inc/trap.h>


// System call command codes (passed in EAX)
#define SYS_TYPE	0x00000003	// Basic operation type
#define SYS_CPUTS	0x00000000	// Write debugging string to console
#define SYS_PUT		0x00000001	// Push data to child and start it
#define SYS_PUT		0x00000001	// Push data to child and start it
#define SYS_GET		0x00000002	// Pull results from child
#define SYS_RET		0x00000003	// Return to parent

#define SYS_START	0x00000010	// Put: start child running
#if LAB >= 99
//#define SYS_STOP	0x00000010	// Get: stop child if not done [ND]
//#define SYS_ANY	0x00000020	// Get: get results from any child [ND]
#endif

#define SYS_REGS	0x00001000	// Get/put register state
#define SYS_FPU		0x00002000	// Get/put FPU state
#if LAB >= 3                    
#define SYS_MEM		0x00004000	// Get/put memory mappings
#if LAB >= 99
#define SYS_PROC	0x00008000	// Get/put child processes
#endif

#define SYS_MEMOP	0x00030000	// Get/put memory operation
#define SYS_ZERO	0x00010000	// Get/put fresh zero-filled memory
#define SYS_COPY	0x00020000	// Get/put virtual copy
#define SYS_MERGE	0x00030000	// Get: diffs only from last snapshot
#define SYS_SNAP	0x00040000	// Put: snapshot child state
#if LAB >= 99
#define SYS_SHARE	0x00080000	// Fresh memory should be shared [ND]
#endif

#define SYS_PERM	0x00000100	// Set memory permissions on get/put
#define SYS_READ	0x00000200	// Read permission (NB: in PTE_AVAIL)
#define SYS_WRITE	0x00000400	// Write permission (NB: in PTE_AVAIL)
#define SYS_RW		0x00000600	// Both read and write permission
#if LAB >= 99                  
#define SYS_EXEC	0x00000800	// Execute permission (NB: in PTE_AVAIL)
#define SYS_RWX		0x00000e00	// Read, write, and execute permission
#endif
#endif	// LAB >= 3


// Register conventions for CPUTS system call:
//	EAX:	System call command
//	EBX:	User pointer to string to output to console
#define SYS_CPUTS_MAX	256	// Max buffer length cputs will accept


// Register conventions on GET/PUT system call entry:
//	EAX:	System call command/flags (SYS_*)
//	EDX:	0-7:	Child process number to get/put
#if LAB >= 99
//		8-15:	if SYS_PROC: process number in child to copy
//		16-23:	if SYS_PROC: process number in parent to copy
#endif
//	EBX:	Get/put CPU state pointer for SYS_REGS and/or SYS_FPU)
//	ECX:	Get/put memory region size
//	ESI:	Get/put local memory region start
//	EDI:	Get/put child memory region start
//	EBP:	reserved


// CPU state save area format for GET/PUT with SYS_REGS flags
typedef struct cpustate {
	trapframe	tf;		// general registers
	fxsave		fx;		// x87/MMX/XMM registers
} cpustate;


// Prototypes for user-level syscalls stubs defined in lib/syscall.c
void sys_cputs(const char *s);
void sys_put(uint32_t flags, uint8_t child, cpustate *cpu);
void sys_get(uint32_t flags, uint8_t child, cpustate *cpu);
void sys_ret(void);

#endif /* !PIOS_INC_SYSCALL_H */
#endif /* LAB >= 2 */
