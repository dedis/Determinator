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

if gmake --version >/dev/null 2>&1; then make=gmake; else make=make; fi

pts=5
timeout=30
preservefs=n
qemu=`$SHELL misc/which-qemu.sh`
brkfn=done
in=/dev/null

echo_n () {
	# suns can't echo -n, and Mac OS X can't echo "x\c"
	# assume argument has no doublequotes
	awk 'BEGIN { printf("'"$*"'"); }' </dev/null
}

# Run QEMU with serial output redirected to grade-out.  If $brkfn is
# non-empty, wait until $brkfn is reached or $timeout expires, then
# kill QEMU.
run () {
	qemuextra=
	if [ "$brkfn" ]; then
		# Generate a unique GDB port
		port=$(expr `id -u` % 5000 + 25000)
		qemuextra="-S -gdb tcp::$port"
	fi

	t0=`date +%s.%N 2>/dev/null`
	(
		ulimit -t $timeout
		exec $qemu -nographic $qemuopts -serial stdio -monitor null \
			-no-reboot $qemuextra -m 1100M
	) <$in >grade-out 2>$err &
	PID=$!

	# Wait for QEMU to start
	sleep 1

	if [ "$brkfn" ]; then
		# Find the address of the function $brkfn in the kernel.
		brkaddr=`grep " $brkfn\$" obj/kern/kernel.sym | sed -e's/ .*$//g'`

		(
			echo "target remote localhost:$port"
			echo "br *0x$brkaddr"
			echo c
		) > grade-gdb-in
		gdb -batch -nx -x grade-gdb-in > /dev/null 2>&1
		rm grade-gdb-in

		# Make sure QEMU is dead.  On OS X, exiting gdb
		# doesn't always exit QEMU.
		kill $PID > /dev/null 2>&1
	fi
}

passfailmsg () {
	msg="$1"
	shift
	if [ $# -gt 0 ]; then
		msg="$msg,"
	fi

	t1=`date +%s.%N 2>/dev/null`
	time=`echo "scale=1; ($t1-$t0)/1" | sed 's/.N/.0/g' | bc 2>/dev/null`

	echo $msg "$@" "(${time}s)"
}

pass () {
	passfailmsg OK "$@"
	score=`expr $pts + $score`
}

fail () {
	passfailmsg WRONG "$@"
}

greptest () {
	echo_n "$1"
	if grep "$2" grade-out >/dev/null
	then
		pass
	else
		fail
	fi
}

# Grep for a multi-line pattern
grmltest () {
	echo_n "$1"
	if tr <grade-out '\n' '!' | grep "$2" >/dev/null
	then
		pass
	else
		fail
	fi
}

