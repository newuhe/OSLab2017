#ifndef __semaphore_h__
#define __semaphore_h__

typedef int sem_t;

#define MAX_SEM_NUM 20

struct Semaphore {
    int inUse;
    int value;
    int pid;
    struct Semaphore *next;
};

struct Semaphore semaphore[MAX_SEM_NUM];

void init_semaphore();
int new_semaphore();
void free_semaphore(int index);

#endif
