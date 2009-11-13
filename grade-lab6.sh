#if LAB >= 6
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

pts=2
timeout=30
preservefs=n

echo_n () {
	# suns can't echo -n, and Mac OS X can't echo "x\c"
	# assume argument has no doublequotes
	awk 'BEGIN { printf("'"$*"'"); }' </dev/null
}

rand() {
	perl -e "my \$r = int(1024 + rand() * (65535 - 1024));print \"\$r\\n\";"
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

# Usage: runqemu <tagname> <defs> <strings...>
runqemu() {
	perl -e "print '$1: '"
	rm -f obj/kern/init.o obj/kern/kernel obj/kern/bochs.img 
	[ "$preservefs" = y ] || rm -f obj/fs/fs.img
	if $verbose
	then
		echo "gmake $2... "
	fi
	gmake $2 >$out
	if [ $? -ne 0 ]
	then
		echo gmake $2 failed 
		exit 1
	fi

	t0=`date +%s.%N 2>/dev/null`
	qemu -hda obj/kern/bochs.img -hdb obj/fs/fs.img \
	     -net user -net nic,model=i82559er -parallel /dev/stdout \
	     -redir tcp:$echosrv_port::10000 -redir tcp:$http_port::80 \
	     -nographic -pidfile qemu.pid -pcap slirp.cap 2>/dev/null&

	sleep 3 # wait for qemu to start up

	qemu_pid=`cat qemu.pid`
	rm -f qemu.pid
}

# Usage: runtestq [-tag <tagname>] <progname> [-Ddef...] STRINGS...
runtestq() {
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
	testnet_defs=
	while expr "x$1" : 'x-D.*' >/dev/null; do
		testnet_defs="DEFS+='$1' $testnet_defs"
		shift
	done
	runqemu "$tag" "DEFS='-DTEST=_binary_obj_user_${prog}_start' DEFS+='-DTESTSIZE=_binary_obj_user_${prog}_size' $testnet_defs" "$@"

	# now run the actuall test
	qemu_test_${prog}
}

score=0

# Reset the file system to its original, pristine state
resetfs() {
	rm -f obj/fs/fs.img
	gmake obj/fs/fs.img >$out
}

http_port=`rand`
echosrv_port=`rand`
echo "using http port: $http_port"
echo "using echo server port: $echosrv_port"

score=0
pts=85
preservefs=n
runtestq -tag 'tcp echo server [echosrv]' echosrv

#pts=15 # points are allocated in the test code
preservefs=n
runtestq -tag 'web server [httpd]' httpd

echo PART A SCORE: $score/100

if [ $score -lt 100 ]; then
    exit 1
fi
#endif
