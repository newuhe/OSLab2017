#include "boot.h"

#define SECTSIZE 512

/*
void asm_print(int row, int col, char c) {
	asm ("movl %0, %%edi;"			: :"r"(((80 * row + col) * 2))  :"%edi"); // 写在屏幕的第5行第0列
	asm ("movw %0, %%eax;"			: :"r"(0x0c00|c) 				:"%eax"); // 0x0黑底,0xc红字,字母ASCII码
	asm ("movw %%ax, %%gs:(%%edi);" : : 							:"%edi"); // 写入显存
	while(1);
}

*/

void bootMain(void) {
	// int a=10, b;
    // asm ("movl %1, %%eax;movl %%eax, %0;":"=r"(b):"r"(a):"%eax"); /* clobbered register */
	// asm_print(5, 2, 'x');
	/* 加载内核至内存，并跳转执行 */
	// while (1);

	struct ELFHeader *elf;
	struct ProgramHeader *ph;

	unsigned char *buf = (unsigned char *)0x4000000;
	for (int i = 0; i < 200; i ++) {
		readSect((void*)(buf + 512 * i), i + 1);
	}

	elf = (struct ELFHeader *)buf;
//	asm_print(5, 0, (elf->entry));

	/* Load each program segment */
	ph = (struct ProgramHeader *)(buf + elf->phoff);
	int i;
	for(i = 0; i < 2; ++i) {
		/* Scan the program header table, load each segment into memory */
			/* read the content of the segment from the ELF file
			 * to the memory region [VirtAddr, VirtAddr + FileSiz)
			 */
		//if (ph->type == 1) {
			unsigned int p = ph->vaddr, q = ph->off;
			while (q < ph->vaddr + ph->filesz) {
				*(unsigned char*)p = *(unsigned char*)(buf + q);
				q++;
				p++;
			}

			/* zero the memory region [VirtAddr + FileSiz, VirtAddr + MemSiz) */
			while (q < ph->vaddr + ph->memsz) {
				*(unsigned char*)p = 0;
				q++;
				p++;
			}
		//}

		ph++;
	}

	void (*entry)(void);
	entry = (void*)(elf->entry);
	entry();

}

void waitDisk(void) { // waiting for disk
	while((inByte(0x1F7) & 0xC0) != 0x40);
}

void readSect(void *dst, int offset) { // reading a sector of disk
	int i;
	waitDisk();
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}
