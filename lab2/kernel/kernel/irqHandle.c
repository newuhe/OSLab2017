#include "x86.h"
#include "device.h"

#define	SYS_write	4 // defined in <sys/syscall.h>

void syscallHandle(struct TrapFrame *tf);

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
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}

void sys_write(struct TrapFrame *tf) {
	static int row = 4, col = 1;
	char c = '\0';

	// ebx:file-descriptor, ecx:str, edx:len
	if (tf->ebx == 1 || tf->ebx == 2) { // stdout & stderr
		int i;
		for(i = 0; i < tf->edx; i++) {
			c = *(char *)(tf->ecx + i);
			putChar(c);
			if (c == '\n') {
				row++;
				col = 1;
				continue;
			}
			if (col == 80) {
				row++;
				col = 1;
			}
			video_print(row, col++, c);
		}
		tf->eax = tf->edx; // return value
	}
	else { // other file descriptor
		panic("sys_write not implemented");
	}
}

void syscallHandle(struct TrapFrame *tf) {
	/* 实现系统调用*/
	switch(tf->eax) {
		case SYS_write: sys_write(tf); break;
		default: panic("syscal not implemented");
	}
}

void GProtectFaultHandle(struct TrapFrame *tf){
	panic("GProtect Fault");
	return;
}
