#!/bin/sh

#if LAB >= 3

# Make a copy of the original init.c
cp kern/init.c kern/init.c.bak

score=0

# Try all the different fault tests
echo -n "Divide by zero: "
sed -e 's/spin/div0/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0x0" bochs.out && \
	grep -q "err  0x0" bochs.out && \
	grep -q "eip  0x800022" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Breakpoint: "
sed -e 's/spin/brkpt/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0x3" bochs.out && \
	grep -q "err  0x0" bochs.out && \
	grep -q "eip  0x800021" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Bad data segment: "
sed -e 's/spin/badds/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0xd" bochs.out && \
	grep -q "err  0x8" bochs.out && \
	grep -q "eip  0x800025" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Read nonexistent page: "
sed -e 's/spin/pgfault_rd_nopage/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0xe" bochs.out && \
	grep -q "err  0xfffc" bochs.out && \
	grep -q "eip  0x800020" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Read kernel-only page: "
sed -e 's/spin/pgfault_rd_noperms/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0xe" bochs.out && \
	grep -q "err  0xfffd" bochs.out && \
	grep -q "eip  0x800020" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Write nonexistent page: "
sed -e 's/spin/pgfault_wr_nopage/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0xe" bochs.out && \
	grep -q "err  0xfffe" bochs.out && \
	grep -q "eip  0x800020" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Write kernel-only page: "
sed -e 's/spin/pgfault_wr_noperms/g' <kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0xe" bochs.out && \
	grep -q "err  0xffff" bochs.out && \
	grep -q "eip  0x800020" bochs.out; then
	score=`echo 5+$score | bc`
	echo OK
else
	echo WRONG
fi

echo -n "Preemption test: "
sed -e 's/u_char spin_start/u_char alice_start, bob_start/' \
	-e 's/u_char spin_end/u_char alice_end, bob_end/' \
	-e 's/env_create..spin.*$/env_create\(\&alice_start, \&alice_end - \&alice_start\); env_create\(\&bob_start, \&bob_end - \&bob_start\)\;/' \
	<kern/init.c.bak >kern/init.c
(gmake clean; gmake; ulimit -t 10; (echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"') >/dev/null 2>/dev/null
if grep -q "oesp 0xefbfffdc" bochs.out && \
	grep -q "trap 0xd" bochs.out && \
	grep -q "err  0x0" bochs.out && \
	grep -q "eip  0x800022" bochs.out; then
	score=`echo 25+$score | bc`
	echo OK
else
	echo WRONG
fi

# Restore the original init.c
mv kern/init.c.bak kern/init.c

echo "Score: $score/50"

#else
#if LAB >= 2

# Kernel will panic, which will drop back to debugger.
(echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"' 

score=0

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

