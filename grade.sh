#!/bin/sh

verbose=false

if [ "x$1" = "x-x" ]
then
	verbose=true
	out=/dev/stdout
	err=/dev/stderr
else
	out=/dev/null
	err=/dev/null
fi

#if LAB >= 4

pts=5
timeout=30
runtest() {
	perl -e "print '$1: '"
	rm -f kern/init.o kern/kernel kern/bochs.img fs/fs.img
	if $verbose
	then
		perl -e "print 'gmake $2... '"
	fi
	if ! gmake $2 kern/bochs.img fs/fs.img >$out
	then
		echo gmake failed 
		exit 1
	fi
	(
		ulimit -t $timeout
		(echo c; echo die; echo quit) |
			bochs-nogui 'parport1: enabled=1, file="bochs.out"'
	) >$out 2>$err
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
	runtest "$tag" "DEFS=-DTEST=binary_user_${prog}_start DEFS+=-DTESTSIZE=binary_user_${prog}_size" "$@"
}

#endif

#if LAB >= 6
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

#elif LAB >= 5
score=0

rm -f fs/fs.img
gmake fs/fs.img >$out

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

rm -f fs/fs.img
gmake fs/fs.img >$out

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

rm -f fs/fs.img
gmake fs/fs.img >$out

pts=25
runtest1 -tag 'spawn via icode' icode \
	'icode: read /motd' \
	'This is /motd, the message of the day.' \
	'icode: spawn /init' \
	'/init: running' \
	'/init: data seems okay' \
	'icode: exiting' \
	'/init: bss seems okay' \
	"/init: args: 'init' 'initarg1' 'initarg2'" \
	'/init: exiting' \

echo PART B SCORE: $score/45

exit 0

#elif LAB >= 4
score=0

runtest1 hello \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'hello, world' \
	'i am environment 00001001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001'

# the [00001001] tags should have [] in them, but that's 
# a regular expression reserved character, and i'll be damned
# if i can figure out how many \ i need to add to get through 
# however many times the shell interprets this string.  sigh.

runtest pingpong2 'DEFS=-DTEST_PINGPONG2' \
	'1802 got 0 from 1001' \
	'1001 got 1 from 1802' \
	'1802 got 8 from 1001' \
	'1001 got 9 from 1802' \
	'1802 got 10 from 1001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' \
	'.00001802. exiting gracefully' \
	'.00001802. free env 00001802'

echo PART A SCORE: $score/10

score=0

runtest1 buggyhello \
	'.00001001. PFM_KILL va 00000001 ip f01.....' \
	'.00001001. free env 00001001'

runtest1 evilhello \
	'.00001001. PFM_KILL va ef800000 ip f01.....' \
	'.00001001. free env 00001001'

runtest1 fault \
	'.00001001. user fault va 00000000 ip 008.....' \
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

runtest1 faultbadhandler \
	'.00001001. PFM_KILL va eebfcf.. ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultbadstack \
	'.00001001. PFM_KILL va ef800000 ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultgoodstack \
	'i faulted at va deadbeef, err 6, stack eebfd...' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' 

runtest1 faultevilhandler \
	'.00001001. PFM_KILL va eebfcf.. ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultevilstack \
	'.00001001. PFM_KILL va ef800000 ip f01.....' \
	'.00001001. free env 00001001'

echo PART B SCORE: $score/55

score=0

runtest1 pingpong1 \
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

echo PART C SCORE: $score/15





#elif LAB >= 3

fault() {
	perl -e "print '$1: '"
	(
		rm -f kern/init.o
		echo gmake "DEFS=-DTEST_START=$2_start -DTEST_END=$2_end"
		gmake "DEFS=-DTEST_START=$2_start -DTEST_END=$2_end"
		ulimit -t 10
		(echo c; echo quit) | bochs-nogui 'parport1: enabled=1, file="bochs.out"'
	) #>/dev/null 2>/dev/null
	if grep "oesp 0xefbfffdc" bochs.out >/dev/null \
	&& grep "trap $3"'$' bochs.out >/dev/null \
	&& grep "err  $4"'$' bochs.out >/dev/null \
	&& grep "eip  $5"'$' bochs.out >/dev/null
	then
		score=`echo 5+$score | bc`
		echo OK
	else
		echo WRONG
	fi
}

preempt() {
	# Check that alice and bob are running together
	perl -e "print 'Scheduling: '"
	(sed -e "s/^env_pop_tf/x&/" <kern/env.c; 
	echo '
	
	void
	env_pop_tf(struct Trapframe *tf)
	{
		printf("%d\n", (struct Env*)tf - envs);
		asm volatile("movl %0,%%esp\n"
			"\tpopal\n"
			"\tpopl %%es\n"
			"\tpopl %%ds\n"
			"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
			"\tiret"
			:: "g" (tf) : "memory");
		panic("iret failed");  /* mostly to placate the compiler */
	}
	
	') >kern/env-test.c
	(	
		rm kern/kernel
		gmake 'DEFS=-DTEST_ALICEBOB' 'ENV=env-test'
		ulimit -t 10
		(echo c; echo quit) | bochs-nogui 'parport1: enable=1, file="bochs.out"'
	) >/dev/null 2>/dev/null

	x=`grep '^[01]$' bochs.out`
	x=`echo $x | tr -d ' '`
	case $x in
	*01010101010101010101*)
		score=`echo 15+$score | bc`
		echo OK
		;;
	*)
		echo WRONG
	esac	
}

		
score=0

