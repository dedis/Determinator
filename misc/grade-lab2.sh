#if LAB >= 2
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. misc/grade-functions.sh


$make
run

score=0

pts=20; greptest "Spinlocks:  " "spinlock_check() succeeded!"
pts=20; greptest "Scheduler:  " "in user()"
pts=20; greptest "Fork/join:  " "proc_check() 2-child test succeeded"
pts=20; greptest "Preemption: " "proc_check() 4-child test succeeded"
pts=20; greptest "Reflection: " "proc_check() trap reflection test succeeded"

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
