/* See COPYRIGHT for copyright information. */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <inc/types.h>

#define MONO_BASE 0x3b4
#define MONO_BUF 0xb0000
#define CGA_BASE 0x3d4
#define CGA_BUF 0xb8000

#define CRT_ROWS 25
#define CRT_COLS 80
#define CRT_SIZE (CRT_ROWS * CRT_COLS)

void cninit (void);
void kbd_init (void);
void cnputc (short int c);
u_int cb_copybuf (char *buf, u_int maxlen);

#endif /* _CONSOLE_H_ */
