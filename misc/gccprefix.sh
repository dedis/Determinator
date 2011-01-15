#!/bin/sh
#
# Find the appropriate prefix to add to gcc/binutils commands.
# Used by our GNUmakefile to set the GCCPREFIX variable.
# We look for the following toolchains, in decreasing preference order:
#
#	1. pios-*	(pios-gcc, pios-ld, etc.)
#	2. i386-elf-*	(i386-elf-gcc, i386-elf-ld, etc.)
#	3. no prefix	ONLY if the system default GCC supports elf32-i386
#

if pios-objdump -i 2>&1 | grep '^elf32-i386$' >/dev/null 2>&1; then
	echo 'pios-'
elif i386-elf-objdump -i 2>&1 | grep '^elf32-i386$' >/dev/null 2>&1; then
	echo 'i386-elf-'
elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; then
	echo ''
else
	echo "***" 1>&2
	echo "*** Error: Can't find an i386-elf version of GCC/binutils." 1>&2
	echo "*** Is the directory with i386-elf-gcc in your PATH?" 1>&2
	echo "*** If your i386-elf toolchain is installed with a command" 1>&2
	echo "*** prefix other than 'i386-elf-', set your GCCPREFIX" 1>&2
	echo "*** environment variable to that prefix and try again." 1>&2
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2
	echo "***" 1>&2
	exit 1
fi

