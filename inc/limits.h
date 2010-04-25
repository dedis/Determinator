#ifndef PIOS_INC_LIMITS_H
#define PIOS_INC_LIMITS_H

#define CHAR_BIT	8
#define SCHAR_MAX	127
#define SCHAR_MIN	-128
#define UCHAR_MAX	255

#define SHRT_MAX	((short)0x7fff)
#define SHRT_MIN	((short)0x8000)
#define USHRT_MAX	((short)0xffff)

#define WORD_BIT	32
#define INT_MAX		(0x7fffffff)
#define INT_MIN		(0x80000000)
#define UINT_MAX	(0xffffffffu)

#define LONG_BIT	32
#define LONG_MAX	(0x7fffffffl)
#define LONG_MIN	(0x80000000l)
#define ULONG_MAX	(0xfffffffful)

#define LLONG_MAX	(0x7fffffffffffffffll)
#define LLONG_MIN	(0x8000000000000000ll)
#define ULLONG_MAX	(0xffffffffffffffffull)

#endif /* !PIOS_INC_LIMITS_H */
