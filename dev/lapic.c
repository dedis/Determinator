#if LAB >= 2
// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.
// Adapted from xv6.

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/cpu.h>

#include <dev/timer.h>
#include <dev/lapic.h>


// Frequency at which we want our local APICs to produce interrupts,
// which are used for context switching.
// Must be at least 19Hz in order to keep the system type up-to-date.
#define HZ		25


volatile uint32_t *lapic;  // Initialized in mp.c


static void
lapicw(int index, int value)
{
	lapic[index] = value;
	lapic[ID];  // wait for write to finish, by reading
}

void
lapic_init()
{
	if (!lapic) 
		return;

	// Enable local APIC; set spurious interrupt vector.
	lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.  
	// First initialize it to the maximum value for calibration.
	lapicw(TDCR, X1);
	lapicw(TIMER, PERIODIC | T_LTIMER);
	lapicw(TICR, ~(uint32_t)0); 

	// Use the 8253 Programmable Interval Timer (PIT),
	// which has a standard clock frequency,
	// to determine this processor's exact bus frequency.
	uint64_t tb = timer_read() + 1;
	while (timer_read() < tb);	// wait until start of a PIT tick
	uint32_t lb = lapic[TCCR];	// read base count from lapic
	while (timer_read() < tb+(TIMER_FREQ+HZ/2)/HZ); // wait 1/HZ sec
	uint32_t le = lapic[TCCR];	// read final count from lapic
	assert(le < lb);
	uint32_t ltot = lb - le;	// total # lapic ticks per 1/HZ tick

	long long lhz = ltot * HZ;
	cprintf("CPU%d: %llu.%09lluHz\n", cpu_cur()->id,
		lhz / 1000000000, lhz % 1000000000);
	lapicw(TICR, ltot);

	// Disable logical interrupt lines.
	lapicw(LINT0, MASKED);
	lapicw(LINT1, MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapic[VER]>>16) & 0xFF) >= 4)
		lapicw(PCINT, MASKED);

	// Map other interrupts to appropriate vectors.
	lapicw(ERROR, T_LERROR);
	lapicw(PCINT, T_PERFCTR);

	// Set up to lowest-priority, "anycast" interrupts
	lapicw(LDR, 0xff << 24);	// Accept all interrupts
	lapicw(DFR, 0xf << 28);		// Flat model
	lapicw(TPR, 0x00);		// Task priority 0, no intrs masked

	// Clear error status register (requires back-to-back writes).
	lapicw(ESR, 0);
	lapicw(ESR, 0);

	// Ack any outstanding interrupts.
	lapicw(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	lapicw(ICRHI, 0);
	lapicw(ICRLO, BCAST | INIT | LEVEL);
	while(lapic[ICRLO] & DELIVS)
		;

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(TPR, 0);
}

// Acknowledge interrupt.
void
lapic_eoi(void)
{
	if (lapic)
		lapicw(EOI, 0);
}

void lapic_errintr(void)
{
	lapic_eoi();	// Acknowledge interrupt
	lapicw(ESR, 0);	// Trigger update of ESR by writing anything
	warn("CPU%d LAPIC error: ESR %x", cpu_cur()->id, lapic[ESR]);
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
}


#define IO_RTC  0x70

// Start additional processor running bootstrap code at addr.
// See Appendix B of MultiProcessor Specification.
void
lapic_startcpu(uint8_t apicid, uint32_t addr)
{
	int i;
	uint16_t *wrv;

	// "The BSP must initialize CMOS shutdown code to 0AH
	// and the warm reset vector (DWORD based at 40:67) to point at
	// the AP startup code prior to the [universal startup algorithm]."
	outb(IO_RTC, 0xF);  // offset 0xF is shutdown code
	outb(IO_RTC+1, 0x0A);
	wrv = (uint16_t*)(0x40<<4 | 0x67);  // Warm reset vector
	wrv[0] = 0;
	wrv[1] = addr >> 4;

	// "Universal startup algorithm."
	// Send INIT (level-triggered) interrupt to reset other CPU.
	lapicw(ICRHI, apicid<<24);
	lapicw(ICRLO, INIT | LEVEL | ASSERT);
	microdelay(200);
	lapicw(ICRLO, INIT | LEVEL);
	microdelay(100);    // should be 10ms, but too slow in Bochs!

	// Send startup IPI (twice!) to enter bootstrap code.
	// Regular hardware is supposed to only accept a STARTUP
	// when it is in the halted state due to an INIT.  So the second
	// should be ignored, but it is part of the official Intel algorithm.
	// Bochs complains about the second one.  Too bad for Bochs.
	for(i = 0; i < 2; i++){
		lapicw(ICRHI, apicid<<24);
		lapicw(ICRLO, STARTUP | (addr>>12));
		microdelay(200);
	}
}

#endif	// LAB >= 2
