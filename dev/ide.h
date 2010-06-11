#if LAB >= 99
/*
 * IDE disk device driver definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Primary author: Eddie Kohler
 */

#ifndef PIOS_DEV_IDE_H
#define PIOS_DEV_IDE_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

struct iodisk;

void ide_init(void);
void ide_intr(void);
void ide_output(const struct iodisk *io);
bool ide_input(struct iodisk *io);

#endif // !PIOS_DEV_IDE_H
#endif // LAB >= 4
