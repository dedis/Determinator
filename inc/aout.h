#if LAB >= 5
#ifndef _AOUT_H_
#define _AOUT_H_ 1

struct Aout {
	u_int  a_magic;		// flags<<26 | mid<<16 | magic
	u_int  a_text;		// text segment size
	u_int  a_data;		// initialized data size
	u_int  a_bss;		// uninitialized data size
	u_int  a_syms;		// symbol table size
	u_int  a_entry;		// entry point
	u_int  a_trsize;	// text relocation size
	u_int  a_drsize;	// data relocation size
};

#define AOUT_MAGIC	0x0064010B

#endif
#endif
