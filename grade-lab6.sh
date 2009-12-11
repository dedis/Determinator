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
	num=$1
	$TCPDUMP -XX -r slirp.cap 2>/dev/null | egrep 0x0000 | (
		n=0
		while read line; do
			if [ $n -eq $num ]; then
				fail "extra packets sent"
				return 1
			fi
			if ! echo $line | egrep ": 5061 636b 6574 203. 3. Packet.0?$n$" > /dev/null; then
				fail "incorrect packet $n of $num"
				return 1
			fi
			n=`expr $n + 1`
		done
		if [ $n -ne $num ]; then
			fail "only got $n of $num packets"
			return 1
		fi
	)
	if [ $? = 0 ]; then
		pass
	fi
}

wait_for_line() {
	found=0
	for tries in `seq 10`; do
		if egrep "$1" jos.out >/dev/null; then
			found=1
			break
		fi
		sleep 1
	done

	if [ $found -eq 0 ]; then
		kill $qemu_pid
		wait 2> /dev/null

		echo "missing '$1'"
		fail
		return 1
	fi
}

qemu_test_testinput() {
	num=$1

	# Wait until it's ready to receive packets
	if ! wait_for_line 'Waiting for packets'; then
		return
	fi

	# Send num UDP packets
	for m in `seq -f '%03g' $num`; do
		echo "Packet $m" | nc -u -q 0 localhost $echosrv_port
	done

	# Wait for the packets to be processed
	sleep 1

	kill $qemu_pid
	wait 2> /dev/null

	egrep '^input: ' jos.out | (
		expect() {
			if ! read line; then
				fail "$name not received"
				exit 1
			fi
			if ! echo "$line" | egrep "$1$" >/dev/null; then
				fail "receiving $name"
				echo "expected input: $1"
				echo "got      $line"
				exit 1
			fi
		}

		# ARP reply
		name="ARP reply"
		expect "0000   5254 0012 3456 ....  .... .... 0806 0001"
		expect "0010   0800 0604 0002 ....  .... .... 0a00 0202"
		expect "0020   5254 0012 3456 0a00  020f"

		for m in `seq -f '%03g' $num`; do
			name="packet $m/$num"
			hex=$(echo $m | sed -re 's/(.)(.)(.)/3\1 3\23\3/')
			expect "0000   5254 0012 3456 ....  .... .... 0800 4500"
			expect "0010   0027 .... 0000 ..11  .... .... .... 0a00"
			expect "0020   020f .... 0007 0013  .... 5061 636b 6574"
			expect "0030   20$hex 0a"
		done
	)
	if [ $? = 0 ]; then
		pass
	fi
}

qemu_test_httpd() {
	if ! wait_for_line 'Waiting for http connections'; then
		return
	fi

	echo ""

	perl -e "print '    wget localhost:$http_port/: '"
	if wget -o wget.log -O /dev/null localhost:$http_port/; then
		fail "got back data";
	else
		if egrep "ERROR 404" wget.log >/dev/null; then
			pass;
		else
			fail "did not get 404 error";
		fi
	fi

	perl -e "print '    wget localhost:$http_port/index.html: '"
	if wget -o /dev/null -O qemu.out localhost:$http_port/index.html; then
		if diff qemu.out fs/index.html > /dev/null; then
			pass;
		else
			fail "returned data does not match index.html";
		fi
	else
		fail "got error";
	fi

	perl -e "print '    wget localhost:$http_port/random_file.txt: '"
	if wget -o wget.log -O /dev/null localhost:$http_port/random_file.txt; then
		fail "got back data";
	else
		if egrep "ERROR 404" wget.log >/dev/null; then
			pass;
		else
			fail "did not get 404 error";
		fi
	fi

	kill $qemu_pid
	wait 2> /dev/null
}

qemu_test_echosrv() {
	if ! wait_for_line 'bound'; then
		return
	fi

	str="$t0: network server works"
	echo $str | nc -q 3 localhost $echosrv_port > qemu.out

	kill $qemu_pid
	wait 2> /dev/null

	if egrep "^$str\$" qemu.out > /dev/null
	then
		pass
	else
		fail
	fi
}

score=0

http_port=`rand`
echosrv_port=`rand`
echo "using http port: $http_port"
echo "using echo server port: $echosrv_port"

qemuopts="$qemuopts -net user -net nic,model=i82559er"
qemuopts="$qemuopts -redir tcp:$echosrv_port::7 -redir tcp:$http_port::80"
qemuopts="$qemuopts -redir udp:$echosrv_port::7"
qemuopts="$qemuopts -pcap slirp.cap"

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

	# Wait for jos.out
	sleep 1
}

pts=15
runtest1 -tag "testinput [5 packets]" -dir net testinput -DTEST_NO_NS
qemu_test_testinput 5

pts=10
runtest1 -tag "testinput [100 packets]" -dir net testinput -DTEST_NO_NS
qemu_test_testinput 100

pts=15
runtest1 -tag 'tcp echo server [echosrv]' echosrv
qemu_test_echosrv

pts=10 # Times 3 tests
runtest1 -tag 'web server [httpd]' httpd
qemu_test_httpd

echo "Score: $score/100"

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
