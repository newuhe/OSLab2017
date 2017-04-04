#include "common.h"
#include "x86.h"
#include "device.h"

void asm_print(int row, int col, char c) {
	asm ("movl %0, %%edi;"			: :"r"(((80 * row + col) * 2))  :"%edi"); // 写在屏幕的第5行第0列
	asm ("movw %0, %%eax;"			: :"r"(0x0c00|c) 				:"%eax"); // 0x0黑底,0xc红字,字母ASCII码
	asm ("movw %%ax, %%gs:(%%edi);" : : 							:"%edi"); // 写入显存
}

void kEntry(void) {
	asm_print(5, 1, 'x');
	while(1);

	initSerial();// initialize serial port
	initIdt(); // initialize idt
	initIntr(); // iniialize 8259a
	initSeg(); // initialize gdt, tss
	loadUMain(); // load user program, enter user space

	while(1);
	assert(0);
}
