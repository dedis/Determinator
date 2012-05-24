/*
 * Based on code written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include "asm.h"

ENTRY(scalbnl)
	movl	%edi,-4(%rsp)
	fildl	-4(%rsp)
	fldt	8(%rsp)
	fscale
	fstp	%st(1)
	ret
END(scalbnl)

.globl	CNAME(ldexpl)
.set	CNAME(ldexpl),CNAME(scalbnl)
