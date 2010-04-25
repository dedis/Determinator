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

// Pointers and addresses are 32 bits long.
// We use pointer types to represent virtual addresses,
// and [u]intptr_t to represent the numerical values of virtual addresses.
typedef int32_t			intptr_t;	// pointer-size signed integer
typedef uint32_t		uintptr_t;	// pointer-size unsigned integer
typedef int32_t			ptrdiff_t;	// difference between two pointers

// size_t is used for memory object sizes, and ssize_t is a signed analog.
typedef uint32_t		size_t;
typedef int32_t			ssize_t;

// intmax_t and uintmax_t represent the maximum-size integers supported.
typedef long long		intmax_t;
typedef unsigned long long	uintmax_t;

// Floating-point types matching the size at which the compiler
// actually evaluates floating-point expressions of a given type. (math.h)
typedef	double			double_t;
typedef	float			float_t;

// off_t is used for file offsets and lengths.
typedef int32_t off_t;

// Random Unix API compatibility types
typedef int pid_t;
typedef int dev_t;
typedef int ino_t;
typedef int mode_t;
typedef int nlink_t;

typedef int64_t time_t;
typedef int32_t suseconds_t;

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
	uint32_t __a = (uint32_t) (a);				\
	(typeof(a)) (__a - __a % (n));				\
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)						\
({								\
	uint32_t __n = (uint32_t) (n);				\
	(typeof(a)) (ROUNDDOWN((uint32_t) (a) + __n - 1, __n));	\
})

// Return the offset of 'member' relative to the beginning of a struct type
#define offsetof(type, member)  ((size_t) (&((type*)0)->member))

// Make the compiler think a value is getting used, even if it isn't.
#define USED(x)		(void)(x)


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


#endif /* !PIOS_INC_TYPES_H */
