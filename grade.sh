#!/bin/sh

verbose=false

if [ "x$1" = "x-v" ]
then
	verbose=true
	out=/dev/stdout
	err=/dev/stderr
else
	out=/dev/null
	err=/dev/null
fi

pts=5
timeout=30

runbochs() {
	# Find the address of the kernel readline function,
	# which the kernel monitor uses to read commands interactively.
	brkaddr=`grep readline obj/kern/kernel.sym | sed -e's/ .*$//g'`
	#echo "brkaddr $brkaddr"

	# Run Bochs, setting a breakpoint at readline(),
	# and feeding in appropriate commands to run, then quit.
	(
		echo vbreak 0x8:0x$brkaddr
		echo c
		echo die
		echo quit
	) | (
		ulimit -t $timeout
		bochs -q 'display_library: nogui' \
			'parport1: enabled=1, file="bochs.out"'
	) >$out 2>$err
}

#if LAB >= 3

# Usage: runtest <tagname> <defs> <strings...>
runtest() {
	perl -e "print '$1: '"
	rm -f obj/kern/init.o obj/kern/kernel obj/kern/bochs.img
	if $verbose
	then
		echo "gmake $2... "
	fi
	if ! gmake $2 >$out
	then
		echo gmake failed 
		exit 1
	fi
	runbochs
	if [ ! -s bochs.out ]
	then
		echo 'no bochs.out'
	else
		shift
		shift
		continuetest "$@"
	fi
}

quicktest() {
	perl -e "print '$1: '"
	shift
	continuetest "$@"
}

continuetest() {
	okay=yes

	not=false
	for i
	do
		if [ "x$i" = "x!" ]
		then
			not=true
		elif $not
		then
			if egrep "^$i\$" bochs.out >/dev/null
			then
				echo "got unexpected line '$i'"
				if $verbose
				then
					exit 1
				fi
				okay=no
			fi
			not=false
		else
			if ! egrep "^$i\$" bochs.out >/dev/null
			then
				echo "missing '$i'"
				if $verbose
				then
					exit 1
				fi
				okay=no
			fi
			not=false
		fi
	done
	if [ "$okay" = "yes" ]
	then
		score=`echo $pts+$score | bc`
		echo OK
	else
		echo WRONG
	fi
}

# Usage: runtest1 [-tag <tagname>] <progname>
runtest1() {
	if [ $1 = -tag ]
	then
		shift
		tag=$1
		prog=$2
		shift
		shift
	else
		tag=$1
		prog=$1
		shift
	fi
	runtest "$tag" "DEFS=-DTEST=_binary_obj_user_${prog}_start DEFS+=-DTESTSIZE=_binary_obj_user_${prog}_size" "$@"
}

#endif

#if LAB >= 6		/******************** LAB 6 ********************/
score=0

# 20 points - run-icode
pts=20
runtest1 -tag 'updated file system switch' icode \
	'icode: read /motd' \
	'This is /motd, the message of the day.' \
	'icode: spawn /init' \
	'init: running' \
	'init: data seems okay' \
	'icode: exiting' \
	'init: bss seems okay' \
	"init: args: 'init' 'initarg1' 'initarg2'" \
	'init: running sh' \

pts=10
runtest1 -tag 'PTE_LIBRARY' testptelibrary \
	'fork handles PTE_LIBRARY right' \
	'spawn handles PTE_LIBRARY right' \

# 10 points - run-testfdsharing
pts=10
runtest1 -tag 'fd sharing' testfdsharing \
	'read in parent succeeded' \
	'read in child succeeded' 

# 10 points - run-testpipe
pts=10
runtest1 -tag 'pipe' testpipe \
	'pipe read closed properly' \
	'pipe write closed properly' \

# 10 points - run-testpiperace
pts=10
runtest1 -tag 'pipe race' testpiperace \
	! 'child detected race' \
	! 'RACE: pipe appears closed' \
	"race didn't happen" \

# 10 points - run-testpiperace2
pts=10
runtest1 -tag 'pipe race 2' testpiperace2 \
	! 'RACE: pipe appears closed' \
	! 'child detected race' \
	"race didn't happen" \

# 10 points - run-primespipe
pts=10
timeout=120
echo 'The primes test has up to 2 minutes to complete.  Be patient.'
runtest1 -tag 'primes' primespipe \
	! 1 2 3 ! 4 5 ! 6 7 ! 8 ! 9 \
	! 10 11 ! 12 13 ! 14 ! 15 ! 16 17 ! 18 19 \
	! 20 ! 21 ! 22 23 ! 24 ! 25 ! 26 ! 27 ! 28 29 \
	! 30 31 ! 32 ! 33 ! 34 ! 35 ! 36 37 ! 38 ! 39 \
	541 1009 1097

# 20 points - run-testshell
pts=20
timeout=60
runtest1 -tag 'shell' testshell \
	'shell ran correctly' \

