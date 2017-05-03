#include "x86.h"
#include "device.h"

void initSerial(void) {
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x80);
	outByte(SERIAL_PORT + 0, 0x01);
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x03);
	outByte(SERIAL_PORT + 2, 0xC7);
	outByte(SERIAL_PORT + 4, 0x0B);
}

static inline int serialIdle(void) {
	return (inByte(SERIAL_PORT + 5) & 0x20) != 0;
}

void putChar(char ch) {
	while (serialIdle() != TRUE);
	outByte(SERIAL_PORT, ch);
}

void update_cursor(int row, int col) {
	uint16_t d = row * SCR_WIDTH + col;
	outByte(0x3d4, 0x0f);
	outByte(0x3d5, 0xff & d);
	outByte(0x3d4, 0x0e);
	outByte(0x3d5, d >> 8);
}

/* print to video segment */
void video_print(int row, int col, char c) {
	asm ("movl %0, %%edi;"			: :"r"(((80 * row + col) * 2))  :"%edi");
	asm ("movw %0, %%eax;"			: :"r"(0x0c00 | c) 				:"%eax"); // 0x0黑底,0xc红字, 字母ASCII码
	asm ("movw %%ax, %%gs:(%%edi);" : : 							:"%edi"); // 写入显存
	update_cursor(row, col);
}

/* directly write to video segment */
void video_print2(int row, int col, char c) {
	unsigned int p1, p2;
	p1 = 0xb8000 + (80 * row + col) * 2;
	p2 = p1 + 1;
	*(unsigned char*)p1 = c;
	*(unsigned char*)p2 = 0x0c;
	update_cursor(row, col);
}

// clear screen
void init_vga() {
	update_cursor(0, 0);
	for (int i = 0; i < 10 * SCR_WIDTH; i ++) {
		*(VMEM + i) = (RED_BLK << 8);
	}
}
