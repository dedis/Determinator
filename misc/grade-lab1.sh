#if LAB >= 1
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. misc/grade-functions.sh


$make
run

score=0

pts=10; greptest "Printf:    " "1234 decimal is 2322 octal!"
pts=15; greptest "Backtrace: " "debug_check() succeeded!"
pts=25; greptest "Traps:     " "trap_check_kernel() succeeded!"
pts=25; greptest "User mode: " "trap_check_user() succeeded!"
pts=25; greptest "Memory:    " "mem_check() succeeded!"

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
