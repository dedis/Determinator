// Definitions for using x86 performance monitoring counters
#ifndef PIOS_KERN_PMC_H_
#define PIOS_KERN_PMC_H_
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/types.h>


void pmc_init(void);

#endif /* PIOS_KERN_PMC_H_ */
