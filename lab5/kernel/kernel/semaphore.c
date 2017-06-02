#include "device.h"
#include "x86.h"

void init_semaphore() {
    for (int i = 0; i < MAX_SEM_NUM; i++) {
        semaphore[i].inUse = 0;
    }
}

int new_semaphore() {
    for (int i = 0; i < MAX_SEM_NUM; i++) {
        if (semaphore[i].inUse == 0) {
            semaphore[i].inUse = 1;
            return i;
        }
    }
    return -1;
}

void free_semaphore(int index) { semaphore[index].inUse = 0; }
