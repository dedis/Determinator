#///BEGIN 1
TOP = .

# Cross-compiler osclass toolchain
CC	:= i386-osclass-aout-gcc -pipe
GCC_LIB := $(shell $(CC) -print-libgcc-file-name)
AS	:= i386-osclass-aout-as
AR	:= i386-osclass-aout-ar
LD	:= i386-osclass-aout-ld
OBJCOPY	:= i386-osclass-aout-objcopy

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= gtar
PERL	:= perl

# Flags
CFLAGS	:= $(CFLAGS) -I$(TOP) -MD -MP
LDFLAGS := $(LDFLAGS)

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=
CLEAN_FILES := .deps
CLEAN_PATS := *.o *.d


# Make sure that 'all' is the first target
all:


# Include Makefrags for subdirectories
include kern/Makefrag
include boot/Makefrag
include user/Makefrag
include tools/mkimg/Makefrag


# Eliminate default suffix rules
.SUFFIXES:
                                                                                

# Rules for building regular object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<


# For embedding one program in another
%.b.c: %.b
	rm -f $@
	$(TOP)/tools/bintoc/bintoc $< $*_bin > $@~ && $(MV) -f $@~ $@
%.b.s: %.b
	rm -f $@
	$(TOP)/tools/bintoc/bintoc -S $< $*_bin > $@~ && $(MV) -f $@~ $@


# For cleaning the source tree
clean:
	rm -rf $(CLEAN_FILES) $(foreach dir,$(OBJDIRS), \
				$(addprefix $(dir)/,$(CLEAN_PATS)))

handin:
	-gmake clean
	-rm handin.tgz
	$(TAR) czvf handin5.tgz .


# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(dir)/*.d))
	@$(PERL) mergedep.pl $@ $^

-include .deps


#///BEGIN 200

# Find all potentially exportable files
LAB_PATS := Makefrag *.c *.h
LAB_DIRS := inc $(OBJDIRS)
LAB_FILES := GNUmakefile .bochsrc mergedep.pl \
	$(wildcard $(foreach dir,$(LAB_DIRS),$(addprefix $(dir)/,$(LAB_PATS))))


# Fake targets to export the student lab handout and solution trees.
# It's important that these aren't just called 'lab%' and 'sol%',
# because that causes 'lab%' to match 'kern/lab3.S' and delete it - argh!
export-lab%:
	rm -rf $@
	$(PERL) mklab.pl $* 0 $(LAB_FILES)
export-sol%:
	rm -rf $@
	$(PERL) mklab.pl $* 1 $(LAB_FILES)

# Distribute the BIOS images Bochs needs with the lab trees
# in order to avoid absolute pathname dependencies in .bochsrc.
bios:
	mkdir $@
bios/%: /usr/local/share/bochs/bios/% bios
	cp $< $@
all: bios/BIOS-bochs-latest bios/VGABIOS-elpin-2.40


#///END


#///END
