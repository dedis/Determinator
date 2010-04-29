// Text-mode CGA/VGA video output.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_VIDEO_H_
#define PIOS_KERN_VIDEO_H_
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <inc/x86.h>


#define MONO_BASE	0x3B4
#define MONO_BUF	0xB0000
#define CGA_BASE	0x3D4
#define CGA_BUF		0xB8000

#define CRT_ROWS	25
#define CRT_COLS	80
#define CRT_SIZE	(CRT_ROWS * CRT_COLS)


void video_init(void);
void video_putc(int c);

#if LAB >= 9
// Scrollback support
#define CRT_SAVEROWS	1024

void video_scroll(int delta);	// for console scrollback
#endif

#endif /* PIOS_KERN_VIDEO_H_ */
