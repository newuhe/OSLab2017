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

void printc(char c) {
    syscall(SYS_write, 1, (uint32_t)&c, 1);
    return;
}

void prints(const char* str) {
    int strsz = 0;
    while (str[strsz] != '\0')
        strsz++;
    syscall(SYS_write, 1, (uint32_t)str, strsz);
}

void printd(unsigned int d, int base) {
    char buf[100];
    int strsz = 0;
    if (d == 0) {
        printc('0');
        return;
    }
    if (d == 0x80000000) {
        if (base == 10)
            prints("-2147483648");
        else
            prints("80000000");
        return;
    }
    if (d < 0 && base == 10) {
        d = -d;
        printc('-');
    }
    while (d) {
        if (d % base >= 10) {
            buf[strsz++] = d % base - 10 + 'a';
        }
        else {
            buf[strsz++] = d % base + '0';
        }
        d /= base;
    }
    for (int i = 0, j = strsz - 1; i < j; i++, j--) {
        char tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
    }
    syscall(SYS_write, 1, (uint32_t)buf, strsz);
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
                case 'd': printd(va_arg(ap, int), 10);   break;
                case 's': prints(va_arg(ap, char*)); break;
                case 'c': printc(va_arg(ap, int));  break;
                case 'x': printd(va_arg(ap, int), 16);  break;
    	    }
    	}
    	else {
            printc(*str);
        }
    	str++;
    }
    va_end(ap);
}
