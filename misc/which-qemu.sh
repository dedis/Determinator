#!/bin/sh
#
# Try to find an appropriate version of QEMU to use.
# Use the version in the /c/cs422/tools/bin directory first (XXX local hack!)
# Otherwise, look in a couple typical places.

if test -x /c/cs422/tools/bin/qemu; then
	echo /c/cs422/tools/bin/qemu
	exit
elif which qemu > /dev/null; then
	echo qemu
	exit
else
	# Find the appropriate binary in the Mac OS X distribution
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $qemu; then
		echo $qemu
		exit
	fi
fi

echo "***" 1>&2
echo "*** Error: Couldn't find a working QEMU executable." 1>&2
echo "*** Is the directory containing the qemu binary in your PATH" 1>&2
echo "*** or have you tried setting the QEMU variable in conf/env.mk?" 1>&2
echo "***" 1>&2

exit 1
