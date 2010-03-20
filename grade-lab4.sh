#if LAB >= 4
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. ./grade-functions.sh


$make

echo >grade-in	# blank line to clear the pipeline
echo >>grade-in "FOOBAR"
echo >>grade-in "echo echo in script >script"	# test the shell a bit
echo >>grade-in "sh <script >out"
echo >>grade-in "cat script out"
echo >>grade-in "wc script out"
echo >>grade-in "exit"				# kill the shell
in=grade-in

run

score=0

# Part 1
pts=5;  greptest "Initial FS:" "initfilecheck passed"
pts=5;  greptest "Read/write:" "readwritecheck passed"
pts=5;  greptest "Seek:      " "seekcheck passed"
pts=10; grmltest "Readdir:   " "found file .sh..*found file .ls..*readdircheck passed"

# Part 2
pts=10; grmltest "Cons out:  " "Buffered console output should NOT have appeared.*write.. to STDOUT_FILENO.*fprintf.. to .stdout.*fprintf.. to .consout. file.*Buffered console output SHOULD have appeared now.123456789"
pts=15; grmltest "Cons in:   " "Enter something: FOOBAR.You typed: FOOBAR"

# Part 3
pts=20; grmltest "Exec:      " "called by execcheck.execcheck done"

# Part 4
pts=5;  grmltest "Reconcile: " "forkwrite: reconcilefile0.forkwrite: reconcilefile1.forkwrite: reconcilefile2.*26 reconcilefile0.*26 reconcilefile1.*26 reconcilefile2"
pts=5;  grmltest "Conflicts: " "reconcilecheck: basic file reconciliation successful.*-          26 reconcilefile0.*-          26 reconcilefile1.*-          26 reconcilefile2.*C          26 reconcilefileC"
pts=10; grmltest "File merge:" "reconcilecheck: running echo.reconcilecheck: echo running.called by reconcilecheck.reconcilecheck: echo finished.reconcilecheck done"

# Final: simple high-level test of correct shell operation
pts=10; grmltest "Shell:     " "echo in script.in script.*1 3 15 script.1 2 10 out"

rm grade-in

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
