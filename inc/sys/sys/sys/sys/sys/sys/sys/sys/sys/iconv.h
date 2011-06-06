#if LAB >= 9
#ifndef PIOS_INC_ICONV_H
#define PIOS_INC_ICONV_H

#include <types.h>

typedef int iconv_t;

iconv_t iconv_open(const char *, const char *);
size_t iconv(iconv_t cd, char **inbuf,
		size_t *inbytesleft, char **outbuf,
		size_t *outbytesleft);
int iconv_close(iconv_t);

#endif	// PIOS_INC_ICONV_H
#endif	// LAB >= 9
