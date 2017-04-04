#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];
TSS tss;

void asm_print(int row, int col, char c) {
	asm ("movl %0, %%edi;"			: :"r"(((80 * row + col) * 2))  :"%edi"); // 写在屏幕的第5行第0列
	asm ("movw %0, %%eax;"			: :"r"(0x0c00 | c) 				:"%eax"); // 0x0黑底,0xc红字,字母ASCII码
	asm ("movw %%ax, %%gs:(%%edi);" : : 							:"%edi"); // 写入显存
	while(1);
}

#define SECTSIZE 512

void waitDisk(void) {
	while((inByte(0x1F7) & 0xC0) != 0x40);
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
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}

void initSeg() {
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_USER);
	gdt[SEG_UDATA] = SEG(STA_W,         0,       0xffffffff, DPL_USER);
	gdt[SEG_TSS] = SEG16(STS_T32A,   &tss,    sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	gdt[SEG_VIDEO] = SEG(STA_W,  0x0b8000,       0xffffffff, DPL_USER);
	setGdt(gdt, sizeof(gdt));

	/*
	 * 初始化TSS
	 */
	asm volatile("movl %%esp, %0": "=r"(tss.esp0));
	tss.ss0 = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	asm volatile("movw $(2 << 3), %ax");
	asm volatile("movw %ax, %ds");
	asm volatile("movw %ax, %es");
	asm volatile("movw %ax, %ss");
	asm volatile("movw %ax, %fs");
	asm volatile("movw $(6 << 3), %ax");
	asm volatile("movw %ax, %gs");
	lLdt(0);

}

void enterUserSpace(uint32_t entry) {
	/*
	 * Before enter user space
	 * you should set the right segment registers here
	 * and use 'iret' to jump to ring3
	 */

	asm volatile("pushl %0"::"r"(USEL(SEG_UDATA)));			// %ss
	asm volatile("pushl %0"::"r"(0x128 << 20));			// %esp
	asm volatile("pushfl");		// %eflags
	asm volatile("pushl %0"::"r"(USEL(SEG_UCODE)));			// %cs
	asm volatile("pushl %0"::"r"(entry));			// %eip

	//asm_print(5, 1, 'x');

	asm volatile("iret");
}

void loadUMain(void) {
	/*加载用户程序至内存*/
	struct ELFHeader *elf;
	struct ProgramHeader *ph;

	unsigned char *buf = (unsigned char *)0x5000000;
	for (int i = 0; i < 100; i ++) {
		readSect((void*)(buf + 512 * i), i + 201);
	}

	elf = (struct ELFHeader *)buf;

	/* Load each program segment */
	ph = (struct ProgramHeader *)(buf + elf->phoff);
	int i;
	for(i = 0; i < elf->phnum; ++i) {
		/* Scan the program header table, load each segment into memory */
			/* read the content of the segment from the ELF file
			 * to the memory region [VirtAddr, VirtAddr + FileSiz)
			 */
		if (ph->type == 1) {
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
		}

		ph++;
	}

	enterUserSpace(elf->entry);
}
