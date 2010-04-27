
#include <sysconf.h>
#include <assert.h>
#include <file.h>
#include <mmu.h>

long sysconf(int name)
{
	switch (name) {
	case _SC_PAGESIZE:
		return PAGESIZE;
	case _SC_OPEN_MAX:
		return OPEN_MAX;
	default:
		panic("unknown sysconf value %d\n", name);
	};
}

