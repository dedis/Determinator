/*
 * x86 processor-specific definitions and utility functions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_X86_H
#define PIOS_INC_X86_H

#include <types.h>
#include <cdefs.h>

// RFLAGS register
#define FL_CF		0x00000001	// Carry Flag
#define FL_PF		0x00000004	// Parity Flag
#define FL_AF		0x00000010	// Auxiliary carry Flag
#define FL_ZF		0x00000040	// Zero Flag
#define FL_SF		0x00000080	// Sign Flag
#define FL_TF		0x00000100	// Trap Flag
#define FL_IF		0x00000200	// Interrupt Flag
#define FL_DF		0x00000400	// Direction Flag
#define FL_OF		0x00000800	// Overflow Flag
#define FL_IOPL_MASK	0x00003000	// I/O Privilege Level bitmask
#define FL_IOPL_0	0x00000000	//   IOPL == 0
#define FL_IOPL_1	0x00001000	//   IOPL == 1
#define FL_IOPL_2	0x00002000	//   IOPL == 2
#define FL_IOPL_3	0x00003000	//   IOPL == 3
#define FL_NT		0x00004000	// Nested Task
#define FL_RF		0x00010000	// Resume Flag
#define FL_VM		0x00020000	// Virtual 8086 mode
#define FL_AC		0x00040000	// Alignment Check
#define FL_VIF		0x00080000	// Virtual Interrupt Flag
#define FL_VIP		0x00100000	// Virtual Interrupt Pending
#define FL_ID		0x00200000	// ID flag


// Struct containing information returned by the CPUID instruction
typedef struct cpuinfo {
	uint32_t	eax;
	uint32_t	ebx;
	uint32_t	edx;
	uint32_t	ecx;
} cpuinfo;

// Used for executing ljmp (known as far jump in the x86-64 ISA) in 64-bit mode
// Using the JMP FAR mem16:32 form
struct farptr32 {
	uint16_t cs;
	uint32_t gcc_packed rip;
};


/*TODO:: Ishan - 29 May, 2011 
  *check if we need functions to write 64 bit values to ports	
  *check if we need to read eflags or rflags. currently we have function to read rflags only.
  *may need to add or remove from model specific instructions.
*/


static gcc_inline void
breakpoint(void)
{
	__asm __volatile("int3");
}

