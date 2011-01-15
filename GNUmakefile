#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
#
# Copyright (C) 2003 Massachusetts Institute of Technology 
# See section "MIT License" in the file LICENSES for licensing terms.
# Primary authors: Bryan Ford, Eddie Kohler, Austin Clemens
#
OBJDIR := obj

ifdef LAB
SETTINGLAB := true
else
-include conf/lab.mk
endif

-include conf/env.mk

ifndef SOL
SOL := 0
endif
ifndef LABADJUST
LABADJUST := 0
endif

#if LAB >= 999			##### Begin Instructor/TA-Only Stuff #####
#
# Anything in an #if LAB >= 999 always gets cut out by misc/mklab.pl on export,
# and thus is for the internal instructor/TA project tree only.
# Students never see this.
# 
# To build within the instructor/TA tree
# (instead of exporting a student tree and then building),
# invoke 'make' as follows:
#
#	make LAB=# SOL=# labsetup
#       make
#
# substituting the appropriate LAB# and SOL# to control the build.
#
# Pass the LAB and SOL values to the C compiler.
LABDEFS = -DLAB=$(LAB) -DSOL=$(SOL)
#
#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

TOP = .

# Cross-compiler toolchain
#
# This Makefile will automatically use a cross-compiler toolchain installed
# as 'pios-*' or 'i386-elf-*', if one exists.  If the host tools ('gcc',
# 'objdump', and so forth) compile for a 32-bit x86 ELF target, that will
# be detected as well.  If you have the right compiler toolchain installed
# using a different name, set GCCPREFIX explicitly in conf/env.mk

# try to infer the correct GCCPREFIX
ifndef GCCPREFIX
GCCPREFIX := $(shell sh misc/gccprefix.sh)
endif

# try to infer the correct QEMU
ifndef QEMU
QEMU := $(shell sh misc/which-qemu.sh)
endif

# try to generate unique GDB and network port numbers
GDBPORT	:= $(shell expr `id -u` % 5000 + 25000)
NETPORT := $(shell expr `id -u` % 5000 + 30000)

# Correct option to enable the GDB stub and specify its port number to qemu.
# First is for qemu versions <= 0.10, second is for later qemu versions.
# QEMUPORT := -s -p $(GDBPORT)
QEMUPORT := -gdb tcp::$(GDBPORT)

CC	:= $(GCCPREFIX)gcc -pipe
AS	:= $(GCCPREFIX)as
AR	:= $(GCCPREFIX)ar
LD	:= $(GCCPREFIX)ld
OBJCOPY	:= $(GCCPREFIX)objcopy
OBJDUMP	:= $(GCCPREFIX)objdump
NM	:= $(GCCPREFIX)nm
GDB	:= $(GCCPREFIX)gdb

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= gtar
PERL	:= perl

# If we're not using the special "PIOS edition" of GCC,
# reconfigure the host OS's compiler for our purposes.
ifneq ($(GCCPREFIX),pios-)
CFLAGS += -nostdinc -m32
LDFLAGS += -nostdlib -m elf_i386
USER_LDFLAGS += -e start -Ttext=0x40000100
endif

# Where does GCC have its libgcc.a and libgcc's include directory?
GCCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))

# x86-64 systems may put libgcc's include directory with the 64-bit compiler
GCCALTDIR := $(dir $(shell $(CC) -print-libgcc-file-name))

# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS += $(DEFS) $(LABDEFS) -fno-builtin -I$(TOP) -I$(TOP)/inc \
		-I$(GCCDIR)/include -I$(GCCALTDIR)/include \
		-MD -Wall -Wno-unused -Werror -gstabs
ifdef LAB9
# Optimize by default only in the research (Determinator) system.
CFLAGS += -O2
endif

# Add -fno-stack-protector if the option exists.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 \
		&& echo -fno-stack-protector)

LDFLAGS += -L$(OBJDIR)/lib -L$(GCCDIR)

# Compiler flags that differ for kernel versus user-level code.
KERN_CFLAGS += $(CFLAGS) -DPIOS_KERNEL
KERN_LDFLAGS += $(LDFLAGS) -nostdlib -Ttext=0x00100000 -L$(GCCDIR)
KERN_LDLIBS += $(LDLIBS) -lgcc

USER_CFLAGS += $(CFLAGS) -DPIOS_USER
USER_LDFLAGS += $(LDFLAGS)
USER_LDINIT += $(OBJDIR)/lib/crt0.o
USER_LDDEPS += $(USER_LDINIT) $(OBJDIR)/lib/libc.a
USER_LDLIBS += $(LDLIBS) -lc -lgcc

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=

# Make sure that 'all' is the first target
all:

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: %.o $(OBJDIR)/boot/%.o $(OBJDIR)/kern/%.o \
	   $(OBJDIR)/lib/%.o $(OBJDIR)/fs/%.o $(OBJDIR)/net/%.o \
	   $(OBJDIR)/user/%.o



#if LAB >= 999			##### Begin Instructor/TA-Only Stuff #####

