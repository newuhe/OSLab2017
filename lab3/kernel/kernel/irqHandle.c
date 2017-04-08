#include "x86.h"
#include "device.h"

#define	SYS_write	4 // defined in <sys/syscall.h>

void syscallHandle(struct TrapFrame *tf);

void timerInterruptHandle(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) {
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x20:
			timerInterruptHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}

void sys_write(struct TrapFrame *tf) {
	asm volatile("movl %0, %%eax":: "r"(KSEL(SEG_VIDEO)));
	asm volatile("movw %ax, %gs");
	static int row = 0, col = 0;
	char c = '\0';
	// ebx:file-descriptor, ecx:str, edx:len
	if (tf->ebx == 1 || tf->ebx == 2) { // stdout & stderr
		int i;
		for(i = 0; i < tf->edx; i++) {
			c = *(char *)(tf->ecx + i);
			putChar(c);
			if (c == '\n') {
				row++;
				col = 0;
				continue;
			}
			if (col == 80) {
				row++;
				col = 0;
			}
			video_print(row, col++, c);
		}
		tf->eax = tf->edx; // return value
	}
	else { // other file descriptor
		panic("sys_write not implemented");
	}
	asm volatile("int $0x20");
	asm volatile("hlt");
}

void syscallHandle(struct TrapFrame *tf) {
	/* 实现系统调用*/
	switch(tf->eax) {
		case SYS_write: sys_write(tf); break;
		/**
		 * TODO: add more syscall
		 */
		default: panic("syscal not implemented");
	}
}

void timerInterruptHandle(struct TrapFrame *tf) {
	/* 实现系统调用*/
	putChar('t');
	putChar('i');
	putChar('m');
	putChar('e');
	putChar('\n');
}


void GProtectFaultHandle(struct TrapFrame *tf){
	panic("GProtect Fault");
	return;
}
