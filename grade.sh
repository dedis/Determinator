#!/bin/sh

# Kernel will panic, which will drop back to debugger.
(echo "c"; echo "quit") | bochs-nogui >bochs.out

score=0

#///LAB3
XXX
#///ELSE
# Lab2 grading
# echo -n "Page directory: "
awk 'BEGIN{printf("Page directory: ");}'		# goddamn suns can't echo -n.
if grep "check_boot_pgdir() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

# echo -n "Page management: "
awk 'BEGIN{printf("Page management: ");}'
if grep "page_check() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

echo "Score: $score/40"
#///END

