// Convenience macros for using GCC-specific compiler features
// that tend to be very useful in OS programming.
#ifndef PIOS_INC_GCC_H
#define PIOS_INC_GCC_H


// Use this to align a variable or struct member at a given boundary.
#define gcc_aligned(mult)	__attribute__((aligned (mult)))

// Use this to _prevent_ GCC from naturally aligning a structure member.
#define gcc_packed		__attribute__((packed))

// Functions declared always_inline will always be inlined,
// even in code compiled without optimization.  In contrast,
// the regular "inline" attribute is just a hint and may not be observed.
#define gcc_inline		__inline __attribute__((always_inline))

// Conversely, this ensures that GCC does NOT inline a function.
#define gcc_noinline		__attribute__((noinline))

// Functions declared noreturn are not expected to return
// (and GCC complains if you write a noreturn function that does).
#define gcc_noreturn		__attribute__((noreturn))

// Functions declared pure have no non-stack writes or other side-effects.
// Those declared pure2 do not even read anything but their direct arguments.
#define gcc_pure		__attribute__((pure))
#define gcc_pure2		__attribute__((const))

#if SOL >= 4

// Surround function declarations in system C header files with these
// so the C++ compiler will know that they're C and not C++ functions.
#ifdef __cplusplus
#define __BEGIN_DECLS	extern "C" {
#define __END_DECLS	}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

// Declare 'alias' to be a weak reference to symbol 'sym'
#define	__weak_reference(sym,alias)	\
	__asm__(".weak " #alias);	\
	__asm__(".equ "  #alias ", " #sym)

#endif	// SOL >= 4

#endif	// PIOS_INC_GCC_H
