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

#define SYS_REGS	0x00000100	// Get/put register state
#define SYS_FPU		0x00000200	// Get/put FPU state
#if LAB >= 3
#define SYS_MEM		0x00000400	// Get/put memory mappings
#define SYS_PROC	0x00000800	// Get/put child processes
                                 
#define SYS_ZERO	0x00001000	// Get/put fresh zero-filled memory
#define SYS_SHARE	0x00002000	// Fresh memory should be shared [ND]
#define SYS_SNAP	0x00004000	// Put: snapshot child state
#define SYS_DIFF	0x00004000	// Get: diffs only from last snapshot
                                 
#define SYS_PERM	0x00010000	// Set memory permissions on get/put
#define SYS_READ	0x00020000	// Read permission
#define SYS_WRITE	0x00040000	// Write permission
#define SYS_EXEC	0x00080000	// Execute permission
#endif	// LAB >= 3


// Register conventions for CPUTS system call:
//	EAX:	System call command
//	EBX:	User pointer to string to output to console
#define SYS_CPUTS_MAX	256	// Max buffer length cputs will accept


// Register conventions on GET/PUT system call entry:
//	EAX:	System call command/flags (SYS_*)
//	EBX:	Get/put CPU state pointer for SYS_REGS and/or SYS_FPU)
//	ECX:	Get/put local memory region limit
//	EDX:	0-7:	Child process number to get/put
//		8-15:	if SYS_PROC: process number in child to copy
//		16-23:	if SYS_PROC: process number in parent to copy
//	ESI:	Get/put parent memory region start
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
