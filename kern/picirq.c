///BEGIN 3
/*
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */

#include <kern/picirq.h>

/* Keep copy of current IRQ mask */
u_short irq_mask_8259A;

/* Initialize the 8259A interrupt controllers. */
void
pic_init (void)
{
  /*
   * ICW1:  0001g0hi
   *    g:  0 = edge triggering, 1 = level triggering
   *    h:  0 = cascaded PICs, 1 = master only
   *    i:  0 = no ICW4, 1 = ICW4 required
   */
  outb(IO_PIC1, 0x11);
  /*
   * ICW2:  Vector offset
   */
  outb(IO_PIC1+1, IRQ_OFFSET);
  /*
   * ICW3:  bit mask of IR lines connected to slave PICs (master PIC),
   *        3-bit No if IR line at which slave connects to master (slave PIC).
   */
  outb(IO_PIC1+1, 1<<IRQ_SLAVE);
  /*
   * ICW4:  000nbmap
   *    n:  1 = special fully nested mode
   *    b:  1 = buffered mode
   *    m:  0 = slave PIC, 1 = maser PIC (ignored when b is 0, as the master/
   *                                      slave role can be hardwired).
   *    a:  1 = Automatic EOI mode
   *    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
   */
  outb(IO_PIC1+1, 0x3);

  /*
   * OCW1:  interrupt mask (start with all masked).
   */
  outb(IO_PIC1+1, 0xff);
  /*
   * OCW3:  0ef01prs
   *   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
   *    p:  0 = no polling, 1 = polling mode
   *   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
   */
  outb(IO_PIC1, 0x68);             /* clear specific mask */
  outb(IO_PIC1, 0x0a);             /* read IRR by default */

  outb(IO_PIC2, 0x11);               /* ICW1 */
  outb(IO_PIC2+1, IRQ_OFFSET + 8);   /* ICW2 */
  outb(IO_PIC2+1, IRQ_SLAVE);        /* ICW3 */
  outb(IO_PIC2+1, 0x3);              /* ICW4 */

  outb(IO_PIC2+1, 0xff);             /* OCW1 */
  outb(IO_PIC2, 0x68);               /* OCW3 */
  outb(IO_PIC2, 0x0a);               /* OCW3 */

  irq_mask_8259A = 0xffff;
}

void
irq_setmask_8259A (u_short mask)
{
  irq_mask_8259A = mask;
  outb(IO_PIC1+1, (char)mask);
  outb(IO_PIC2+1, (char)(mask >> 8));
}
///END
