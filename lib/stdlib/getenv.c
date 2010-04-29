
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char *
getenv(const char *name)
{
	if (strcmp(name, "OMP_NUM_THREADS") == 0) {
		char *str = "8";
		warn("getenv HACK HACK HACK: returning %s", str);
		return str;
	}
	return NULL;
}

