/*
 * 6.828 file system format
 */

#define _BSD_EXTENSION
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
/* It's too hard to know who has typedefed what. */
#include <inc/types.h>
#include <inc/mmu.h>
#include <inc/fs.h>

#ifndef PLAN9
#define USED
#endif

#define nelem(x) (sizeof(x)/sizeof((x)[0]))
typedef struct Super Super;
typedef struct File File;

Super super;
int diskfd;
u_int nblock;
u_int nbitblock;
u_int nextb;

typedef struct Block Block;
struct Block
{
	u_int busy;
	u_int bno;
	u_int used;
	u_char buf[BY2BLK];
};

Block cache[16];

long
readn(int f, void *av, long n)
{
	char *a;
	long m, t;

	a = av;
	t = 0;
	while(t < n){
		m = read(f, a+t, n-t);
		if(m <= 0){
			if(t == 0)
				return m;
			break;
		}
		t += m;
	}
	return t;
}

void
flushb(Block *b)
{
	if(lseek(diskfd, b->bno*BY2BLK, 0) < 0
	|| write(diskfd, b->buf, BY2BLK) != BY2BLK){
		perror("flushb");
		fprintf(stderr, "\n");
		exit(1);
	}
}

Block*
getblk(uint bno, uint clr)
{
	int i, least;
	static int t;
	Block *b;

	if(bno >= nblock){
		fprintf(stderr, "attempt to access past end of disk bno=%d\n", bno);
		*(int*)0=0;
		exit(1);
	}

	least = -1;
	for(i=0; i<nelem(cache); i++){
		if(cache[i].bno == bno){
			b = &cache[i];
			goto out;
		}
		if(!cache[i].busy && (least==-1 || cache[i].used < cache[least].used))
			least = i;
	}

	if(least == -1){
		fprintf(stderr, "panic: block cache full\n");
		exit(1);
	}

	b = &cache[least];
	flushb(b);

	if(lseek(diskfd, bno*BY2BLK, 0) < 0
	|| readn(diskfd, b->buf, BY2BLK) != BY2BLK){
		fprintf(stderr, "read block %d: ", bno);
		perror("");
		fprintf(stderr, "\n");
		exit(1);
	}
	b->bno = bno;

out:
	if(clr)
		memset(b->buf, 0, sizeof b->buf);
	b->used = ++t;
	if(b->busy){
		fprintf(stderr, "panic: b is busy\n");
		exit(1);
	}
	b->busy = 1;
	return b;
}

void
putblk(Block *b)
{
	b->busy = 0;
}

void
opendisk(char *name)
{
	int i;
	struct stat s;
	Block *b;

	if((diskfd = open(name, O_RDWR)) < 0){
		fprintf(stderr, "open %s: ", name);
		perror("");
		fprintf(stderr, "\n");
		exit(1);
	}
	
	if(fstat(diskfd, &s) < 0){
		fprintf(stderr, "cannot stat %s: ", name);
		perror("");
		fprintf(stderr, "\n");
		exit(1);
	}

	if(s.st_size < 1024 || s.st_size > 4*1024*1024){
		fprintf(stderr, "bad disk size %d\n", s.st_size);
		exit(1);
	}

	nblock = s.st_size/BY2BLK;
	nbitblock = (nblock+BIT2BLK-1)/BIT2BLK;
	for(i=0; i<nbitblock; i++){
		b = getblk(2+i, 0);
		memset(b->buf, 0xFF, BY2BLK);
		putblk(b);
	}

	nextb = 2+nbitblock;

	super.magic = FS_MAGIC;
	super.nblocks = nblock;
	super.root.type = FTYPE_DIR;
}

void
writefile(char *name)
{
	int fd;
	char *last;
	File *f;
	int i, n, nblk;
	Block *dirb, *b, *bindir;

	if((fd = open(name, O_RDONLY)) < 0){
		fprintf(stderr, "open %s:", name);
		perror("");
		exit(1);
	}

	last = strrchr(name, '/');
	if(last)
		last++;
	else
		last = name;

	if(super.root.size > 0){
		dirb = getblk(super.root.direct[super.root.size/BY2BLK-1], 0);
		f = (File*)dirb->buf;
		for(i=0; i<FILE2BLK; i++)
			if(f[i].name[0] == 0){
				f = &f[i];
				goto gotit;
			}
	}
	// allocate new block
	dirb = getblk(nextb, 1);
	super.root.direct[super.root.size/BY2BLK] = nextb++;
	super.root.size += BY2BLK;
	f = (File*)dirb->buf;
	
gotit:
	strcpy(f->name, last);
	n = 0;
	for(nblk=0;; nblk++){
		b = getblk(nextb, 1);
		n = readn(fd, b->buf, BY2BLK);
		if(n < 0){
			fprintf(stderr, "reading %s: ", name);
			perror("");
			exit(1);
		}
		if(n == 0)
			break;
		nextb++;
		if(nblk < NDIRECT)
			f->direct[nblk] = b->bno;
		else if(nblk < NDIRECT+NINDIRECT){
			if(f->indirect == 0){
				bindir = getblk(nextb++, 1);
				f->indirect = bindir->bno;
			}else
				bindir = getblk(f->indirect, 0);
			((u_int*)bindir->buf)[nblk-NDIRECT] = b->bno;
			putblk(bindir);
		}else{
			fprintf(stderr, "%s: file too large\n", name);
			exit(1);
		}
		
		putblk(b);
		if(n < BY2BLK)
			break;
	}
	f->size = nblk*BY2BLK+n;
	putblk(dirb);
}

void
finishfs(void)
{
	int i;
	Block *b;

	for(i=0; i<nextb; i++){
		b = getblk(2+i/BIT2BLK, 0);
		((u_int*)b->buf)[(i%BIT2BLK)/32] &= ~(1<<(i%32));
		putblk(b);
	}

	// this is slow but not too slow.  i do not care
	if(nblock != nbitblock*BIT2BLK){
		b = getblk(2+nbitblock-1, 0);
		for(i=nblock%BIT2BLK; i<BIT2BLK; i++)
			((u_int*)b->buf)[i/32] &= ~(1<<(i%32));
		putblk(b);
	}

	b = getblk(1, 1);
	memmove(b->buf, &super, sizeof(Super));
	putblk(b);
}

void
flushdisk(void)
{
	int i;

	for(i=0; i<nelem(cache); i++)
		if(cache[i].used)
			flushb(&cache[i]);
}

void
usage(void)
{
	fprintf(stderr, "usage: fsformat kern/fs.img files...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int i;

	if(argc < 2)
		usage();

	opendisk(argv[1]);
	for(i=2; i<argc; i++)
		writefile(argv[i]);
	finishfs();
	flushdisk();
	exit(0);
}
