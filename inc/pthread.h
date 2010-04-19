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


// The "barrier" type is an integer holding the index
// its index in the parent's global barriers array.
// The array entry at the corresponding index holds
// the barrier's count, i.e., the number of threads
// to stop at that barrier.
typedef int pthread_barrier_t;


// We do not currently support pthread_barrierattrs.
typedef void pthread_barrierattr_t;

// Maximum number of barriers a process/thread may use
#define BARRIER_MAX 256  

int pthread_barrier_init(pthread_barrier_t * barrier, 
			 const pthread_barrierattr_t * attr, 
			 unsigned int count);

int pthread_barrier_wait(pthread_barrier_t * barrier);

int pthread_barrier_destroy(pthread_barrier_t * barrier);

#endif /* !PIOS_INC_PTHREAD_H */
