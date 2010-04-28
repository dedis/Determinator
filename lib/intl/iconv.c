#if LAB >= 9

#include <assert.h>
#include <iconv.h>

iconv_t iconv_open(const char *tocode, const char *fromcode)
{
	panic("iconv_open() not implemented");
}

size_t iconv(iconv_t cd, char **inbuf,
		size_t *inbytesleft, char **outbuf,
		size_t *outbytesleft)
{
	panic("iconv() not implemented");
}

int iconv_close(iconv_t cd)
{
	return 0;
}

#endif	// LAB >= 9
