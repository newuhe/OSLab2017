#ifndef __SERIAL_H__
#define __SERIAL_H__

void initSerial(void);

void putChar(char);
void printc(char c);
void printd(int d);
void prints(const char *str);
void printx(unsigned int d);

void video_printc(char c);
void video_printd(int d);
void video_prints(const char *str);

void video_print(int row, int col, char c);
void video_print2(int row, int col, char c);

void waitDisk(void);
void readSect(void *dst, int offset);
void writeSect(void *src, int offset);

void init_vga(); // clear screen

#define SERIAL_PORT  0x3F8

#endif
