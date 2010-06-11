#if LAB >= 2
/*
 * PIOS system call definitions.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#ifndef PIOS_INC_SYSCALL_H
#define PIOS_INC_SYSCALL_H

#include <trap.h>


// System call command codes (passed in EAX)
#define SYS_TYPE	0x0000000f	// Basic operation type
#define SYS_CPUTS	0x00000000	// Write debugging string to console
#define SYS_PUT		0x00000001	// Push data to child and start it
#define SYS_GET		0x00000002	// Pull results from child
#define SYS_RET		0x00000003	// Return to parent
#if LAB >= 9
#define SYS_TIME	0x00000004	// Get time since kernel boot
#define SYS_NCPU	0x00000005	// Set max number of running CPUs
#endif

#define SYS_START	0x00000010	// Put: start child running
#if LAB >= 99
//#define SYS_STOP	0x00000010	// Get: stop child if not done [ND]
//#define SYS_ANY	0x00000020	// Get: get results from any child [ND]
#endif

#define SYS_REGS	0x00001000	// Get/put register state
#define SYS_FPU		0x00002000	// Get/put FPU state (with SYS_REGS)
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
#if LAB >= 5
//	EDX:	bits 15-8: Node number to migrate to, 0 for current
//		bits 7-0: Child process number on above node to get/put
#else
//	EDX:	bits 7-0: Child process number to get/put
#endif
#if LAB >= 99
//		8-15:	if SYS_PROC: process number in child to copy
//		16-23:	if SYS_PROC: process number in parent to copy
#endif
//	EBX:	Get/put CPU state pointer for SYS_REGS and/or SYS_FPU)
//	ECX:	Get/put memory region size
//	ESI:	Get/put local memory region start
//	EDI:	Get/put child memory region start
//	EBP:	reserved


#ifndef __ASSEMBLER__

// CPU state save area format for GET/PUT with SYS_REGS flags
// XXX rename to 'procstate'?
typedef struct cpustate {
	uint32_t	pff;		// process feature flags - see below
	uint32_t	icnt;		// insns executed so far
	uint32_t	imax;		// max insns to execute before ret
	trapframe	tf;		// general registers
	fxsave		fx;		// x87/MMX/XMM registers
} cpustate;

// process feature enable/status flags
#define PFF_USEFPU	0x0001		// process has used the FPU
#define PFF_NONDET	0x0100		// enable nondeterministic features
#define PFF_ICNT	0x0200		// enable instruction count/recovery


static void gcc_inline
sys_cputs(const char *s)
{
	// Pass system call number and flags in EAX,
	// parameters in other registers.
	// Interrupt kernel with vector T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because it doesn't
	// look to the compiler like it computes useful values.
	// 
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %0" :
		: "i" (T_SYSCALL),
		  "a" (SYS_CPUTS),
		  "b" (s)
		: "cc", "memory");
}

static void gcc_inline
sys_put(uint32_t flags, uint16_t child, cpustate *cpu,
		void *localsrc, void *childdest, size_t size)
{
	asm volatile("int %0" :
		: "i" (T_SYSCALL),
		  "a" (SYS_PUT | flags),
		  "b" (cpu),
		  "d" (child),
		  "S" (localsrc),
		  "D" (childdest),
		  "c" (size)
		: "cc", "memory");
}

static void gcc_inline
sys_get(uint32_t flags, uint16_t child, cpustate *cpu,
		void *childsrc, void *localdest, size_t size)
{
	asm volatile("int %0" :
		: "i" (T_SYSCALL),
		  "a" (SYS_GET | flags),
		  "b" (cpu),
		  "d" (child),
		  "S" (childsrc),
		  "D" (localdest),
		  "c" (size)
		: "cc", "memory");
}

static void gcc_inline
sys_ret(void)
{
	asm volatile("int %0" : :
		"i" (T_SYSCALL),
		"a" (SYS_RET));
}

#if LAB >= 9
static uint64_t gcc_inline
sys_time(void)
{
	uint32_t hi, lo;
	asm volatile("int %2"
		: "=d" (hi),
		  "=a" (lo)
		: "i" (T_SYSCALL),
		  "a" (SYS_TIME));
	return (uint64_t)hi << 32 | lo;
}

static void gcc_inline
sys_ncpu(int newlimit)
{
	asm volatile("int %0"
		:
		: "i" (T_SYSCALL),
		  "a" (SYS_NCPU),
		  "c" (newlimit));
}
#endif	// SOL >= 4

#endif /* !__ASSEMBLER__ */

#endif /* !PIOS_INC_SYSCALL_H */
#endif /* LAB >= 2 */
