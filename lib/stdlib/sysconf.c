
#include <sysconf.h>
#include <assert.h>
#include <file.h>
#include <mmu.h>

#define PIOS_KERNEL
#include <dev/lapic.h>	// XXX for HZ

long sysconf(int name)
{
	switch (name) {
	case _SC_PAGESIZE:
		return PAGESIZE;
	case _SC_OPEN_MAX:
		return OPEN_MAX;
	case _SC_CLK_TCK:
		return HZ;
	default:
		panic("unknown sysconf value %d\n", name);
	};
}

