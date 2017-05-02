#include "x86.h"
#include "device.h"

/* defined in <sys/syscall.h> */
#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_write	4
#define SYS_sleep	200 // user defined

void IDLE();
void schedule();

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

void sys_exit(struct TrapFrame *tf) {
	// TODO:
}

void sys_fork(struct TrapFrame *tf) {
	// TODO:
}

void sys_sleep(struct TrapFrame *tf) {
	// TODO:
	// int sleep_time = tf->eax;
}

void sys_write(struct TrapFrame *tf) {
	//	putChar(tf->gs + '0'); 	//should be '0'
	//	putChar(tf->ds);		//should be '#'
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
}

void syscallHandle(struct TrapFrame *tf) {
	/* 实现系统调用*/
	switch(tf->eax) {
		case SYS_exit:  sys_exit(tf);  break;
		case SYS_fork:  sys_fork(tf);  break;
		case SYS_write: sys_write(tf); break;
		case SYS_sleep: sys_sleep(tf); break;
		/**
		 * TODO: add more syscall
		 */
		default: panic("syscal not implemented");
	}
}

void timerInterruptHandle(struct TrapFrame *tf) {
	putChar('0');
	if(current->timeCount == 0) {
		schedule();
	}
}


void GProtectFaultHandle(struct TrapFrame *tf){
	panic("GProtect Fault");
	return;
}

void IDLE() {
	while(1) {
		putChar('0');
		waitForInterrupt();
	}
}

void schedule() {

}
