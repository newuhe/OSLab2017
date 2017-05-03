#ifndef __SERIAL_H__
#define __SERIAL_H__

void initSerial(void);
void putChar(char);
void video_print(int row, int col, char c);
void video_print2(int row, int col, char c);

void init_vga(); // clear screen

#define SERIAL_PORT  0x3F8


#define SCR_WIDTH 80
#define SCR_HEIGH 25
#define RED_BLK 0x0c
#define VMEM ((uint16_t *)0xb8000)

#endif
