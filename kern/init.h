#if LAB >= 1
// Physical memory management definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_INIT_H
#define PIOS_KERN_INIT_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif


// Called on each processor to initialize the kernel.
void init(void);

// Called when there is no more work left to do in the system.
// The grading scripts trap calls to this to know when to stop.
void done(void);


#endif /* !PIOS_KERN_INIT_H */
#endif // LAB >= 1
