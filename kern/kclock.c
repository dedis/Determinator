#if LAB >= 2	// pmap.c must read NVRAM to detect how much memory we have
/* See COPYRIGHT for copyright information. */

/* The Run Time Clock and other NVRAM access functions that go with it. */
/* The run time clock is hard-wired to IRQ8. */

#include <inc/x86.h>
#if LAB >= 4
#include <inc/stdio.h>
#include <inc/isareg.h>
#include <inc/timerreg.h>
#endif

#include <kern/kclock.h>
#if LAB >= 4
#include <kern/picirq.h>
#endif


u_int
mc146818_read(void *sc, u_int reg)
{
	outb(IO_RTC, reg);
	return(inb(IO_RTC+1));
}

void
mc146818_write(void *sc, u_int reg, u_int datum)
{
	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}


#if LAB >= 4	// when we actually start needing the timers
void
kclock_init(void)
{
	/* initialize 8253 clock to interrupt 100 times/sec */
	outb(TIMER_MODE, TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);
	outb(IO_TIMER1, TIMER_DIV(100) % 256);
	outb(IO_TIMER1, TIMER_DIV(100) / 256);
	printf("	Setup timer interrupts via 8259A\n");
	irq_setmask_8259A (irq_mask_8259A & ~(1<<0));
	printf("	unmasked timer interrupt\n");
}
#endif /* LAB >= 4 */

#endif /* LAB >= 2 */
