#ifndef _X86_H_
#define _X86_H_

#include <inc/types.h>

static __inline u_int8_t
inb(int port)
{
	u_int8_t data;
	__asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insb(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsb"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static __inline u_int16_t
inw(int port)
{
	u_int16_t data;
	__asm __volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insw(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsw"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static __inline u_int32_t
inl(int port)
{
	u_int32_t data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insl(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsl"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static __inline void
outb(int port, u_int8_t data)
{
	__asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsb(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsb"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static __inline void
outw(int port, u_int16_t data)
{
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsw(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsw"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static __inline void
outsl(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsl"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static __inline void
outl(int port, u_int32_t data)
{
	__asm __volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static __inline void 
invlpg(u_int addr)
{ 
	__asm __volatile("invlpg (%0)" : : "r" (addr) : "memory");
}  

static __inline void
lidt(void *p)
{
	__asm __volatile("lidt (%0)" : : "r" (p));
}

static __inline void
lldt(u_short sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static __inline void
ltr(u_short sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static __inline void
lcr0(u_int val)
{
	__asm __volatile("movl %0,%%cr0" : : "r" (val));
}

static __inline u_int
rcr0(void)
{
	u_int val;
	__asm __volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

static __inline u_int
rcr2(void)
{
	u_int val;
	__asm __volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

static __inline void
lcr3(u_int val)
{
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

static __inline u_int
rcr3(void)
{
	u_int val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

static __inline void
lcr4(u_int val)
{
	__asm __volatile("movl %0,%%cr4" : : "r" (val));
}

static __inline u_int
rcr4(void)
{
	u_int val;
	__asm __volatile("movl %%cr4,%0" : "=r" (val));
	return val;
}

static __inline void
tlbflush(void)
{
	u_int val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

static __inline u_int
read_eflags(void)
{
        u_int   ef;
        __asm __volatile("pushfl; popl %0" : "=r" (ef));
        return(ef);
}


static __inline void
write_eflags(u_int ef)
{
        __asm __volatile("pushl %0; popfl" : : "r" (ef));
}

static __inline void
cpuid(u_int info, u_int *eaxp, u_int *ebxp, u_int *ecxp, u_int *edxp)
{
	u_int eax, ebx, ecx, edx;
	asm volatile("cpuid" 
		: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
		: "a" (info));
	if (eaxp) *eaxp = eax;
	if (ebxp) *ebxp = ebx;
	if (ecxp) *ecxp = ecx;
	if (edxp) *edxp = edx;
}

#endif /* _X86_H_ */
