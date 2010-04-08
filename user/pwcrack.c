// Brute-force MD5-based "password cracker":
// exhaustively searches for a short ASCII string
// whose MD5 hash yields a given hash output.

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/string.h>
#include <inc/unistd.h>
#include <inc/assert.h>
#include <inc/syscall.h>

#include "md5.c"	// Bad practice, but hey, it's easy...

#define MINCHR		(' ')	// Minimum ASCII value a string may contain
#define MAXCHR		('~')	// Maximum ASCII value a string may contain

#define MAXLEN		10
#define BLOCKLEN	2

#define hexdig(c)	((c) >= '0' && (c) <= '9' ? (c) - '0' : \
			 (c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
			 (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : -1)


int found;
char out[MAXLEN+1];


void
usage()
{
	fprintf(stderr, "Usage: pwcrack <md5-hash>\n");
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
		if (++str[lo] <= MAXCHR)
			return 0;	// no carry; we're done.
		str[lo] = MINCHR;	// wrap around
		lo++;			// carry out to next character
	}
	return 1;			// carry out to hi position
}

// Search all strings of length 'len' for one that hashes to 'hash'.
int
search(uint8_t *str, int len, int lo, int hi, const unsigned char *hash)
{
	assert(lo < hi);
	assert(hi <= len);

	do {
		unsigned char h[16];
		MD5_CTX ctx;
		MD5Init(&ctx);
		MD5Update(&ctx, str, len);
		MD5Final(h, &ctx);
		if (memcmp(h, hash, 16) == 0) {
			strcpy(out, (char*)str);
			return found = 1;
		}
	} while (!incstr(str, lo, hi));
	return 0;	// no match at this string length
}

// Like 'search', but do in parallel across 2 nodes with 2 threads each
int psearch(uint8_t *str, int len, const unsigned char *hash)
{
	if (len <= BLOCKLEN)
		return search(str, len, 0, len, hash);

	// Iterate over blocks, searching 4 blocks at a time in parallel
	int done = 0;
	do {
		// Child numbers to use when forking off workers:
		// node number in bits 15-8, intra-node child number in 7-0.
		static int child[4] = {0x0101,0x0102,0x0203,0x0204};

		int i;
		for (i = 0; i < sizeof(child)/sizeof(child[0]); i++) {
			if (!tfork(child[i])) {
				cprintf("child %x: search from '%s'\n",
					child[i], str);
				search(str, len, 0, BLOCKLEN, hash);
				sys_ret();
			}
			done |= incstr(str, BLOCKLEN, len);
		}
		for (i = 0; i < sizeof(child)/sizeof(child[0]); i++)
			tjoin(child[i]);	// collect results
		if (found)
			return 1;
	} while (!done);
	return 0;	// no match at this string length
}

int
main(int argc, char **argv)
{
	if (argc != 2 || strlen(argv[1]) != 16*2)
		usage();

	// Decode the MD5 hash from hex
	unsigned char hash[16];
	int i;
	for (i = 0; i < 16; i++) {
		int hi = hexdig(argv[1][i*2+0]);
		int lo = hexdig(argv[1][i*2+1]);
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

