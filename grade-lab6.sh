#if LAB >= 6
#!/bin/sh

qemuopts="-hda obj/kern/kernel.img -hdb obj/fs/fs.img"
. ./grade-functions.sh

TCPDUMP=`PATH=$PATH:/usr/sbin:/sbin which tcpdump`
if [ x$TCPDUMP = x ]; then
	echo "Unable to find tcpdump in path, /usr/sbin, or /sbin" >&2
	exit 1
fi

$make

rand() {
	perl -e "my \$r = int(1024 + rand() * (65535 - 1024));print \"\$r\\n\";"
}

qemu_test_testoutput() {
	t1=`date +%s.%N 2>/dev/null`
	time=`echo "scale=1; ($t1-$t0)/1" | sed 's/.N/.0/g' | bc 2>/dev/null`
	time="(${time}s)"

	num=$1
	$TCPDUMP -XX -r slirp.cap 2>/dev/null | egrep 0x0000 | (
		n=0
		while read line; do
			if [ $n -eq $num ]; then
				echo "WRONG, extra packets sent" $time
				return 1
			fi
			if ! echo $line | egrep ": 5061 636b 6574 203. 3. Packet.0?$n$" > /dev/null; then
				echo "WRONG, incorrect packet $n of $num" $time
				return 1
			fi
			n=`expr $n + 1`
		done
		if [ $n -ne $num ]; then
			echo "WRONG, only got $n of $num packets" $time
			return 1
		fi
	)
	if [ $? = 0 ]; then
		score=`expr $pts + $score`
		echo "OK" $time
	fi
}

qemu_test_httpd() {
	pts=5

	echo ""

	perl -e "print '    wget localhost:$http_port/: '"
	if wget -o wget.log -O /dev/null localhost:$http_port/; then
		echo "WRONG, got back data";
	else
		if egrep "ERROR 404" wget.log >/dev/null; then
			score=`expr $pts + $score`
			echo "OK";
		else
			echo "WRONG, did not get 404 error";
		fi
	fi

	perl -e "print '    wget localhost:$http_port/index.html: '"
	if wget -o /dev/null -O qemu.out localhost:$http_port/index.html; then
		if diff qemu.out fs/index.html > /dev/null; then
			score=`expr $pts + $score`
			echo "OK";
		else
			echo "WRONG, returned data does not match index.html";
		fi
	else
		echo "WRONG, got error";
	fi

	perl -e "print '    wget localhost:$http_port/random_file.txt: '"
	if wget -o wget.log -O /dev/null localhost:$http_port/random_file.txt; then
		echo "WRONG, got back data";
	else
		if egrep "ERROR 404" wget.log >/dev/null; then
			score=`expr $pts + $score`
			echo "OK";
		else
			echo "WRONG, did not get 404 error";
		fi
	fi

	kill $qemu_pid
	wait 2> /dev/null

	t1=`date +%s.%N 2>/dev/null`
	time=`echo "scale=1; ($t1-$t0)/1" | sed 's/.N/.0/g' | bc 2>/dev/null`
	time="(${time}s)"
}

qemu_test_echosrv() {
	pts=85

	str="$t0: network server works"
	echo $str | nc -q 3 localhost $echosrv_port > qemu.out

	kill $qemu_pid
	wait 2> /dev/null

	t1=`date +%s.%N 2>/dev/null`
	time=`echo "scale=1; ($t1-$t0)/1" | sed 's/.N/.0/g' | bc 2>/dev/null`
	time="(${time}s)"

	if egrep "^$str\$" qemu.out > /dev/null
	then
		score=`expr $pts + $score`
		echo OK $time
	else
		echo WRONG $time
	fi
}

# Reset the file system to its original, pristine state
resetfs() {
	rm -f obj/fs/fs.img
	$make obj/fs/fs.img >$out
}

score=0

http_port=`rand`
echosrv_port=`rand`
echo "using http port: $http_port"
echo "using echo server port: $echosrv_port"

qemuopts="$qemuopts -net user -net nic,model=i82559er"
qemuopts="$qemuopts -redir tcp:$echosrv_port::7 -redir tcp:$http_port::80"
qemuopts="$qemuopts -pcap slirp.cap"

resetfs

pts=5
runtest1 -tag 'testtime' testtime -DTEST_NO_NS \
	'starting count down: 5 4 3 2 1 0 ' \

# Make continuetest a no-op and check results ourselves
continuetest () {
	return
}

pts=15
rm -f obj/net/testoutput*
runtest1 -tag 'testoutput [5 packets]' -dir net testoutput -DTEST_NO_NS -DTESTOUTPUT_COUNT=5
qemu_test_testoutput 5

pts=10
rm -f obj/net/testoutput*
runtest1 -tag 'testoutput [100 packets]' -dir net testoutput -DTEST_NO_NS -DTESTOUTPUT_COUNT=100
qemu_test_testoutput 100

# Override run to start QEMU and return without waiting
run() {
	t0=`date +%s.%N 2>/dev/null`
	# The timeout here doesn't really matter, but it helps prevent
	# runaway qemu's
	(
		ulimit -t $timeout
		exec $qemu -nographic $qemuopts -serial file:jos.out -monitor null -no-reboot
	) >$out 2>$err &
        qemu_pid=$!

	sleep 8 # wait for qemu to start up
}

runtest1 -tag 'tcp echo server [echosrv]' echosrv
qemu_test_echosrv

runtest1 -tag 'web server [httpd]' httpd
qemu_test_httpd

echo "Score: $score/130"

if [ $score -lt 130 ]; then
    exit 1
fi
#endif
