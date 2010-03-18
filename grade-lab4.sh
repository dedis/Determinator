#if LAB >= 4
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img"
. ./grade-functions.sh


$make

echo >grade-in	# blank line to clear the pipeline
echo >>grade-in "FOOBAR"
in=grade-in

run

score=0

# Part 1
pts=5;  greptest "Initial FS:" "initfilecheck passed"
pts=5;  greptest "Read/write:" "readwritecheck passed"
pts=5;  greptest "Seek:" "seekcheck passed"
pts=10; grmltest "Readdir:" "found file .sh..*found file .ls..*readdircheck passed"

# Part 2
pts=10; grmltest "Cons out:" "Buffered console output should NOT have appeared.*write.. to STDOUT_FILENO.*fprintf.. to .stdout.*fprintf.. to .consout. file.*Buffered console output SHOULD have appeared now.123456789"
pts=10; grmltest "Cons in:" "Enter something: FOOBAR.You typed: FOOBAR"


rm grade-in

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
