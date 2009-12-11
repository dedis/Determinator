#if LAB >= 1
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. ./grade-functions.sh


$make
run

score=0

	pts=20
	echo_n "Printf: "
#ifdef ENV_CLASS_NYU
	if grep "480 decimal is 740 octal!" jos.out >/dev/null
#else /* !ENV_CLASS_NYU */
	if grep "6828 decimal is 15254 octal!" jos.out >/dev/null
#endif /* !ENV_CLASS_NYU */
	then
		pass
	else
		fail
	fi

	pts=10
	echo "Backtrace:"
	args=`grep "ebp f01.* eip f0100.* args" jos.out | awk '{ print $6 }'`
	cnt=`echo $args | grep '^00000000 00000000 00000001 00000002 00000003 00000004 00000005' | wc -w`
	echo_n "   Count "
	if [ $cnt -eq 8 ]
	then
		pass
	else
		fail
	fi

	cnt=`grep "ebp f01.* eip f0100.* args" jos.out | awk 'BEGIN { FS = ORS = " " }
{ print $6 }
END { printf("\n") }' | grep '^00000000 00000000 00000001 00000002 00000003 00000004 00000005' | wc -w`
	echo_n "   Args "
	if [ $cnt -eq 8 ]; then
		pass
	else
		fail "($args)"
	fi

	syms=`grep "kern/init.c:.* test_backtrace" jos.out`
	symcnt=`grep "kern/init.c:.* test_backtrace" jos.out | wc -l`
	echo_n "   Symbols "
	if [ $symcnt -eq 6 ]; then
		pass
	else
		fail "($syms)"
	fi

echo "Score: $score/50"

if [ $score -lt 50 ]; then
    exit 1
fi
#endif