# Use a fake target to make sure both LAB and SOL are defined.
all inc/types.h: checklab
checklab:
ifdef SETTINGLAB
	@echo "run: make LAB=N SOL=N labsetup, then just run make"
	@false
endif
	@echo "Building LAB=$(LAB) SOL=$(SOL)"

labsetup:
	rm -rf obj labsetup
	test -d conf || mkdir conf
	echo >conf/lab.mk "LAB=$(LAB)"
	echo >>conf/lab.mk "SOL=$(SOL)"
	echo >>conf/lab.mk "LAB1=true"
ifneq ($(LAB), 1)
	echo >>conf/lab.mk "LAB2=true"
ifneq ($(LAB), 2)
	echo >>conf/lab.mk "LAB3=true"
ifneq ($(LAB), 3)
	echo >>conf/lab.mk "LAB4=true"
ifneq ($(LAB), 4)
	echo >>conf/lab.mk "LAB5=true"
ifneq ($(LAB), 5)
	echo >>conf/lab.mk "LAB9=true"
endif	# LAB != 4
endif	# LAB != 4
endif	# LAB != 3
endif	# LAB != 2
endif	# LAB != 1

ifndef LAB5
all: $(OBJDIR)/fs/fs.img
$(OBJDIR)/fs/fs.img:
	$(V)mkdir -p $(@D)
	$(V)touch $@
endif

distclean: clean-labsetup
clean-labsetup:
	rm -f conf/lab.mk
#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

# Include Makefrags for subdirectories
include boot/Makefrag
include kern/Makefrag
#if LAB >= 2
include lib/Makefrag
#endif
#if LAB >= 3
include user/Makefrag
#endif

ifdef LAB9
# Phoenix benchmark
#include phoenix/phenix-2.0.0/src/Makefrag
endif

#if LAB >= 999			##### Begin Instructor/TA-Only Stuff #####
# Find all potentially exportable files
LAB_PATS := COPYRIGHT Makefrag *.c *.h *.S *.ld
LAB_DIRS := inc boot kern dev lib user
LAB_FILES := CODING GNUmakefile misc/mergedep.pl misc/grade-functions.sh \
	misc/gccprefix.sh misc/which-qemu.sh \
	.gdbinit.tmpl boot/sign.pl conf/env.mk \
	$(foreach lab,1 2 3 4 5,misc/grade-lab$(lab).sh) \
	$(wildcard $(foreach dir,$(LAB_DIRS),$(addprefix $(dir)/,$(LAB_PATS))))

# Fake targets to export the student lab handout and solution trees.
# It's important that these aren't just called 'lab%' and 'sol%',
# because that causes 'lab%' to match 'kern/lab3.S' and delete it - argh!
#
# - lab% is the tree we hand out to students when each lab is assigned;
#	subsequent lab handouts include more lab code but no solution code.
#
# - sol% is the solution tree we hand out when a lab is fully graded;
#	it includes all lab code and solution code up to that lab number %.
#
# - prep% is the tree we use internally to get something equivalent to
#	what the students are supposed to start with just before lab %:
#	it contains all lab code up to % and all solution code up to %-1.
#
# In general, only sol% and prep% trees are supposed to compile directly.
#
export-lab%: always
	rm -rf lab$*
	num=`expr $* - $(LABADJUST)`; \
		$(MKLABENV) $(PERL) misc/mklab.pl $$num 0 lab$* $(LAB_FILES)
	test -d lab$*/conf || mkdir lab$*/conf
	echo >lab$*/conf/lab.mk "LAB=$*"
	echo >>lab$*/conf/lab.mk "PACKAGEDATE="`date`
export-sol%: always
	rm -rf sol$*
	num=`expr $* - $(LABADJUST)`; \
		$(MKLABENV) $(PERL) misc/mklab.pl $$num $$num sol$* $(LAB_FILES)
	test -d sol$*/conf || mkdir sol$*/conf
	echo >sol$*/conf/lab.mk "LAB=$*"
	echo >>sol$*/conf/lab.mk "PACKAGEDATE="`date`
export-prep%: always
	rm -rf prep$*
	num=`expr $* - $(LABADJUST)`; \
		$(MKLABENV) $(PERL) misc/mklab.pl $$num `expr $$num - 1` prep$* $(LAB_FILES)
	test -d prep$*/conf || mkdir prep$*/conf
	echo >prep$*/conf/lab.mk "LAB=$*"
	echo >>prep$*/conf/lab.mk "PACKAGEDATE="`date`
	tar czf prep$*.tar.gz prep$*
update-prep%: always
	test -d prep$* -a -f prep$*.tar.gz
	mv prep$* curprep$*
	tar xzf prep$*.tar.gz
	-diff -ru --exclude=obj prep$* curprep$* > prep$*.patch
	rm -rf prep$* curprep$*
	$(MAKE) export-prep$*
	cd prep$* && patch -p1 < ../prep$*.patch

