
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char *
getenv(const char *name)
{
	if (strcmp(name, "OMP_NUM_THREADS") == 0) {
		warn("getenv hack hack hack");
		return "4";
	}
	return NULL;
}

