#if LAB >= 1
/*
 * Variable argument (varargs) parsing macros compliant with the C standard.
 *
 * Copyright (c) 1991, 1993 The Regents of the University of California.
 * See section "BSD License" in the file LICENSES for licensing terms.
 *
 * This code is derived from JOS and from BSD. 
 */

#ifndef PIOS_INC_STDARG_H
#define	PIOS_INC_STDARG_H

typedef char *va_list;

#define	__va_size(type) \
	(((sizeof(type) + sizeof(long) - 1) / sizeof(long)) * sizeof(long))

#define	va_start(ap, last) \
	((ap) = (va_list)&(last) + __va_size(last))

#define	va_arg(ap, type) \
	(*(type *)((ap) += __va_size(type), (ap) - __va_size(type)))

#define	va_end(ap)	((void)0)

#endif	/* !PIOS_INC_STDARG_H */
#endif	// LAB >= 1
