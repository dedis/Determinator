///LAB3
#ifndef _AOUT_H_
#define _AOUT_H_ 1

struct Aout {
	u_long	a_midmag;	/* flags<<26 | mid<<16 | magic */
	u_long	a_text;		/* text segment size */
	u_long	a_data;		/* initialized data size */
	u_long	a_bss;		/* uninitialized data size */
	u_long	a_syms;		/* symbol table size */
	u_long	a_entry;	/* entry point */
	u_long	a_trsize;	/* text relocation size */
	u_long	a_drsize;	/* data relocation size */
};

#endif
///END