static gcc_inline uint8_t
inb(int port)
{
	uint8_t data;
	__asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static gcc_inline void
insb(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsb"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static gcc_inline uint16_t
inw(int port)
{
	uint16_t data;
	__asm __volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static gcc_inline void
insw(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsw"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static gcc_inline uint32_t
inl(int port)
{
	uint32_t data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static gcc_inline void
insl(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsl"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static gcc_inline uint64_t
inq(int port)
{
	uint64_t data;
	__asm __volatile("inq %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static gcc_inline void
insq(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsq"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static gcc_inline void
outb(int port, uint8_t data)
{
	__asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static gcc_inline void
outsb(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsb"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static gcc_inline void
outw(int port, uint16_t data)
{
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static gcc_inline void
outsw(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsw"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static gcc_inline void
outl(int port, uint32_t data)
{
	__asm __volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static gcc_inline void
outsl(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsl"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static gcc_inline void
outq(int port, uint64_t data)
{
	__asm __volatile("outq %0,%w1" : : "a" (data), "d" (port));
}

static gcc_inline void
outsq(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsq"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static gcc_inline void 
invlpg(void *addr)
{ 
	__asm __volatile("invlpg (%0)" : : "r" (addr) : "memory");
}  

static gcc_inline void
lidt(void *p)
{
	__asm __volatile("lidt (%0)" : : "r" (p));
}

static gcc_inline void
lldt(uint16_t sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static gcc_inline void
ltr(uint16_t sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static gcc_inline void
lcr0(uint64_t val)
{
	__asm __volatile("movq %0,%%cr0" : : "r" (val));
}

static gcc_inline uint64_t
rcr0(void)
{
	uint64_t val;
	__asm __volatile("movq %%cr0,%0" : "=r" (val));
	return val;
}

static gcc_inline uint64_t
rcr2(void)
{
	uint64_t val;
	__asm __volatile("movq %%cr2,%0" : "=r" (val));
	return val;
}

static gcc_inline void
lcr3(uint64_t val)
{
	__asm __volatile("movq %0,%%cr3" : : "r" (val));
}

static gcc_inline uint64_t
rcr3(void)
{
	uint64_t val;
	__asm __volatile("movq %%cr3,%0" : "=r" (val));
	return val;
}

static gcc_inline void
lcr4(uint64_t val)
{
	__asm __volatile("movq %0,%%cr4" : : "r" (val));
}

static gcc_inline uint64_t
rcr4(void)
{
	uint64_t cr4;
	__asm __volatile("movq %%cr4,%0" : "=r" (cr4));
	return cr4;
}

static gcc_inline void
tlbflush(void)
{
	uint64_t cr3;
	__asm __volatile("movq %%cr3,%0" : "=r" (cr3));
	__asm __volatile("movq %0,%%cr3" : : "r" (cr3));
}

static gcc_inline uint64_t
read_rflags(void)
{
        uint64_t rflags;
        __asm __volatile("pushfq; popq %0" : "=rm" (rflags));
        return rflags;
}

static gcc_inline void
write_rflags(uint64_t rflags)
{
        __asm __volatile("pushq %0; popfq" : : "rm" (rflags));
}

static gcc_inline uint64_t
read_rbp(void)
{
        uint64_t rbp;
        __asm __volatile("movq %%rbp,%0" : "=rm" (rbp));
        return rbp;
}

static gcc_inline uint64_t
read_rsp(void)
{
        uint64_t rsp;
        __asm __volatile("movq %%rsp,%0" : "=rm" (rsp));
        return rsp;
}

static gcc_inline uint16_t
read_cs(void)
{
        uint16_t cs;
        __asm __volatile("movw %%cs,%0" : "=rm" (cs));
        return cs;
}

// Atomically set *addr to newval and return the old value of *addr.
static inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
	uint32_t result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock; xchgl %0, %1" :
	       "+m" (*addr), "=a" (result) :
	       "1" (newval) :
	       "cc");
	return result;
}

// Atomically add incr to *addr.
static inline void
lockadd(volatile int32_t *addr, int32_t incr)
{
	asm volatile("lock; addl %1,%0" : "+m" (*addr) : "r" (incr) : "cc");
}

// Atomically add incr to *addr and return true if the result is zero.
static inline uint8_t
lockaddz(volatile int32_t *addr, int32_t incr)
{
	uint8_t zero;
	asm volatile("lock; addl %2,%0; setzb %1"
		: "+m" (*addr), "=rm" (zero)
		: "r" (incr)
		: "cc");
	return zero;
}

// Atomically add incr to *addr and return the old value of *addr.
static inline int64_t
xadd(volatile uint64_t *addr, int64_t incr)
{
	int64_t result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock; xaddq %0, %1" :
	       "+m" (*addr), "=a" (result) :
	       "1" (incr) :
	       "cc");
	return result;
}

static inline void
pause(void)
{
	asm volatile("pause" : : : "memory");
}

static gcc_inline void
cpuid(uint32_t idx, cpuinfo *info)
{
	asm volatile("cpuid" 
		: "=a" (info->eax), "=b" (info->ebx),
		  "=c" (info->ecx), "=d" (info->edx)
		: "a" (idx));
}

static gcc_inline uint64_t
rdtsc(void)
{
        uint64_t tsc;
        asm volatile("rdtsc" : "=A" (tsc));
        return tsc;
}

// Enable external device interrupts.
static gcc_inline void
sti(void)
{
	asm volatile("sti");
}

// Disable external device interrupts.
static gcc_inline void
cli(void)
{
	asm volatile("cli");
}

#if LAB >= 5
// Byte-swap a 32-bit word to convert to/from big-endian byte order.
// (Reverses the order of the 4 bytes comprising the word.)
static gcc_inline uint64_t
bswap(uint64_t v)
{
	uint64_t r;
	asm volatile("bswap %0" : "=r" (r) : "0" (v));
	return r;
}

// Host/network byte-order conversion for x86
#define htons(v)	(((uint16_t)(v) >> 8) | (uint16_t)((v) << 8))
#define ntohs(v)	(((uint16_t)(v) >> 8) | (uint16_t)((v) << 8))
#define htonl(v)	bswap(v)
#define ntohl(v)	bswap(v)
#endif // LAB >= 5

#if LAB >= 9
// Read and write model-specific registers.
static gcc_inline uint64_t
rdmsr(int32_t msr)
{
        uint64_t val;
        asm volatile("rdmsr" : "=A" (val) : "c" (msr));
        return val;
}

static gcc_inline void
wrmsr(int32_t msr, uint64_t val)
{
        asm volatile("wrmsr" : : "c" (msr), "A" (val));
}

// Read performanc-monitoring counters.
static gcc_inline uint32_t
rdpmc(int32_t ctr)
{
        uint64_t v;
        asm volatile("rdpmc" : "=A" (v) : "c" (ctr));
        return v;
}
#endif // LAB >= 9

#endif /* !PIOS_INC_X86_H */
