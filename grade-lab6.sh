#if LAB >= 6
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img -hdb obj/fs/fs.img"
. ./grade-functions.sh


$make
run

score=0

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
