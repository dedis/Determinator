#if LAB >= 6
#ifndef _PIPE_H_
#define _PIPE_H_ 1

#include <inc/mmu.h>

#define BY2PIPE 32		// small to provoke races

struct Pipe {
	u_int p_rpos;		// read position
	u_int p_wpos;		// write position
	u_int p_page;		// pipe physical page number
	u_int p_rdpage;		// reader physical page number
	u_int p_wrpage;		// writer physical page number
	u_char p_buf[BY2PIPE];	// data buffer
};

#endif
#endif
