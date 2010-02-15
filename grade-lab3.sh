#if LAB >= 3
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. ./grade-functions.sh


$make
run

score=0

pts=15; greptest "Page tables:" "pmap_check() succeeded!"


echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
