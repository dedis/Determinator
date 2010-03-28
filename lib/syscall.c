#if LAB >= 2
// System call stubs.

#include <inc/syscall.h>

void
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

void
sys_put(uint32_t flags, uint8_t child, cpustate *cpu,
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

void
sys_get(uint32_t flags, uint8_t child, cpustate *cpu,
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

void
sys_ret(void)
{
	asm volatile("int %0" : :
		"i" (T_SYSCALL),
		"a" (SYS_RET));
}

#if SOL >= 4
uint64_t
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
#endif

#endif	// LAB >= 2