# Try all the different fault tests
gmake clean >/dev/null 2>/dev/null
fault 'Divide by zero' div0 0x0 0x0 0x800022
fault 'Breakpoint' brkpt 0x3 0x0 0x800021
fault 'Bad data segment' badds 0xd 0x8 0x800025
fault 'Read nonexistent page' pgfault_rd_nopage 0xe 0xfffc 0x800020
fault 'Read kernel-only page' pgfault_rd_noperms 0xe 0xfffd 0x800020
fault 'Write nonexistent page' pgfault_wr_nopage 0xe 0xfffe 0x800020
fault 'Write kernel-only page' pgfault_wr_noperms 0xe 0xffff 0x800020

gmake clean >/dev/null 2>/dev/null
preempt

gmake clean >/dev/null 2>/dev/null

echo "Score: $score/50"

#elif LAB >= 2

# Kernel will panic, which will drop back to debugger.
gmake clean >/dev/null 2>/dev/null
(
	sed 's/^check_boot_pgdir/x&/; s/^page_check/x&/; s/^va2pa/x&/' kern/pmap.c
	echo '

static u_long
va2pa(Pde *pgdir, u_long va)
{
	Pte *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir&PTE_P))
		return ~0;
	p = (Pte*)KADDR(PTE_ADDR(*pgdir));
	if (!(p[PTX(va)]&PTE_P))
		return ~0;
	return PTE_ADDR(p[PTX(va)]);
}
		
static void
check_boot_pgdir(void)
{
	u_long i, n;
	Pde *pgdir;

	pgdir = boot_pgdir;

	// check pages array
	n = ROUND(npage*sizeof(struct Page), BY2PG);
	for(i=0; i<n; i+=BY2PG)
		assert(va2pa(pgdir, UPAGES+i) == PADDR(pages)+i);
	
	// check envs array
	n = ROUND(NENV*sizeof(struct Env), BY2PG);
	for(i=0; i<n; i+=BY2PG)
		assert(va2pa(pgdir, UENVS+i) == PADDR(envs)+i);

	// check phys mem
	for(i=0; KERNBASE+i != 0; i+=BY2PG)
		assert(va2pa(pgdir, KERNBASE+i) == i);

	// check kernel stack
	for(i=0; i<KSTKSIZE; i+=BY2PG)
		assert(va2pa(pgdir, KSTACKTOP-KSTKSIZE+i) == PADDR(bootstack)+i);

	// check for zero/non-zero in PDEs
	for (i = 0; i < PDE2PD; i++) {
		switch (i) {
		case PDX(VPT):
		case PDX(UVPT):
		case PDX(KSTACKTOP-1):
		case PDX(UPAGES):
		case PDX(UENVS):
			assert(pgdir[i]);
			break;
		default:
			if(i >= PDX(KERNBASE))
				assert(pgdir[i]);
			else
				assert(pgdir[i]==0);
			break;
		}
	}
	printf("MAGIC STRING check_boot_pgdir() succeeded!\n");
}

void
page_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we cannot allocate a page table 
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) == 0);
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at BY2PG because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, pp0, PDMAP, 0) < 0);

	// insert pp1 at BY2PG (replacing pp2)
	assert(page_insert(boot_pgdir, pp1, BY2PG, 0) == 0);

	// should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at BY2PG
	page_remove(boot_pgdir, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at BY2PG should free it
	page_remove(boot_pgdir, BY2PG);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	printf("MAGIC STRING page_check() succeeded!\n");
}



'
) >kern/pmap-test.c

gmake 'PMAP=pmap-test'

(echo "c"; echo "quit") | bochs-nogui 'parport1: enable=1, file="bochs.out"' 

score=0

# echo -n "Page directory: "
awk 'BEGIN{printf("Page directory: ");}' </dev/null	# goddamn suns can't echo -n.
if grep "MAGIC STRING check_boot_pgdir() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

# echo -n "Page management: "
awk 'BEGIN{printf("Page management: ");}' </dev/null
if grep "MAGIC STRING page_check() succeeded!" bochs.out >/dev/null
then
	score=`echo 20+$score | bc`
	echo OK
else
	echo WRONG
fi

# echo -n "Printf: "
awk 'BEGIN{printf("Printf: ");}' </dev/null
if grep "6828 decimal is 15254 octal!" bochs.out >/dev/null
then
	score=`echo 10+$score | bc`
	echo OK
else
	echo WRONG
fi

echo "Score: $score/50"

#endif

