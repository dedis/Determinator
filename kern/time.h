#if LAB >= 6
#ifndef PIOS_KERN_TIME_H
#define PIOS_KERN_TIME_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

void time_init(void);
void time_tick(void); 
unsigned int time_msec(void);

#endif /* PIOS_KERN_TIME_H */
#endif
