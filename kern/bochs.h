#ifndef _BOCHS_H_
#define _BOCHS_H_ 1

static __inline void
_bochs_outw(int port, u_int16_t data)
{
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static inline void
bochs(void)
{
	_bochs_outw(0x8A00, 0x8A00);
	_bochs_outw(0x8A00, 0x8AE0);
}

#endif

