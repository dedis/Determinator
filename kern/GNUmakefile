#if LAB >= 1
TOP = ..
include $(TOP)/ARCH

OBJDIR = obj
PROG = kernel.gdb
ifndef INITPROG
INITPROG = simple
endif
DEFS = -DKERNEL
NOLIB = yes

LOAD_DIRECTIVE = -Ttext F0100020
LDFLAGS = $(DBG) -dc -dp -static $(LOAD_DIRECTIVE) -nostdlib -e start

EXTRAINC = 
INC = -nostdinc -I$(TOP) $(EXTRAINC)
EXTRATRG = kernel bochs.img
EXTRACLN = $(OBJDIR)/kernel $(OBJDIR)/initprog.h

WARNOK = pmap.c trap.c syscall.c

## Only build files if they exist.
##
## locore.S must be first, so it's the 
## first code in the text segment!!!
##
VPATH += kern
SRCFILES += $(shell test -f ../locore.S && echo "locore.S")
SRCFILES += $(shell test -f ../init.c && echo "init.c")
SRCFILES += $(shell test -f ../console.c && echo "console.c")
SRCFILES += $(shell test -f ../pmap.c && echo "pmap.c")
SRCFILES += $(shell test -f ../printf.c && echo "printf.c")
SRCFILES += $(shell test -f ../env.c && echo "env.c")
SRCFILES += $(shell test -f ../kclock.c && echo "kclock.c")
SRCFILES += $(shell test -f ../picirq.c && echo "picirq.c")
SRCFILES += $(shell test -f ../trap.c && echo "trap.c")
SRCFILES += $(shell test -f ../lab3.S && echo "lab3.S")
SRCFILES += $(shell test -f ../sched.c && echo "sched.c")
SRCFILES += $(shell test -f ../syscall.c && echo "syscall.c")

EXTRAOBJ = 
EXTRAOBJ += $(shell test -f ../../user/kenv0/kenv0.b.s && echo "kenv0.b.o")
EXTRAOBJ += $(shell test -f ../../user/$(INITPROG)/$(INITPROG).b.s && echo $(INITPROG).b.o)



include $(TOP)/GNUmakefile.global

##  Get embedded binaries from user directory
vpath %.b.s $(patsubst %.b.o, $(TOP)/user/%, $(EXTRAOBJ))

EXTRACLN += bochs.img bochs.img~

bochs.img: kernel ../tools/bootloader/bootloader.S ../tools/bootloader/main.c
	dd if=/dev/zero	of=bochs.img~ count=10000
	dd if=$(TOP)/tools/bootloader/bootloader of=bochs.img~ conv=notrunc
	dd if=kernel of=bochs.img~ seek=1 conv=notrunc
	mv bochs.img~ bochs.img
#endif
