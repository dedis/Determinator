#if LAB >= 4
// 8253/8254/82C54 Programmable Interval Timer (PIT) driver code.
// See COPYRIGHT for copyright information.

#include <inc/x86.h>

#include <dev/timer.h>
#include <dev/pic.h>

void
timer_init(unsigned hz)
{
	// initialize 8253 clock to interrupt 100 times/sec
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	outb(IO_TIMER1, TIMER_DIV(hz) % 256);
	outb(IO_TIMER1, TIMER_DIV(hz) / 256);
	cprintf("	Setup timer interrupts via 8259A\n");

	// Enable the timer interrupt via the interrupt controller (PIC)
	pic_setmask(irq_mask_8259A & ~(1<<0));
	cprintf("	unmasked timer interrupt\n");
}

#endif /* LAB >= 4 */
