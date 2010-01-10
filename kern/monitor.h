#ifndef PIOS_KERN_MONITOR_H
#define PIOS_KERN_MONITOR_H
#ifndef PIOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

struct trapframe;

// Activate the kernel monitor,
// optionally providing a trap frame indicating the current state
// (NULL if none).
void monitor(struct trapframe *tf);

// Functions implementing monitor commands.
int mon_help(int argc, char **argv, struct trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct trapframe *tf);
int mon_backtrace(int argc, char **argv, struct trapframe *tf);

#endif	// !PIOS_KERN_MONITOR_H
