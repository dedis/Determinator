#!/bin/sh

# Set a breakpoint at "panic()" and run until that breakpoint is reached.
panic=`i386-osclass-aout-nm kern/kernel | grep "_panic$" | sed -e 's/^f/0/' -e 's/ .*$//'`
(echo "b 0x$panic"; echo "c"; echo "quit") | bochs-nogui >bochs.out

score=0

#///LAB3
XXX
#///ELSE
# Lab2 grading
echo -n "Page directory: "
if grep "check_boot_page_directory() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Page management: "
if grep "ppage_check() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

echo "Score: $score/40"
#///END

