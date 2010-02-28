#if LAB >= 4

#include <inc/assert.h>
#include <inc/stdarg.h>

int
execl(const char *path, const char *arg0, ...)
{
	return execv(path, &arg0);
}

int
execv(const char *path, const char *argv[])
{
	panic("execv");
}

#endif /* LAB >= 4 */
