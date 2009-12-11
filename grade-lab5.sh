#if LAB >= 5
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img -hdb obj/fs/fs.img"
. ./grade-functions.sh


$make

score=0
total=0

runtest1 -tag 'fs i/o [testfsipc]' testfsipc \
	'FS can do I/O' \
	! 'idle loop can do I/O' \

quicktest 'read_block [testfsipc]' \
	'superblock is good' \

quicktest 'write_block [testfsipc]' \
	'write_block is good' \

quicktest 'read_bitmap [testfsipc]' \
	'read_bitmap is good' \

quicktest 'alloc_block [testfsipc]' \
	'alloc_block is good' \

quicktest 'file_open [testfsipc]' \
	'file_open is good' \

quicktest 'file_get_block [testfsipc]' \
	'file_get_block is good' \

quicktest 'file_truncate [testfsipc]' \
	'file_truncate is good' \

quicktest 'file_flush [testfsipc]' \
	'file_flush is good' \

quicktest 'file rewrite [testfsipc]' \
	'file rewrite is good' \

quicktest 'serv_* [testfsipc]' \
	'serve_open is good' \
	'serve_map is good' \
	'serve_close is good' \
	'stale fileid is good' \

pts=10
runtest1 -tag 'motd display [writemotd]' writemotd \
	'OLD MOTD' \
	'This is /motd, the message of the day.' \
	'NEW MOTD' \
	'This is the NEW message of the day!' \

preservefs=y
runtest1 -tag 'motd change [writemotd]' writemotd \
	'OLD MOTD' \
	'This is the NEW message of the day!' \
	'NEW MOTD' \
	! 'This is /motd, the message of the day.' \

pts=25
preservefs=n
runtest1 -tag 'spawn via icode [icode]' icode \
	'icode: read /motd' \
	'This is /motd, the message of the day.' \
	'icode: spawn /init' \
	'init: running' \
	'init: data seems okay' \
	'icode: exiting' \
	'init: bss seems okay' \
	"init: args: 'init' 'initarg1' 'initarg2'" \
	'init: exiting' \

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
