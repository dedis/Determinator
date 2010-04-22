
#include <inc/stdio.h>
#include <inc/unistd.h>
#include <inc/syscall.h>
#include <inc/trap.h>

#undef LAB
#define LAB 5	// XXX
#include "md5.c"

int main(int argc, char **argv)
{
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
		cpustate cs;
		sys_get(SYS_REGS, 0, &cs, 0, 0, 0);
		cprintf("@ %x: trap %d icnt %d imax %d\n",
			cs.tf.tf_eip, cs.tf.tf_trapno, cs.icnt, cs.imax);
		if (cs.tf.tf_trapno != T_SYSCALL && cs.tf.tf_trapno != T_ICNT)
			break;

		cs.pff |= PFF_ICNT;
		cs.imax = 1000;
		cs.icnt = 0;

		sys_put(SYS_REGS | SYS_START, 0, &cs, 0, 0, 0);
	}
	return 0;
}

