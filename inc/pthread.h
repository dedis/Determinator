// Minimal version of the pthreads API for PIOS
#ifndef PIOS_INC_PTHREAD_H
#define PIOS_INC_PTHREAD_H

#include <types.h>


// A "pthread" in PIOS is just a child process number.
typedef int pthread_t;

// We don't support any pthread_attrs currently...
typedef void pthread_attr_t;


// Fork and join threads
int pthread_create(pthread_t *out_thread, const pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t th, void **out_exitval);


#endif /* !PIOS_INC_PTHREAD_H */
