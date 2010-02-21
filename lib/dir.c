#if LAB >= 4

#include <inc/dirent.h>


DIR *opendir(const char *name)
{
}

int closedir(DIR *dir)
{
}

struct dirent *readdir(DIR *dir)
{
}

void rewinddir(DIR *dir)
{
}

void seekdir(DIR *dir, long ofs)
{
}

long telldir(DIR *dir)
{
}

#endif	// LAB >= 4
