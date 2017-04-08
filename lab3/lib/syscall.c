#include "lib.h"
#include "types.h"
#include<stdarg.h>

#define	SYS_write	4 // defined in <sys/syscall.h>

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

void printf(const char *str, ...)
{
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
