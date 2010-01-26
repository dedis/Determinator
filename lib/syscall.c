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
sys_put(uint32_t flags, uint8_t child, cpustate *cpu)
{
	asm volatile("int %0" :
		: "i" (T_SYSCALL),
		  "a" (SYS_PUT | flags),
		  "b" (cpu),
		  "d" (child)
		: "cc", "memory");
}

void
sys_get(uint32_t flags, uint8_t child, cpustate *cpu)
{
	asm volatile("int %0" :
		: "i" (T_SYSCALL),
		  "a" (SYS_GET | flags),
		  "b" (cpu),
		  "d" (child)
		: "cc", "memory");
}

void
sys_ret(void)
{
	asm volatile("int %0" : :
		"i" (T_SYSCALL),
		"a" (SYS_RET));
}

#endif	// LAB >= 2
