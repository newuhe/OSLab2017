#ifndef __lib_h__
#define __lib_h__

#include "semaphore.h"

void printf(const char *format, ...);

int fork();
void sleep(int time);
void exit();

#endif
