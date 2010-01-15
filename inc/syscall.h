#if LAB >= 2
#ifndef PIOS_INC_SYSCALL_H
#define PIOS_INC_SYSCALL_H


// System call command codes (passed in EAX)
#define SYS_TYPE	0x00000003	// Basic operation type
#define SYS_RET		0x00000000	// Return to parent
#define SYS_PUT		0x00000001	// Push data to child and start it
#define SYS_GET		0x00000002	// Pull results from child

#define SYS_START	0x00000010	// Put: start child running
#define SYS_STOP	0x00000010	// Get: stop child if not done [ND]
#define SYS_ANY		0x00000020	// Get: get results from any child [ND]

#define SYS_REGS	0x00000100	// Get/put register state
#define SYS_FPU		0x00000200	// Get/put FPU state
#define SYS_MEM		0x00000400	// Get/put memory mappings
#define SYS_PROC	0x00000800	// Get/put child processes
                                 
#define SYS_ZERO	0x00001000	// Get/put fresh zero-filled memory
#define SYS_SHARE	0x00002000	// Fresh memory should be shared [ND]
                                 
#define SYS_PERM	0x00010000	// Get/put memory permissions
#define SYS_READ	0x00020000	// Read permission
#define SYS_WRITE	0x00040000	// Write permission
#define SYS_EXEC	0x00040000	// Execute permission


// Register conventions on system call entry:
//	EAX:	System call command/flags (SYS_*)
//	EBX:	Get/put CPU state pointer for SYS_REGS and/or SYS_FPU)
//	ECX:	Get/put local memory region limit
//	EDX:	0-7:	Child process number to get/put
//		8-15:	if SYS_PROC: process number in child to copy
//		16-23:	if SYS_PROC: process number in parent to copy
//	ESI:	Get/put parent memory region start
//	EDI:	Get/put child memory region start
//	EBP:	reserved


#endif /* !PIOS_INC_SYSCALL_H */
#endif /* LAB >= 2 */