echo SCORE: $score/100

#elif LAB >= 5		/******************** LAB 5 ********************/
score=0

# Reset the file system to its original, pristine state
resetfs() {
	rm -f obj/fs/fs.img
	gmake obj/fs/fs.img >$out
}


resetfs

runtest1 -tag 'fs i/o' testfsipc \
	'FS can do I/O' \
	! 'idle loop can do I/O' \

quicktest 'read_block' \
	'superblock is good' \

quicktest 'write_block' \
	'write_block is good' \

quicktest 'read_bitmap' \
	'read_bitmap is good' \

quicktest 'alloc_block' \
	'alloc_block is good' \

quicktest 'file_open' \
	'file_open is good' \

quicktest 'file_get_block' \
	'file_get_block is good' \

quicktest 'file_truncate' \
	'file_truncate is good' \

quicktest 'file_flush' \
	'file_flush is good' \

quicktest 'file rewrite' \
	'file rewrite is good' \

quicktest 'serv_*' \
	'serve_open is good' \
	'serve_map is good' \
	'serve_close is good' \
	'stale fileid is good' \

echo PART A SCORE: $score/55

resetfs

score=0
pts=10
runtest1 -tag 'motd display' writemotd \
	'OLD MOTD' \
	'This is /motd, the message of the day.' \
	'NEW MOTD' \
	'This is the NEW message of the day!' \

runtest1 -tag 'motd change' writemotd \
	'OLD MOTD' \
	'This is the NEW message of the day!' \
	'NEW MOTD' \
	! 'This is /motd, the message of the day.' \

resetfs

pts=25
runtest1 -tag 'spawn via icode' icode \
	'icode: read /motd' \
	'This is /motd, the message of the day.' \
	'icode: spawn /init' \
	'init: running' \
	'init: data seems okay' \
	'icode: exiting' \
	'init: bss seems okay' \
	"init: args: 'init' 'initarg1' 'initarg2'" \
	'init: exiting' \

echo PART B SCORE: $score/45

exit 0

#elif LAB >= 4		/******************** LAB 4 ********************/

score=0
timeout=10

runtest1 dumbfork \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'0: I am the parent!' \
	'9: I am the parent!' \
	'0: I am the child!' \
	'9: I am the child!' \
	'19: I am the child!' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' \
	'.00001802. exiting gracefully' \
	'.00001802. free env 00001802'

echo PART A SCORE: $score/5

runtest1 faultread \
	! 'I read ........ from location 0!' \
	'.00001001. user fault va 00000000 ip 008.....' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000e Page Fault' \
	'  err  0x00000004' \
	'.00001001. free env 00001001'

runtest1 faultwrite \
	'.00001001. user fault va 00000000 ip 008.....' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000e Page Fault' \
	'  err  0x00000006' \
	'.00001001. free env 00001001'

runtest1 faultdie \
	'i faulted at va deadbeef, err 6' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' 

runtest1 faultalloc \
	'fault deadbeef' \
	'this string was faulted in at deadbeef' \
	'fault cafebffe' \
	'fault cafec000' \
	'this string was faulted in at cafebffe' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001'

runtest1 faultallocbad \
	'.00001001. PFM_KILL va deadbeef ip f01.....' \
	'.00001001. free env 00001001' 

runtest1 faultnostack \
	'.00001001. PFM_KILL va eebfff.. ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultbadhandler \
	'.00001001. PFM_KILL va eebfef.. ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultevilhandler \
	'.00001001. PFM_KILL va eebfef.. ip f01.....' \
	'.00001001. free env 00001001'

runtest1 forktree \
	'....: I am .0.' \
	'....: I am .1.' \
	'....: I am .000.' \
	'....: I am .100.' \
	'....: I am .110.' \
	'....: I am .111.' \
	'....: I am .011.' \
	'....: I am .001.' \
	'.000028... exiting gracefully' \
	'.000048... exiting gracefully' \
	'.000058... exiting gracefully' \
	'.000078... exiting gracefully' \
	'.000078... free env 000078..'

echo PART B SCORE: $score/50

runtest1 spin \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'I am the parent.  Forking the child...' \
	'.00001001. new env 00001802' \
	'I am the parent.  Running the child...' \
	'I am the child.  Spinning...' \
	'I am the parent.  Killing the child...' \
	'.00001001. destroying 00001802' \
	'.00001001. free env 00001802' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001'

runtest1 pingpong \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'.00001001. new env 00001802' \
	'send 0 from 1001 to 1802' \
	'1802 got 0 from 1001' \
	'1001 got 1 from 1802' \
	'1802 got 8 from 1001' \
	'1001 got 9 from 1802' \
	'1802 got 10 from 1001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' \
	'.00001802. exiting gracefully' \
	'.00001802. free env 00001802' \

