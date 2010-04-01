#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
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
# Anything in an #if LAB >= 999 always gets cut out by mklab.pl on export,
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
# This Makefile will automatically use the cross-compiler toolchain
# installed as 'i386-elf-*', if one exists.  If the host tools ('gcc',
# 'objdump', and so forth) compile for a 32-bit x86 ELF target, that will
# be detected as well.  If you have the right compiler toolchain installed
# using a different name, set GCCPREFIX explicitly in conf/env.mk

# try to infer the correct GCCPREFIX
ifndef GCCPREFIX
GCCPREFIX := $(shell if i386-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# try to infer the correct QEMU
ifndef QEMU
QEMU := $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in conf/env.mk?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

# try to generate a unique GDB port
GDBPORT	:= $(shell expr `id -u` % 5000 + 25000)

# Correct option to enable the GDB stub and specify its port number to qemu.
# First is for qemu versions <= 0.10, second is for later qemu versions.
QEMUPORT := -s -p $(GDBPORT)
#QEMUPORT := -gdb tcp::$(GDBPORT)

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

# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -O1 -fno-builtin -I$(TOP) -MD 
CFLAGS += -Wall -Wno-unused -Werror -gstabs -m32

# Add -fno-stack-protector if the option exists.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# Kernel versus user compiler flags
KERN_CFLAGS := $(CFLAGS) -DPIOS_KERNEL
USER_CFLAGS := $(CFLAGS) -DPIOS_USER

# Linker flags
LDFLAGS := -m elf_i386 -e start -nostdlib

KERN_LDFLAGS := $(LDFLAGS) -Ttext=0x00100000
USER_LDFLAGS := $(LDFLAGS) -Ttext=0x40000000

GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

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
	echo >>conf/lab.mk "LAB6=true"
ifneq ($(LAB), 6)
	echo >>conf/lab.mk "LAB7=true"
endif	# LAB != 6
endif	# LAB != 5
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

#if LAB >= 999			##### Begin Instructor/TA-Only Stuff #####
# Find all potentially exportable files
LAB_PATS := COPYRIGHT Makefrag *.c *.h *.S *.ld
LAB_DIRS := inc boot kern dev lib user
LAB_FILES := CODING GNUmakefile mergedep.pl grade-functions.sh .gdbinit.tmpl \
	boot/sign.pl \
	conf/env.mk \
	$(foreach lab,1 2 3 4 5 6 7,grade-lab$(lab).sh) \
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
		$(MKLABENV) $(PERL) mklab.pl $$num 0 lab$* $(LAB_FILES)
	test -d lab$*/conf || mkdir lab$*/conf
	echo >lab$*/conf/lab.mk "LAB=$*"
	echo >>lab$*/conf/lab.mk "PACKAGEDATE="`date`
export-sol%: always
	rm -rf sol$*
	num=`expr $* - $(LABADJUST)`; \
		$(MKLABENV) $(PERL) mklab.pl $$num $$num sol$* $(LAB_FILES)
	test -d sol$*/conf || mkdir sol$*/conf
	echo >sol$*/conf/lab.mk "LAB=$*"
	echo >>sol$*/conf/lab.mk "PACKAGEDATE="`date`
export-prep%: always
	rm -rf prep$*
	num=`expr $* - $(LABADJUST)`; \
		$(MKLABENV) $(PERL) mklab.pl $$num `expr $$num - 1` prep$* $(LAB_FILES)
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
build-all-sols: build-sol1 build-sol2 build-sol3 build-sol4 build-sol5 build-sol6 build-sol7
build-all-labs: build-lab1 build-lab2 build-lab3 build-lab4 build-lab5 build-lab6 build-lab7
build-all: build-all-sols build-all-labs

grade-sol%: export-sol% always
	cd sol$*; $(MAKE) grade

grade-all: grade-sol1 grade-sol2 grade-sol3 grade-sol4 grade-sol5 grade-sol6 always

#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

IMAGES = $(OBJDIR)/kern/kernel.img
QEMUOPTS = -smp 2 -hda $(OBJDIR)/kern/kernel.img -serial mon:stdio
QEMUNET = -net user -net nic,model=i82559er

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

ifdef LAB5
qemu: $(IMAGES)
	$(QEMU) $(QEMUOPTS) $(QEMUNET),macaddr=52:54:00:12:34:01 &
	$(QEMU) $(QEMUOPTS) $(QEMUNET),macaddr=52:54:00:12:34:02
else
qemu: $(IMAGES)
	$(QEMU) $(QEMUOPTS)
endif
#if LAB >= 5
# qemu -net nic -net socket,mcast=230.0.0.1:1234
#endif

qemu-nox: $(IMAGES)
	echo "*** Use Ctrl-a x to exit"
	$(QEMU) -nographic $(QEMUOPTS)

qemu-gdb: $(IMAGES) .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUPORT)

qemu-gdb-nox: $(IMAGES) .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUPORT)

which-qemu:
	@echo $(QEMU)

gdb: $(IMAGES)
	$(GDB) $(OBJDIR)/kern/kernel

gdb-boot: $(IMAGS)
	$(GDB) $(OBJDIR)/boot/bootblock.elf

# For deleting the build
clean:
	rm -rf $(OBJDIR)

realclean: clean
	rm -rf lab$(LAB).tar.gz grade-log

distclean: realclean
	rm -rf conf/gcc.mk

grade: grade-lab$(LAB).sh
	$(V)$(MAKE) clean >/dev/null 2>/dev/null
	$(MAKE) all
	sh grade-lab$(LAB).sh

tarball: realclean
	tar cf - `find . -type f | grep -v '^\.*$$' | grep -v '/CVS/' | grep -v '/\.svn/' | grep -v '/\.git/' | grep -v 'lab[0-9].*\.tar\.gz'` | gzip > lab$(LAB)-handin.tar.gz

# For test runs
run-%:
	$(V)rm -f $(OBJDIR)/kern/init.o $(IMAGES)
	$(V)$(MAKE) "DEFS=-DTEST=_binary_obj_user_$*_start -DTESTSIZE=_binary_obj_user_$*_size" $(IMAGES)
	echo "*** Use Ctrl-a x to exit"
	$(QEMU) -nographic $(QEMUOPTS)

xrun-%:
	$(V)rm -f $(OBJDIR)/kern/init.o $(IMAGES)
	$(V)$(MAKE) "DEFS=-DTEST=_binary_obj_user_$*_start -DTESTSIZE=_binary_obj_user_$*_size" $(IMAGES)
	$(QEMU) $(QEMUOPTS)

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps

always:
	@:

.PHONY: all always \
	handin tarball clean realclean clean-labsetup distclean grade labsetup

