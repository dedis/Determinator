#if LAB >= 3
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. ./grade-functions.sh


$make
run

score=0

pts=15; greptest "Page tables:" "pmap_check() succeeded!"
pts=15; greptest "Load testvm:" "testvm: loadcheck passed"
pts=20; greptest "Basic fork: " "testvm: forkcheck passed"
pts=15; greptest "Protection: " "testvm: protcheck passed"
pts=20; greptest "Memory ops: " "testvm: memopcheck passed"
pts=15; greptest "Merge:      " "testvm: mergecheck passed"


echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
