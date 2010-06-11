// Brute-force MD5-based "password cracker":
// exhaustively searches for a short ASCII string
// whose MD5 hash yields a given hash output.
// Similar to pwcrack.c, but single-node only,
// and uses pthreads-compatible bench.h for benchmarking.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <inc/bench.h>

#include "md5.c"	// Bad practice, but hey, it's easy...

#define MINCHR		(' ')	// Minimum ASCII value a string may contain
#define MAXCHR		('~')	// Maximum ASCII value a string may contain

#define MAXTHREADS	16
#define MAXLEN		10
#define BLOCKLEN	3

#define hexdig(c)	((c) >= '0' && (c) <= '9' ? (c) - '0' : \
			 (c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
			 (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : -1)


int nthreads;

int found;
char out[MAXLEN+1];


void
usage()
{
	fprintf(stderr, "Usage: pwcrack <nthreads> <md5-hash>\n");
	exit(1);
}

// Increment string 'str' lexicographically,
// starting with the "least significant character" at position 'lo',
// and ending just before limit character position 'hi',
// returning 1 when all characters wrap around to MINCHR.
int
incstr(uint8_t *str, int lo, int hi)
{
	while (lo < hi) {
		assert(str[lo] >= MINCHR && str[lo] <= MAXCHR);
		if (++str[lo] <= MAXCHR)
			return 0;	// no carry; we're done.
		str[lo] = MINCHR;	// wrap around
		lo++;			// carry out to next character
	}
	return 1;			// carry out to hi position
}

typedef struct search_args {
	uint8_t str[MAXLEN+1];
	int len, lo, hi;
	const unsigned char *hash;
} search_args;

// Search all strings of length 'len' for one that hashes to 'hash'.
void *
search(void *args)
{
	search_args *a = args;
	assert(a->lo < a->hi);
	assert(a->hi <= a->len);

	//cprintf("searching strings starting from '%s'\n", a->str);
	do {
		unsigned char h[16];
		//cprintf("checking '%s'\n", a->str);
		MD5_CTX ctx;
		MD5Init(&ctx);
		MD5Update(&ctx, a->str, a->len);
		MD5Final(h, &ctx);
		if (memcmp(h, a->hash, 16) == 0) {
			strcpy(out, (char*)a->str);
			found = 1;
			return NULL;
		}
	} while (!incstr(a->str, a->lo, a->hi));
	return NULL;
}

// Like 'search', but do in parallel across 2 nodes with 2 threads each
int psearch(uint8_t *str, int len, const unsigned char *hash)
{
	if (len <= BLOCKLEN) {
		search_args a = { {0}, len, 0, len, hash };
		strcpy((char*)a.str, (char*)str);
		search(&a);
		return found;
	}

	// Iterate over blocks, searching 4 blocks at a time in parallel
	int done = 0;
	do {
		int i;
		search_args a[nthreads];
		for (i = 0; i < nthreads; i++) {
			strcpy((char*)a[i].str, (char*)str);
			//cprintf("forking child to check '%s'\n", str);
			a[i].len = len;
			a[i].lo = 0;
			a[i].hi = BLOCKLEN;
			a[i].hash = hash;
			bench_fork(i, search, &a[i]);
			done |= incstr(str, BLOCKLEN, len);
		}
		for (i = 0; i < nthreads; i++)
			bench_join(i);	// collect results
		if (found)
			return 1;
	} while (!done);
	return 0;	// no match at this string length
}

int
main(int argc, char **argv)
{
	if (argc != 3 || (nthreads = atoi(argv[1])) <= 0
			|| strlen(argv[2]) != 16*2)
		usage();
	assert(nthreads > 0);

	// Decode the MD5 hash from hex
	unsigned char hash[16];
	int i;
	for (i = 0; i < 16; i++) {
		int hi = hexdig(argv[2][i*2+0]);
		int lo = hexdig(argv[2][i*2+1]);
		if (hi < 0 || lo < 0)
			usage();
		hash[i] = (hi << 4) | lo;
	}

	// Search all strings of length 1, then of length 2, ...
	int len;
	for (len = 1; len < MAXLEN; len++) {
		cprintf("Searching strings of length %d\n", len);
		uint8_t str[len+1];
		str[len] = 0;		// Null terminate for printing match
		memset(str, MINCHR, len);
		if (psearch(str, len, hash)) {
			printf("Match: '%s'\n", out);
			exit(0);
		}
	}
	cprintf("not found\n");
	return 1;
}

