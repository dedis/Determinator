#ifndef _KERN_MONITOR_H_
#define _KERN_MONITOR_H_

struct Trapframe;

// Activate the kernel monitor,
// optionally providing a trap frame indicating the current state
// (NULL if none).
void monitor(struct Trapframe *tf);

// Functions implementing monitor commands.
void mon_help(int argc, char **argv);
void mon_kerninfo(int argc, char **argv);

#endif	// not _KERN_MONITOR_H_
