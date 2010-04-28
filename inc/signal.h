#if LAB >= 9
#ifndef PIOS_INC_SIGNAL_H
#define PIOS_INC_SIGNAL_H

#include <types.h>

// These are the signals defined in the Single Unix Spec v3,
// with BSD-compatible numeric definitions.
#define SIGHUP		1	// Hangup
#define SIGINT		2	// Terminal interrupt signal
#define SIGQUIT		3	// Terminal quit signal
#define SIGILL		4	// Illegal instruction
#define SIGTRAP		5	// Trace/breakpoint trap
#define SIGABRT		6	// Process abort signal
#define SIGFPE		8	// Erroneous arithmetic operation
#define SIGKILL		9	// Kill
#define SIGBUS		10	// Access to undefined memory region
#define SIGSEGV		11	// Invalid memory reference
#define SIGSYS		12	// Bad system call
#define SIGPIPE		13	// Write on a pipe with no readers
#define SIGALRM		14	// Alarm clock
#define	SIGTERM		15	// Termination signal
#define SIGURG		16	// Urgent data available on socket
#define SIGSTOP		17	// Stop signal
#define SIGTSTP		18	// Terminal stop signal
#define SIGCONT		19	// Continue executing, if stopped
#define SIGCHLD		20	// Child process status change
#define SIGTTIN		21	// Background process attempting read
#define SIGTTOU		22	// Background process attempting write
#define SIGXCPU		24	// CPU time limit exceeded
#define	SIGXFZ		25	// File size limit exceeded
#define SIGPROF		27	// Profiling timer expired
#define SIGUSR1		30	// User-defined signal 1
#define SIGUSR2		31	// User-defined signal 2


typedef void (*sighandler_t)(int);

// Generic signal handler values
#define	SIG_DFL		((void (*)(int))0)
#define	SIG_IGN		((void (*)(int))1)
#define	SIG_ERR		((void (*)(int))-1)

typedef volatile int sig_atomic_t;
typedef uint32_t sigset_t;

struct sigaction {
	void		(*sa_handler)(int);	// Signal handler function
	sigset_t	sa_mask;		// Signals to block
	int		sa_flags;		// Signal action flags below
};

// Signal action flags
#define SA_ONSTACK	0x0001	// Deliver signal on alternate stack
#define SA_NOCLDSTOP	0x0002	// Do not generate SIGCHLD when children stop
#define SA_NOCLDWAIT	0x0004	// Do not wait for terminated children
#define SA_NODEFER	0x0008	// Do not block signal on signal handler entry
#define SA_RESETHAND	0x0010	// Reset signal action on signal handler entry
#define SA_RESTART	0x0020	// Restart system call on signal return
#define SA_SIGINFO	0x0040	// Pass siginfo struct to signal handler


sighandler_t signal(int sig, sighandler_t newhandler);

#endif /* ! PIOS_INC_SIGNAL_H */
#endif // LAB >= 9
