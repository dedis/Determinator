
#ifndef _INC_STRING_H_
#define _INC_STRING_H_

#include <inc/types.h>

void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);

// XXX get rid of these - deprecated
void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);

#endif /* not _INC_STRING_H_ */
