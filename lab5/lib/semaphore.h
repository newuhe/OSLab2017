#ifndef __semaphore_h__
#define __semaphore_h__

typedef int sem_t;

int sem_init(sem_t *sem, unsigned int value);
int sem_post(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_destroy(sem_t *sem);

#endif
