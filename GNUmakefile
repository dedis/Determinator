#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
#
OBJDIR := obj

ifdef GCCPREFIX
SETTINGGCCPREFIX := true
else
-include conf/gcc.mk
endif

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
DEFS	:= $(DEFS) -DLAB=$(LAB) -DSOL=$(SOL)
#
#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

TOP = .

# Cross-compiler jos toolchain
#
# This Makefile will automatically use the cross-compiler toolchain
# installed as 'i386-jos-elf-*', if one exists.  If the host tools ('gcc',
# 'objdump', and so forth) compile for a 32-bit x86 ELF target, that will
# be detected as well.  If you have the right compiler toolchain installed
# using a different name, set GCCPREFIX explicitly by doing
#
#	make 'GCCPREFIX=i386-jos-elf-' gccsetup
#

CC	:= $(GCCPREFIX)gcc -pipe
GCC_LIB := $(shell $(CC) -print-libgcc-file-name)
AS	:= $(GCCPREFIX)as
AR	:= $(GCCPREFIX)ar
LD	:= $(GCCPREFIX)ld
OBJCOPY	:= $(GCCPREFIX)objcopy
OBJDUMP	:= $(GCCPREFIX)objdump
NM	:= $(GCCPREFIX)nm

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= gtar
PERL	:= perl

# Compiler flags
# Note that -O2 is required for the boot loader to fit within 512 bytes;
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
CFLAGS	:= $(CFLAGS) $(DEFS) -O2 -fno-builtin -I$(TOP) -MD -Wall -Wno-format -ggdb

# Linker flags for user programs
ULDFLAGS := -Ttext 0x800020

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=


# Make sure that 'all' is the first target
all:


# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: %.o $(OBJDIR)/%.o $(OBJDIR)/boot/%.o $(OBJDIR)/fs/%.o \
	$(OBJDIR)/user/%.o

# Rules for building regular object files
$(OBJDIR)/%.o: %.c
	@echo + cc $<
	$(V)mkdir -p $(@D)
	$(V)$(CC) -nostdinc $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo + as $<
	$(V)mkdir -p $(@D)
	$(V)$(CC) -nostdinc $(CFLAGS) -c -o $@ $<

# try to infer the correct GCCPREFIX
conf/gcc.mk:
	@if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'GCCPREFIX=i386-jos-elf-' >conf/gcc.mk; \
	elif objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'GCCPREFIX=' >conf/gcc.mk; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'echo GCCPREFIX= >conf/gcc.mk'." 1>&2; \
	echo "***" 1>&2; exit 1; fi
	@f=`grep GCCPREFIX conf/gcc.mk | sed 's/.*=//'`; if echo $$f | grep '^[12]\.' >/dev/null 2>&1; then echo "***" 1>&2; \
	echo "*** Error: Your gcc compiler is too old." 1>&2; \
	echo "*** The labs will only work with gcc-3.0 or later, and are only" 1>&2; \
	echo "*** tested on gcc-3.3 and later." 1>&2; \
	echo "***" 1>&2; exit 1; fi

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
	rm -rf obj
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
endif	# LAB != 5
endif	# LAB != 4
endif	# LAB != 3
endif	# LAB != 2
endif	# LAB != 1

ifndef LAB5
all: $(OBJDIR)/fs/fs.img
$(OBJDIR)/fs/fs.img:
	$(V)mkdir -p $(@D)
	touch $@
endif

clean: clean-labsetup
clean-labsetup:
	rm -f conf/lab.mk
#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

# Include Makefrags for subdirectories
include boot/Makefrag
include kern/Makefrag
#if LAB >= 3
include lib/Makefrag
#endif
#if LAB >= 3
include user/Makefrag
#endif
#if LAB >= 5
include fs/Makefrag
#endif

#if LAB >= 999			##### Begin Instructor/TA-Only Stuff #####
# Find all potentially exportable files
LAB_PATS := COPYRIGHT Makefrag *.c *.h *.S
LAB_DIRS := inc boot kern lib user fs
LAB_FILES := CODING GNUmakefile .bochsrc mergedep.pl grade.sh boot/sign.pl \
	fs/lorem fs/motd fs/newmotd fs/script \
	fs/testshell.sh fs/testshell.key fs/testshell.out fs/out \
	conf/env.mk \
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
	num=`echo $$(($*-$(LABADJUST)))`; \
		$(MKLABENV) $(PERL) mklab.pl $$num 0 lab$* $(LAB_FILES)
	test -d lab$*/conf || mkdir lab$*/conf
	echo >lab$*/conf/lab.mk "LAB=$*"
	echo >>lab$*/conf/lab.mk "PACKAGEDATE="`date`
