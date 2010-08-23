#if LAB >= 5
/*
 * MD5.H - header file for MD5C.C
 *
 * Copyright (C) 1991-2, RSA Data Security, Inc.
 * See section "RSA License" in the file LICENSES for licensing terms.
 */

/* MD5 context. */
typedef struct {
	uint32_t           state[4];	/* state (ABCD) */
	uint32_t           count[2];	/* number of bits, modulo 2^64 (lsb
					 * first) */
	unsigned char   buffer[64];	/* input buffer */
}               MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char[16], MD5_CTX *);

#endif // LAB >= 5
