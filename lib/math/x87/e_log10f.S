/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include "asm.h"

ENTRY(log10f)
	fldlg2
	flds	4(%esp)
	fyl2x
	ret
END(log10f)
