/* See COPYRIGHT for copyright information. */

#ifndef _PICIRQ_H_
#define _PICIRQ_H_

#define MAX_IRQS 16
/* I/O Addresses of the two 8259A programmable interrupt controllers */
#define IO_PIC1 0x20     /* Master (IRQs 0-7) */
#define IO_PIC2 0xa0     /* Slave (IRQs 8-15) */
#define IRQ_SLAVE 0x2    /* IRQ at which slave connects to master */
#define IRQ_OFFSET 0x20  /* IRQ 0 corresponds to int IRQ_OFFSET */


#ifndef __ASSEMBLER__

#include <inc/types.h>
#include <inc/x86.h>

extern u_short irq_mask_8259A;
void pic_init (void);
void irq_setmask_8259A (u_short mask);

#endif /* !__ASSEMBLER__ */

#endif /* _PICIRQ_H_ */
