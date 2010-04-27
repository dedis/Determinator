#if LAB >= 9
// System configuration definitions
#ifndef PIOS_INC_SYSCONF_H
#define PIOS_INC_SYSCONF_H

#define	_SC_PAGESIZE	1	// Get system page size
#define	_SC_PAGE_SIZE	1
#define _SC_OPEN_MAX	2

long sysconf(int name);

#endif /* ! PIOS_INC_SYSCONF_H */
#endif // LAB >= 9
