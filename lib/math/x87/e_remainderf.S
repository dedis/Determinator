/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include "asm.h"

ENTRY(remainderf)
	flds	8(%esp)
	flds	4(%esp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	ret
END(remainderf)
