#if LAB >= 2
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. ./grade-functions.sh


$make
run

score=0

pts=20
echo_n "Page directory: "
 if grep "check_boot_pgdir() succeeded!" pios.out >/dev/null
 then
	pass
 else
	fail
 fi

pts=30
echo_n "Page management: "
 if grep "page_check() succeeded!" pios.out >/dev/null
 then
	pass
 else
	fail
 fi

echo "Score: $score/50"

if [ $score -lt 50 ]; then
    exit 1
fi
#endif
