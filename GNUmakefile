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
-include .gccsetup
endif

ifdef LAB
SETTINGLAB := true
else
-include .labsetup
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
#endif // LAB >= 999		##### Begin Instructor/TA-Only Stuff #####

LABADJUST := 0
-include .classconfig

TOP = .

# Cross-compiler jos toolchain
# Users of 32-bit x86 ELF platforms can leave GCCPREFIX empty,
# so that they can use the native compilers for their systems.
# People on other systems (e.g., x86-64 or Mac OS X) will need
# to install an i386-jos-elf tool chain and set GCCPREFIX
# by doing
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
.SECONDARY:

# Rules for building regular object files
$(OBJDIR)/%.o: %.c
	@echo cc $<
	@mkdir -p $(@D)
	@$(CC) -nostdinc $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@mkdir -p $(@D)
	@echo as $<
	@$(CC) -nostdinc $(CFLAGS) -c -o $@ $<


# Setup a prefix for the GCC tools if we're cross-compiling.
gccsetup:
	echo >.gccsetup "GCCPREFIX=$(GCCPREFIX)"


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
	echo >.labsetup "LAB=$(LAB)"
	echo >>.labsetup "SOL=$(SOL)"
	echo >>.labsetup "LAB1=true"
ifneq ($(LAB), 1)
	echo >>.labsetup "LAB2=true"
ifneq ($(LAB), 2)
	echo >>.labsetup "LAB3=true"
ifneq ($(LAB), 3)
	echo >>.labsetup "LAB4=true"
ifneq ($(LAB), 4)
	echo >>.labsetup "LAB5=true"
ifneq ($(LAB), 5)
	echo >>.labsetup "LAB6=true"
endif	# LAB != 5
endif	# LAB != 4
endif	# LAB != 3
endif	# LAB != 2
endif	# LAB != 1

ifndef LAB5
all: $(OBJDIR)/fs/fs.img
$(OBJDIR)/fs/fs.img:
	@mkdir -p $(@D)
	touch $@
endif

clean: clean-labsetup
clean-labsetup:
	rm -f .labsetup
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
	$(wildcard $(foreach dir,$(LAB_DIRS),$(addprefix $(dir)/,$(LAB_PATS))))

BIOS_FILES := bios/BIOS-bochs-latest bios/VGABIOS-lgpl-latest

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
export-lab%: $(BIOS_FILES)
	rm -rf lab$*
	num=`echo $$(($*-$(LABADJUST)))`; \
		$(MKLABENV) $(PERL) mklab.pl $$num 0 lab$* $(LAB_FILES)
	echo >lab$*/.labsetup "LAB=$*"
export-sol%: $(BIOS_FILES)
	rm -rf sol$*
	num=`echo $$(($*-$(LABADJUST)))`; \
		$(MKLABENV) $(PERL) mklab.pl $$num $$num sol$* $(LAB_FILES)
	echo >sol$*/.labsetup "LAB=$*"
export-prep%: $(BIOS_FILES)
	rm -rf prep$*
	num=`echo $$(($*-$(LABADJUST)))`;
		$(MKLABENV) $(PERL) mklab.pl $num `expr $num - 1` prep$* $(LAB_FILES)
	echo >prep$*/.labsetup "LAB=$*"

%.c: $(OBJDIR)
lab%.tar.gz: $(BIOS_FILES)
	gmake export-lab$*
	tar cf - lab$* | gzip > lab$*.tar.gz

build-lab%: export-lab%
	cd lab$*; gmake
build-sol%: export-sol%
	cd sol$*; gmake
build-prep%: export-prep%
	cd prep$*; gmake

# Distribute the BIOS images Bochs needs with the lab trees
# in order to avoid absolute pathname dependencies in .bochsrc.
bios:
	mkdir $@
bios/%: /usr/local/share/bochs/% bios
	cp $< $@
all: $(BIOS_FILES)
#endif // LAB >= 999		##### End Instructor/TA-Only Stuff #####

bochs: $(OBJDIR)/kern/bochs.img $(OBJDIR)/fs/fs.img
	bochs-nogui

# For deleting the build
clean:
	rm -rf $(OBJDIR)

grade:
	@gmake clean >/dev/null 2>/dev/null
	gmake all
	sh grade.sh

#ifdef ENV_HANDIN_COPY
HANDIN_CMD = tar cf - . | gzip > ~class/handin/lab2/$$USER/lab2.tar.gz
#else
HANDIN_CMD = tar cf - . | gzip | uuencode lab$(LAB).tar.gz | mail 6.828-handin@pdos.lcs.mit.edu
#endif
handin: clean
	$(HANDIN_CMD)

# For test runs
run-%:
	@rm -f $(OBJDIR)/kern/init.o $(OBJDIR)/kern/bochs.img
	@gmake "DEFS=-DTEST=binary_user_$*_start -DTESTSIZE=binary_user_$*_size" $(OBJDIR)/kern/bochs.img $(OBJDIR)/fs/fs.img
	bochs-nogui

xrun-%:
	@rm -f $(OBJDIR)/kern/init.o $(OBJDIR)/kern/bochs.img
	@gmake "DEFS=-DTEST=binary_user_$*_start -DTESTSIZE=binary_user_$*_size" $(OBJDIR)/kern/bochs.img $(OBJDIR)/fs/fs.img
	bochs

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps

.phony: lab%.tar.gz

