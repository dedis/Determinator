/*
 * Generic type definitions and macros used throughout PIOS.
 * Most are C/Unix standard, with a few PIOS-specific exceptions.
 *
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * See section "BSD License" in the file LICENSES for licensing terms.
 *
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_TYPES_H
#define PIOS_INC_TYPES_H

#ifndef NULL
#define NULL ((void*) 0)
#endif

// Represents true-or-false values
typedef int bool;
#define false 0
#define true 1

// Explicitly-sized versions of integer types
typedef signed char		int8_t;
typedef unsigned char		uint8_t;
typedef short			int16_t;
typedef unsigned short		uint16_t;
typedef int			int32_t;
typedef unsigned int		uint32_t;
typedef long long		int64_t;
typedef unsigned long long	uint64_t;

// Pointers and addresses are 32/64 bits long.
// long is 4 bytes when compiled as 32-bit code and 
// long is 8 bytes when compiled as 64-bit code.
// We use pointer types to represent virtual addresses,
// and [u]intptr_t to represent the numerical values of virtual addresses.
typedef long long		intptr_t;	// pointer-size signed integer
typedef unsigned long long	uintptr_t;	// pointer-size unsigned integer
typedef long long		ptrdiff_t;	// difference between pointers

// size_t is used for memory object sizes, and ssize_t is a signed analog.
typedef uintptr_t	size_t;
typedef intptr_t	ssize_t;

// intmax_t and uintmax_t represent the maximum-size integers supported.
typedef long long		intmax_t;
typedef unsigned long long	uintmax_t;

// Floating-point types matching the size at which the compiler
// actually evaluates floating-point expressions of a given type. (math.h)
typedef	double			double_t;
typedef	float			float_t;

// Unix API compatibility types
typedef int			off_t;		// file offsets and lengths
typedef int			pid_t;		// process IDs
typedef int			ino_t;		// file inode numbers
typedef int			mode_t;		// file mode flags
#if LAB >= 9

// Types not needed in PIOS, only by legacy applications
typedef int			uid_t;
typedef int			gid_t;
typedef int			dev_t;
typedef int			nlink_t;
typedef int			blksize_t;
typedef int			blkcnt_t;

typedef int64_t			time_t;
typedef int32_t			suseconds_t;
#endif

// Efficient min and max operations
#define MIN(_a, _b)						\
({								\
	typeof(_a) __a = (_a);					\
	typeof(_b) __b = (_b);					\
	__a <= __b ? __a : __b;					\
})
#define MAX(_a, _b)						\
({								\
	typeof(_a) __a = (_a);					\
	typeof(_b) __b = (_b);					\
	__a >= __b ? __a : __b;					\
})

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)						\
({								\
	uintptr_t __a = (uintptr_t) (a);				\
	(typeof(a)) (__a - __a % (n));				\
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)						\
({								\
	uintptr_t __n = (uintptr_t) (n);				\
	(typeof(a)) (ROUNDDOWN((uintptr_t) (a) + __n - 1, __n));	\
})

// Return the offset of 'member' relative to the beginning of a struct type
#define offsetof(type, member)  ((size_t) (&((type*)0)->member))

// Make the compiler think a value is getting used, even if it isn't.
#define USED(x)		(void)(x)


#if LAB >= 9
// Limits for the types defined above
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

#define SSIZE_MAX	INT_MAX


// A common way for portable to tell the endianness of a machine
// (traditionally in endian.h)
#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define PDP_ENDIAN	3412
#define BYTE_ORDER	LITTLE_ENDIAN	// x86 is little-endian


// For interoperability with GCC's compiler header files
#define _SIZE_T
#define _STDINT_H
#define _PTRDIFF_T

#endif	// LAB >= 9
#endif /* !PIOS_INC_TYPES_H */