export-sol%: always
	rm -rf sol$*
	num=`echo $$(($*-$(LABADJUST)))`; \
		$(MKLABENV) $(PERL) mklab.pl $$num $$num sol$* $(LAB_FILES)
	echo >sol$*/conf/lab.mk "LAB=$*"
export-prep%: always
	rm -rf prep$*
	num=`echo $$(($*-$(LABADJUST)))`;
		$(MKLABENV) $(PERL) mklab.pl $num `expr $num - 1` prep$* $(LAB_FILES)
	echo >prep$*/conf/lab.mk "LAB=$*"

%.c: $(OBJDIR)
lab%.tar.gz: always
	$(MAKE) export-lab$*
	tar cf - lab$* | gzip > lab$*.tar.gz

build-lab%: export-lab% always
	cd lab$*; $(MAKE)
build-sol%: export-sol% always
	cd sol$*; $(MAKE)
build-prep%: export-prep% always
	cd prep$*; $(MAKE)

#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

bochs: $(OBJDIR)/kern/bochs.img $(OBJDIR)/fs/fs.img
	bochs-nogui

# For deleting the build
clean:
	rm -rf $(OBJDIR) lab$(LAB).tar.gz

grade:
	$(V)$(MAKE) clean >/dev/null 2>/dev/null
	$(MAKE) all
	sh grade.sh

#ifdef ENV_HANDIN_COPY
HANDIN_CMD = tar cf - . | gzip > ~class/handin/lab$(LAB)/$$USER/lab$(LAB).tar.gz
#else
HANDIN_CMD = tar cf - . | gzip | uuencode lab$(LAB).tar.gz | mail $(HANDIN_EMAIL)
#endif
handin: clean
	$(HANDIN_CMD)
tarball: clean
	tar cf - `ls -a | grep -v '^\.*$$' | grep -v '^lab$(LAB)\.tar\.gz'` | gzip > lab$(LAB).tar.gz

# For test runs
run-%:
	$(V)rm -f $(OBJDIR)/kern/init.o $(OBJDIR)/kern/bochs.img
	$(V)$(MAKE) "DEFS=-DTEST=_binary_obj_user_$*_start -DTESTSIZE=_binary_obj_user_$*_size" $(OBJDIR)/kern/bochs.img $(OBJDIR)/fs/fs.img
	bochs-nogui

xrun-%:
	$(V)rm -f $(OBJDIR)/kern/init.o $(OBJDIR)/kern/bochs.img
	$(V)$(MAKE) "DEFS=-DTEST=_binary_obj_user_$*_start -DTESTSIZE=_binary_obj_user_$*_size" $(OBJDIR)/kern/bochs.img $(OBJDIR)/fs/fs.img
	bochs

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps

# Create a patch from ../lab$(LAB).tar.gz.
patch patch.diff:
	@test -r ../lab$(LAB).tar.gz || (echo "***" 1>&2; \
	echo "*** Can't find '../lab$(LAB).tar.gz'.  Download it" 1>&2; \
	echo "*** into my parent directory and try again." 1>&2; \
	echo "***" 1>&2; false)
	(gzcat ../lab$(LAB).tar.gz 2>/dev/null || zcat ../lab$(LAB).tar.gz) | tar xf -
	@pkgdate=`grep PACKAGEDATE lab$(LAB)/conf/lab.mk | sed 's/.*=//'`; \
	test "$(PACKAGEDATE)" = "$$pkgdate" || (echo "***" 1>&2; \
	echo "*** The ../lab$(LAB).tar.gz tarball was created on $$pkgdate," 1>&2; \
	echo "*** but your work directory was expanded from a tarball created" 1>&2; \
	echo "*** on $(PACKAGEDATE)!  I can't tell the difference" 1>&2; \
	echo "*** between your changes and the changes between the tarballs," 1>&2; \
	echo "*** so I won't create an automatic patch." 1>&2; \
	echo "***" 1>&2; false)
	@rm -f patch.diff
	@for f in `cd lab$(LAB) && find . -type f -print`; do \
	diff -u lab$(LAB)/$$f $$f >>patch.diff || echo "*** $$f differs; appending to patch.diff" 1>&2; done
	@test -n patch.diff || echo "*** No differences found" 1>&2
	@rm -rf lab$(LAB)

apply-patch:
	@test -r patch.diff || (echo "***" 1>&2; \
	echo "*** No 'patch.diff' file found!  Did you remember to" 1>&2; \
	echo "*** run 'make patch'?" 1>&2; \
	echo "***" 1>&2; false)
	patch -p0 <patch.diff

always:

.PHONY: all always patch apply-patch handin tarball grade
