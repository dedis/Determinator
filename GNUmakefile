TOP = .

# Cross-compiler osclass toolchain
CC	:= i386-osclass-aout-gcc -pipe
GCC_LIB := $(shell $(CC) -print-libgcc-file-name)
AS	:= i386-osclass-aout-as
AR	:= i386-osclass-aout-ar
LD	:= i386-osclass-aout-ld
OBJCOPY	:= i386-osclass-aout-objcopy
OBJDUMP	:= i386-osclass-aout-objdump

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= gtar
PERL	:= perl

# Compiler flags
# Note that -O2 is required for the boot loader to fit within 512 bytes;
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
DEFS	:=
CFLAGS	:= $(CFLAGS) $(DEFS) -O2 -fno-builtin -I$(TOP) -MD -MP -Wall -ggdb

# Linker flags for user programs
ULDFLAGS := -Ttext 0x800020

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=
CLEAN_FILES := .deps bochs.log
CLEAN_PATS := *.o *.d *.asm


# Make sure that 'all' is the first target
all:


# Include Makefrags for subdirectories
include kern/Makefrag
include boot/Makefrag
#if LAB >= 4
include user/Makefrag
#endif

# Eliminate default suffix rules
.SUFFIXES:
                                                                                

# Rules for building regular object files
%.o: %.c
	@echo cc $<
	@$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.S
	@echo as $<
	@$(CC) $(CFLAGS) -c -o $@ $<


# For embedding one program in another
%.b.c: %.b
	rm -f $@
	$(TOP)/tools/bintoc/bintoc $< $*_bin > $@~ && $(MV) -f $@~ $@
%.b.s: %.b
	rm -f $@
	$(TOP)/tools/bintoc/bintoc -S $< $*_bin > $@~ && $(MV) -f $@~ $@


#if LAB >= 999
# Find all potentially exportable files
LAB_PATS := COPYRIGHT Makefrag *.c *.h *.S
LAB_DIRS := inc user $(OBJDIRS)
LAB_FILES := CODING GNUmakefile .bochsrc mergedep.pl grade.sh boot/sign.pl \
	$(wildcard $(foreach dir,$(LAB_DIRS),$(addprefix $(dir)/,$(LAB_PATS))))

BIOS_FILES := bios/BIOS-bochs-latest bios/VGABIOS-elpin-2.40

CLEAN_FILES += bios lab? sol?

# Fake targets to export the student lab handout and solution trees.
# It's important that these aren't just called 'lab%' and 'sol%',
# because that causes 'lab%' to match 'kern/lab3.S' and delete it - argh!
export-lab%: $(BIOS_FILES)
	rm -rf lab$*
	$(PERL) mklab.pl $* 0 lab$* $(LAB_FILES)
	cp -R bios lab$*/
export-sol%: $(BIOS_FILES)
	rm -rf sol$*
	$(PERL) mklab.pl $* $* sol$* $(LAB_FILES)
	cp -R bios sol$*/
export-prep%: $(BIOS_FILES)
	rm -rf prep$*
	$(PERL) mklab.pl $* `echo -1+$* | bc` prep$* $(LAB_FILES)
	cp -R bios prep$*/

lab%.tar.gz: $(BIOS_FILES)
	gmake export-lab$*
	tar cf - lab$* | gzip > lab$*.tar.gz

# Distribute the BIOS images Bochs needs with the lab trees
# in order to avoid absolute pathname dependencies in .bochsrc.
bios:
	mkdir $@
bios/%: /usr/local/share/bochs/% bios
	cp $< $@
all: $(BIOS_FILES)
#endif

bochs: kern/bochs.img
	bochs-nogui

kernel.asm: kern/kernel
	$(OBJDUMP) -S --adjust-vma=0xf00ff000 kern/kernel >kernel.asm

# For cleaning the source tree
clean:
	rm -rf $(CLEAN_FILES) $(foreach dir,$(OBJDIRS), \
				$(addprefix $(dir)/,$(CLEAN_PATS)))

grade: all
	sh grade.sh

handin: clean
	-rm -f handin5.tgz
	tar cf - . | gzip | uuencode handin.tar.gz | mail 6.828-handin@pdos.lcs.mit.edu

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(dir)/*.d))
	@$(PERL) mergedep.pl $@ $^

-include .deps

.phony: lab%.tar.gz