runtest1 primes \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'.00001001. new env 00001802' \
	'2 .00001802. new env 00002003' \
	'3 .00002003. new env 00002804' \
	'5 .00002804. new env 00003005' \
	'7 .00003005. new env 00003806' \
	'11 .00003806. new env 00004007' 

echo PART C SCORE: $score/65


#elif LAB >= 3		/******************** LAB 3 ********************/

score=0

runtest1 hello \
	'.00000000. new env 00000800' \
	'hello, world' \
	'i am environment 00000800' \
	'.00000800. destroying 00000800' \
	'.00000800. free env 00000800' \
	'Destroyed the only environment - nothing more to do!'

# the [00000800] tags should have [] in them, but that's 
# a regular expression reserved character, and i'll be damned if
# I can figure out how many \ i need to add to get through 
# however many times the shell interprets this string.  sigh.

runtest1 buggyhello \
	'.00000800. PFM_KILL va 00000001 ip f01.....' \
	'.00000800. free env 00000800'

runtest1 evilhello \
	'.00000800. PFM_KILL va ef800000 ip f01.....' \
	'.00000800. free env 00000800'

runtest1 divzero \
	! '1/0 is ........!' \
	'TRAP frame at 0xefbfff..' \
	'  trap 0x00000000 Divide error' \
	'  eip  0x008.....' \
	'  ss   0x----0023' \
	'.00000800. free env 00000800'

runtest1 breakpoint \
	'Welcome to the JOS kernel monitor!' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x00000003 Breakpoint' \
	'  eip  0x008.....' \
	'  ss   0x----0023' \
	! '.00000800. free env 00000800'

runtest1 softint \
	'Welcome to the JOS kernel monitor!' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000d General Protection' \
	'  eip  0x008.....' \
	'  ss   0x----0023' \
	'.00000800. free env 00000800'

runtest1 badsegment \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000d General Protection' \
	'  err  0x0000001c' \
	'  eip  0x008.....' \
	'  ss   0x----0023' \
	'.00000800. free env 00000800'

runtest1 faultread \
	! 'I read ........ from location 0!' \
	'.00000800. user fault va 00000000 ip 008.....' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000e Page Fault' \
	'  err  0x00000004' \
	'.00000800. free env 00000800'

runtest1 faultreadkernel \
	! 'I read ........ from location 0xf0100000!' \
	'.00000800. user fault va f0100000 ip 008.....' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000e Page Fault' \
	'  err  0x00000005' \
	'.00000800. free env 00000800' \

runtest1 faultwrite \
	'.00000800. user fault va 00000000 ip 008.....' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000e Page Fault' \
	'  err  0x00000006' \
	'.00000800. free env 00000800'

runtest1 faultwritekernel \
	'.00000800. user fault va f0100000 ip 008.....' \
	'TRAP frame at 0xefbfffbc' \
	'  trap 0x0000000e Page Fault' \
	'  err  0x00000007' \
	'.00000800. free env 00000800'

runtest1 testbss \
	'Making sure bss works right...' \
	'Yes, good.  Now doing a wild write off the end...' \
	'.00000800. user fault va 00c..... ip 008.....' \
	'.00000800. free env 00000800'



echo Score: $score/60

score=0


#elif LAB >= 2		/******************** LAB 2 ********************/

gmake
runbochs

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
	score=`echo 30+$score | bc`
	echo OK
 else
	echo WRONG
 fi

echo "Score: $score/50"

#elif LAB >= 1		/******************** LAB 1 ********************/

gmake
runbochs

score=0

	# echo -n "Printf: "
	awk 'BEGIN{printf("Printf: ");}' </dev/null
#ifdef ENV_CLASS_NYU
	if grep "480 decimal is 740 octal!" bochs.out >/dev/null
#else /* !ENV_CLASS_NYU */
	if grep "6828 decimal is 15254 octal!" bochs.out >/dev/null
#endif /* !ENV_CLASS_NYU */
	then
		score=`echo 20+$score | bc`
		echo OK
	else
		echo WRONG
	fi

	# echo -n "Printf: "
	awk 'BEGIN{printf("Backtrace: ");}' </dev/null
	cnt=`grep "ebp f0109...  eip f0100...  args" bochs.out|wc -w`
	if [ $cnt -eq 80 ]
	then
		score=`echo 15+$score | bc`
		awk 'BEGIN{printf("Count OK");}' </dev/null
	else
		awk 'BEGIN{printf("Count WRONG");}' </dev/null
	fi

	cnt=`grep "ebp f0109...  eip f0100...  args" bochs.out | awk 'BEGIN { FS = ORS = " " }
{ print $6 }
END { printf("\n") }' | grep '^00000000 00000000 00000001 00000002 00000003 00000004 00000005' | wc -w`
	if [ $cnt -eq 8 ]; then
		score=`echo 15+$score | bc`
		echo , Args OK
	else
		echo , Args WRONG
	fi

echo "Score: $score/50"

#endif

