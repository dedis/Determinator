#if LAB >= 9
#ifndef PIOS_INC_SETJMP_H
#define	PIOS_INC_SETJMP_H

#include <types.h>
#include <cdefs.h>

typedef uint32_t jmp_buf[6];

int setjmp(jmp_buf buf);
void gcc_noreturn longjmp(jmp_buf buf, int ret);

#endif	/* !PIOS_INC_SETJMP_H */
#endif	// LAB >= 9
