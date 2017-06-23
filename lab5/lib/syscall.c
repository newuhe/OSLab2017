#include "lib.h"
#include "types.h"
#include <stdarg.h>

/* defined in <sys/syscall.h> */
#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_write	4

// user defined
#define SYS_sleep       200
#define SYS_sem_init    201
#define SYS_sem_post    202
#define SYS_sem_wait    203
#define SYS_sem_destroy 204

#define	SYS_open	205
#define	SYS_read	206
#define	SYS_lseek	208
#define	SYS_close	209
#define	SYS_remove	210
#define	SYS_ls	211
#define	SYS_cat	212

/*
 * io lib here
 * 库函数写在这
 */
int32_t syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
	int32_t ret = 0;

	asm volatile("int $0x80": "=a"(ret) : "a"(eax), "b"(ebx), "c"(ecx), "d"(edx));

	return ret;
}

/* print one character */
void printc(char c) {
    syscall(SYS_write, 1, (uint32_t)&c, 1);
    return;
}

/* print c-string */
void prints(const char* str) {
    int strsz = 0;
    while (str[strsz] != '\0')
        strsz++;
    syscall(SYS_write, 1, (uint32_t)str, strsz);
}

/* print decimal */
void printd(int d) {
    char buf[100];
    int strsz = 0;
    if (d == 0) {
        prints("0");
        return;
    }
    if (d == 0x80000000) {
        prints("-2147483648");
        return;
    }
    if (d < 0) {
        prints("-");
        d = -d;
    }
    while (d) {
        buf[strsz++] = d % 10 + '0';
        d /= 10;
    }
    for (int i = 0, j = strsz - 1; i < j; i++, j--) {
        char tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
    }
	buf[strsz] = '\0';
	prints(buf);
}

/* print hex integer */
void printx(unsigned int d) {
    char buf[100];
    int strsz = 0;
    if (d == 0) {
        prints("0");
        return;
    }
    while (d) {
        if (d % 16 >= 10) {
            buf[strsz] = d % 16 - 10 + 'a';
        }
        else {
            buf[strsz] = d % 16 + '0';
        }
        d /= 16;
        strsz++;
    }
    for (int i = 0, j = strsz - 1; i < j; i++, j--) {
        char tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
    }
	buf[strsz] = '\0';
	prints(buf);
}

void printf(const char *str, ...) {
    char token;
    va_list ap;
    va_start(ap, str);
    if (str == 0)
        return;
    while(*str != '\0') {
    	if(*str == '%') {
    	    token = *++str;
    	    switch (token) {
                case 'd': printd(va_arg(ap, int));   break;
                case 's': prints(va_arg(ap, char*)); break;
                case 'c': printc(va_arg(ap, int));   break;
                case 'x': printx(va_arg(ap, int));   break;
    	    }
    	}
    	else {
            printc(*str);
        }
    	str++;
    }
    va_end(ap);
}

int fork() {
	// TODO:
	return syscall(SYS_fork, 1, 1, 1);
}

void exit() {
	// TODO:
	syscall(SYS_exit, 1, 1, 1);
}

void sleep(int time_) {
	// TODO:
	syscall(SYS_sleep, time_, 1, 1);
}

int sem_init(sem_t *sem, unsigned int value) {
	return syscall(SYS_sem_init, (uint32_t)sem, value, 1);
}

int sem_post(sem_t *sem) {
	return syscall(SYS_sem_post, (uint32_t)sem, 1, 1);
}

int sem_wait(sem_t *sem) {
	return syscall(SYS_sem_wait, (uint32_t)sem, 1, 1);
}

int sem_destroy(sem_t *sem) {
	return syscall(SYS_sem_destroy, (uint32_t)sem, 1, 1);
}

int open(char *path, int flags) {
	return syscall(SYS_open, (uint32_t)path, flags, 1);
}

int read(int fd, void *buffer, int size) {
	return syscall(SYS_read, fd, (uint32_t)buffer, size);
}

int write(int fd, void *buffer, int size) {
	return syscall(SYS_write, fd, (uint32_t)buffer, size);
}

int lseek(int fd, int offset, int whence) {
	return syscall(SYS_lseek, fd, offset, whence);
}

int close(int fd) {
	return syscall(SYS_close, fd, 1, 1);
}

int remove(char *path) {
	return syscall(SYS_remove, (uint32_t)path, 1, 1);
}

void ls(char *path) {
	syscall(SYS_ls, (uint32_t)path, 1, 1);
}

void cat(char *path) {
	syscall(SYS_cat, (uint32_t)path, 1, 1);
}
