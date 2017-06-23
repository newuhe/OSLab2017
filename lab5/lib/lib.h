#ifndef __lib_h__
#define __lib_h__

#include "semaphore.h"

void printf(const char *format, ...);

int fork();
void sleep(int time);
void exit();

int open(char *path, int flags);
int read(int fd, void *buffer, int size);
int write(int fd, void *buffer, int size);
int lseek(int fd, int offset, int whence);
int close(int fd);
int remove(char *path);
void ls(char *path);
void cat(char *path);

#define O_RDWR 0x1
#define O_CREAT 0x2

#endif
