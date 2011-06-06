#if LAB >= 9

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/unistd.h>
#include <inc/syscall.h>
#include <inc/trap.h>

#include "md5.c"

int main(int argc, char **argv)
{
	// test raw instruction counting mechanism
	if (tfork(0) == 0) {
		sys_ret();

		uint32_t st[4];
		uint8_t bl[64];
		while (1) {
			MD5Transform(st, bl);
		}
	}

	int i;
	for (i = 0; i < 10; i++) {
		procstate ps;
		sys_get(SYS_REGS, 0, &ps, 0, 0, 0);
		cprintf("@ %x: trap %d icnt %d imax %d\n",
			ps.tf.rip, ps.tf.trapno, ps.icnt, ps.imax);
		if (ps.tf.trapno != T_SYSCALL && ps.tf.trapno != T_ICNT)
			break;

		ps.pff |= PFF_ICNT;
		ps.imax = 1000;
		ps.icnt = 0;

		sys_put(SYS_REGS | SYS_START, 0, &ps, 0, 0, 0);
	}
	return 0;
}

#endif	// LAB >= 9
