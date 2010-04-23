/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include "asm.h"

ENTRY(sqrtf)
	flds	4(%esp)
	fsqrt
	ret
END(sqrtf)
