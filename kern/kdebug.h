#if LAB >= 2
#ifndef JOS_KERN_KDEBUG_H
#define JOS_KERN_KDEBUG_H

#include <inc/types.h>

struct Eipdebuginfo {
	const char *eip_fn;		// Name of function containing EIP
					//  - Note: not null terminated!
	int eip_fnlen;			// Length of function name
	uintptr_t eip_fnaddr;		// Address of start of function
	const char *eip_file;		// Source code filename for EIP
	int eip_line;			// Source code linenumber for EIP
};

// Fills in the 'info' structure with information about the specified
// instruction address, 'eip'.  Returns 0 if information was found, and
// negative if not.
int debuginfo_eip(uintptr_t eip, struct Eipdebuginfo *info);

#endif
#endif
