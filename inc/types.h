/*	$OpenBSD: types.h,v 1.12 1997/11/30 18:50:18 millert Exp $	*/
/*	$NetBSD: types.h,v 1.29 1996/11/15 22:48:25 jtc Exp $	*/

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif /* !NULL */

typedef __signed char              int8_t;
typedef unsigned char            u_int8_t;
typedef short                     int16_t;
typedef unsigned short          u_int16_t;
typedef int                       int32_t;
typedef unsigned int            u_int32_t;
/* LONGLONG */
typedef long long                 int64_t;
/* LONGLONG */
typedef unsigned long long      u_int64_t;

typedef int32_t                 register_t;

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

typedef	u_int64_t	u_quad_t;	/* quads */
typedef	int64_t		quad_t;
typedef	quad_t *	qaddr_t;

typedef u_int32_t        size_t;


#define min(_a, _b)                           \
 ({                                            \
   typeof (_a) __a = (_a);                     \
   typeof (_b) __b = (_b);                     \
   __a <= __b ? __a : __b;                     \
 })


/* Static assert, for compile-time assertion checking */
#define StaticAssert(c) switch (c) case 0: case (c):

#define offsetof(type, member)  ((size_t)(&((type *)0)->member))

#endif /* !_SYS_TYPES_H_ */