lab%.tar.gz: always
	$(MAKE) export-lab$*
	tar cf - lab$* | gzip > lab$*.tar.gz

build-lab%: export-lab% always
	cd lab$*; $(MAKE)
build-sol%: export-sol% always
	cd sol$*; $(MAKE)
build-prep%: export-prep% always
	cd prep$*; $(MAKE)
build-all-sols: build-sol1 build-sol2 build-sol3 build-sol4 build-sol5
build-all-labs: build-lab1 build-lab2 build-lab3 build-lab4 build-lab5
build-all: build-all-sols build-all-labs

grade-sol%: export-sol% always
	cd sol$*; $(MAKE) grade

grade-all: grade-sol1 grade-sol2 grade-sol3 grade-sol4 grade-sol5 grade-sol6 always

#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

NCPUS = 2
ifdef LAB9
# Local hack: use more CPUs by default when running on our main test server.
#NCPUS := $(shell if test `uname -n` = "korz"; then echo 12; else echo 2; fi)
endif
IMAGES = $(OBJDIR)/kern/kernel.img
QEMUOPTS = -smp $(NCPUS) -hda $(OBJDIR)/kern/kernel.img -serial mon:stdio \
		-k en-us -m 1100M
#QEMUNET = -net socket,mcast=230.0.0.1:$(NETPORT) -net nic,model=i82559er
QEMUNET1 = -net nic,model=i82559er,macaddr=52:54:00:12:34:01 \
		-net socket,connect=:$(NETPORT) -net dump,file=node1.dump
QEMUNET2 = -net nic,model=i82559er,macaddr=52:54:00:12:34:02 \
		-net socket,listen=:$(NETPORT) -net dump,file=node2.dump

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

ifneq ($(LAB),5)
# Launch QEMU and run PIOS. Labs 1-4 need only one instance of QEMU.
qemu: $(IMAGES)
	$(QEMU) $(QEMUOPTS)
else
# Lab 5 is a distributed system, so we need (at least) two instances.
# Only one instance gets input from the terminal, to avoid confusion.
qemu: $(IMAGES)
	@rm -f node?.dump
	$(QEMU) $(QEMUOPTS) $(QEMUNET2) </dev/null | sed -e 's/^/2: /g' &
	@sleep 1
	$(QEMU) $(QEMUOPTS) $(QEMUNET1)
endif

# Launch QEMU without a virtual VGA display (use when X is unavailable).
qemu-nox: $(IMAGES)
	echo "*** Use Ctrl-a x to exit"
	$(QEMU) -nographic $(QEMUOPTS)

ifneq ($(LAB),5)
# Launch QEMU for debugging. Labs 1-4 need only one instance of QEMU.
qemu-gdb: $(IMAGES) .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUPORT)
else
# Launch QEMU for debugging the 2-node distributed system in Lab 5.
qemu-gdb: $(IMAGES) .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	@rm -f node?.dump
	$(QEMU) $(QEMUOPTS) $(QEMUNET2) </dev/null | sed -e 's/^/2: /g' &
	@sleep 1
	$(QEMU) $(QEMUOPTS) $(QEMUNET1) -S $(QEMUPORT)
endif

# Launch QEMU for debugging, without a virtual VGA display.
qemu-gdb-nox: $(IMAGES) .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUPORT)

# For deleting the build
clean:
	rm -rf $(OBJDIR)

realclean: clean
	rm -rf lab$(LAB).tar.gz grade-out

distclean: realclean
	rm -rf conf/gcc.mk

grade: misc/grade-lab$(LAB).sh
	$(V)$(MAKE) clean >/dev/null 2>/dev/null
	$(MAKE) all
	sh misc/grade-lab$(LAB).sh

tarball: realclean
	tar cf - `find . -type f | grep -v '^\.*$$' | grep -v '/CVS/' | grep -v '/\.svn/' | grep -v '/\.git/' | grep -v 'lab[0-9].*\.tar\.gz'` | gzip > lab$(LAB)-handin.tar.gz

ifdef LAB9
# For test runs.  (XXX probably doesn't work but would be useful; get working!)
run-%:
	$(V)rm -f $(OBJDIR)/kern/init.o $(IMAGES)
	$(V)$(MAKE) "DEFS=-DTEST=_binary_obj_user_$*_start -DTESTSIZE=_binary_obj_user_$*_size" $(IMAGES)
	echo "*** Use Ctrl-a x to exit"
	$(QEMU) -nographic $(QEMUOPTS)

xrun-%:
	$(V)rm -f $(OBJDIR)/kern/init.o $(IMAGES)
	$(V)$(MAKE) "DEFS=-DTEST=_binary_obj_user_$*_start -DTESTSIZE=_binary_obj_user_$*_size" $(IMAGES)
	$(QEMU) $(QEMUOPTS)
endif

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'misc/mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) misc/mergedep.pl $@ $^

-include $(OBJDIR)/.deps

always:
	@:

.PHONY: all always \
	handin tarball clean realclean clean-labsetup distclean grade labsetup

