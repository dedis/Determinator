
#ifndef _INC_STRING_H_
#define _INC_STRING_H_

#include <inc/types.h>

int		strlen(const char*);
char*		strcpy(char*, const char*);
int		strcmp(const char*, const char*);
const char*	strchr(const char*, char);

void *		memset(void *, int, size_t);
void *		memcpy(void *, const void *, size_t);

#endif /* not _INC_STRING_H_ */
