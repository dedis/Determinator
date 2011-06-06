#if LAB >= 9
#ifndef PIOS_INC_SEMAPHORE_H
#define PIOS_INC_SEMAPHORE_H

typedef int sem_t;

int	sem_init(sem_t *, int, unsigned);
int	sem_destroy(sem_t *);
sem_t *	sem_open(const char *, int, ...);
int	sem_close(sem_t *);
int	sem_unlink(const char *);
int	sem_post(sem_t *);
int	sem_wait(sem_t *);
int	sem_trywait(sem_t *);
int	sem_getvalue(sem_t *, int *);

#endif /* ! PIOS_INC_SEMAPHORE_H */
#endif // LAB >= 9
