#include "device.h"
#include "x86.h"

void initSerial(void) {
    outByte(SERIAL_PORT + 1, 0x00);
    outByte(SERIAL_PORT + 3, 0x80);
    outByte(SERIAL_PORT + 0, 0x01);
    outByte(SERIAL_PORT + 1, 0x00);
    outByte(SERIAL_PORT + 3, 0x03);
    outByte(SERIAL_PORT + 2, 0xC7);
    outByte(SERIAL_PORT + 4, 0x0B);
}

#define SECTOR_SIZE 512

void waitDisk(void) {
    while ((inByte(0x1F7) & 0xC0) != 0x40)
        ;
}

void readSect(void *dst, int offset) {
    int i;
    waitDisk();

    outByte(0x1F2, 1);
    outByte(0x1F3, offset);
    outByte(0x1F4, offset >> 8);
    outByte(0x1F5, offset >> 16);
    outByte(0x1F6, (offset >> 24) | 0xE0);
    outByte(0x1F7, 0x20);

    waitDisk();
    for (i = 0; i < SECTOR_SIZE / 4; i++) {
        ((int *)dst)[i] = inLong(0x1F0);
    }
}

void writeSect(void *src, int offset) {
    int i;
    waitDisk();

    outByte(0x1F2, 1);
    outByte(0x1F3, offset);
    outByte(0x1F4, offset >> 8);
    outByte(0x1F5, offset >> 16);
    outByte(0x1F6, (offset >> 24) | 0xE0);
    outByte(0x1F7, 0x30);

    waitDisk();
    for (i = 0; i < SECTOR_SIZE / 4; i++) {
        outLong(0x1F0, ((uint32_t *)src)[i]);
    }
}

static inline int serialIdle(void) {
    return (inByte(SERIAL_PORT + 5) & 0x20) != 0;
}

void putChar(char ch) {
    while (serialIdle() != TRUE)
        ;
    outByte(SERIAL_PORT, ch);
}

void update_cursor(int r, int c) {
    uint16_t pos = r * 80 + c;
    outByte(0x3d4, 0x0f);
    outByte(0x3d5, 0xff & pos);
    outByte(0x3d4, 0x0e);
    outByte(0x3d5, pos >> 8);
}

/* print to video segment */
void video_print(int row, int col, char c) {
    asm("movl %0, %%edi;" : : "r"(((80 * row + col) * 2)) : "%edi");
    asm("movw %0, %%eax;"
        :
        : "r"(0x0c00 | c)
        : "%eax");  // 0x0黑底,0xc红字, 字母ASCII码
    asm("movw %%ax, %%gs:(%%edi);" : : : "%edi");  // 写入显存
    update_cursor(row, col);
}

/* directly write to video segment */
void video_print2(int row, int col, char c) {
    unsigned int p1, p2;
    p1 = 0xb8000 + (80 * row + col) * 2;
    p2 = p1 + 1;
    *(uint8_t *)p1 = c;
    *(uint8_t *)p2 = 0x0c;
    update_cursor(row, col);
}

// clear screen
void init_vga() {
    update_cursor(0, 0);
    for (int i = 0; i < 10 * 80; i++) {
        *((uint16_t *)0xb8000 + i) = (0x0c << 8);
    }
}

/* print one character */
void printc(char c) { putChar(c); }

/* print c-string */
void prints(const char *str) {
    int strsz = 0;
    int i;
    while (str[strsz] != '\0') strsz++;
    for (i = 0; i < strsz; i++) putChar(str[i]);
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
        } else {
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

void sys_write(struct TrapFrame *tf);

void video_printc(char c) {
	struct TrapFrame tf;
	tf.ebx = 1;
	tf.ecx = (uint32_t)&c;
	tf.edx = 1;
	tf.ecx -= (NR_PCB(pcb_cur) * PROC_MEMSZ);
	sys_write(&tf);
}

void video_prints(const char *str) {
    int strsz = 0;
    while (str[strsz] != '\0')
        strsz++;

	struct TrapFrame tf;
	tf.ebx = 1;
	tf.ecx = (uint32_t)str;
	tf.edx = strsz;
	tf.ecx -= (NR_PCB(pcb_cur) * PROC_MEMSZ);
	sys_write(&tf);
}

void video_printd(int d) {
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
	video_prints(buf);
}
