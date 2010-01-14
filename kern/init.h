#if LAB >= 1
// Physical memory management definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_MAIN_H
#define PIOS_KERN_MAIN_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif


// Page containing cpu struct and kernel stack for bootstrap CPU.
// Defined in entry.S.
extern struct cpu bootcpu;


// Called from entry.S, only on the bootstrap processor.
// PIOS conventional: all '_init' functions get called
// only at bootstrap and only on the bootstrap processor.
void init(void);

// Called after bootstrap initialization on ALL processors,
// to initialize each CPU's private state and start it doing work.
void startup(void);

// Called when there is no more work left to do in the system.
// The grading scripts trap calls to this to know when to stop.
void done(void);


#endif /* !PIOS_KERN_MAIN_H */
#endif // LAB >= 1
