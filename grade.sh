#!/bin/sh

# Kernel will panic, which will drop back to debugger.
(echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"' 

score=0

#if LAB >= 3

XXX

#else
#if LAB >= 2

# Lab2 grading
# echo -n "Page directory: "
awk 'BEGIN{printf("Page directory: ");}' </dev/null	# goddamn suns can't echo -n.
if grep "check_boot_pgdir() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

# echo -n "Page management: "
awk 'BEGIN{printf("Page management: ");}' </dev/null
if grep "page_check() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

# echo -n "Printf: "
awk 'BEGIN{printf("Printf: ");}' </dev/null
if grep "6828 decimal is 15254 octal!" bochs.out >/dev/null
then
	score=`echo 10+$score | bc`
	echo OK
else
	echo WRONG
fi

echo "Score: $score/50"

#endif /* LAB >= 2 */
#endif /* LAB >= 3 */

