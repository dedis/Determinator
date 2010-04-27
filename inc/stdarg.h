#if LAB >= 1
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
