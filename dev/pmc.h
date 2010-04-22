// Definitions for using x86 performance monitoring counters
#ifndef PIOS_KERN_PMC_H_
#define PIOS_KERN_PMC_H_
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/types.h>


extern bool pmc_avail;
extern void (*pmc_set)(int64_t maxcnt);
extern int64_t (*pmc_get)(int64_t maxcnt);


void pmc_init(void);

#endif /* PIOS_KERN_PMC_H_ */
