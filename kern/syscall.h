#if LAB >= 4

#ifndef _KERN_SYSCALL_H_
#define _KERN_SYSCALL_H_

#include <inc/syscall.h>

int syscall(u_int num, u_int a1, u_int a2, u_int a3, u_int a4, u_int a5);

#endif /* !_KERN_SYSCALL_H_ */
#endif /* LAB >= 4 */
