#include "device.h"
#include "x86.h"

/* defined in <sys/syscall.h> */
#define SYS_exit 1
#define SYS_fork 2
#define SYS_write 4
#define SYS_sleep 200  // user defined

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
    asm volatile("movl %0, %%eax" ::"r"(KSEL(SEG_KDATA)));
    asm volatile("movw %ax, %ds");
    asm volatile("movw %ax, %fs");
    asm volatile("movw %ax, %es");
    asm volatile("movl %0, %%eax" ::"r"(KSEL(SEG_VIDEO)));
    asm volatile("movw %ax, %gs");

    switch (tf->irq) {
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
        default:
            assert(0);
    }
}

void sys_exit(struct TrapFrame *tf) {
    // TODO:
    uint32_t pid = pcb[pcb_cur].pid;
    if (pcb_head == -1) {
        return;
    }
    if (pcb[pcb_head].pid == pid) {
        int t = pcb_head;
        pcb_head = pcb[pcb_head].next;
        pcb[t].next = pcb_free;
        pcb_free = t;
    } else {
        int p = pcb_head, i = pcb[pcb_head].next;
        while (i != -1) {
            if (pcb[i].pid == pid) {
                pcb[p].next = pcb[i].next;
                pcb[i].next = pcb_free;
                pcb_free = i;
                break;
            }
            p = i;
            i = pcb[i].next;
        }
    }
    if (pcb[pcb_cur].pid == pid) {
        pcb_cur = -1;
        schedule();
    }
}

void sys_fork(struct TrapFrame *tf) {
    // TODO:
    int pi = new_pcb();

    // copy user space memory
    int src = APP_MEM_START + pcb_cur * PROC_MEMSZ,
        dst = APP_MEM_START + pi * PROC_MEMSZ;
    for (int i = 0; i < PROC_MEMSZ; i++) {
        *((uint8_t *)dst + i) = *((uint8_t *)src + i);
    }

    // copy kernel stack
    for (int i = 0; i < KERNEL_STACK_SIZE; i++) {
        pcb[pi].stack[i] = pcb[pcb_cur].stack[i];
    }

    pcb[pi].tf.eax = 0;                 // child process return value
    pcb[pcb_cur].tf.eax = pcb[pi].pid;  // father process return value

    pcb[pcb_cur].state = RUNNABLE;

    schedule();
}

void sys_sleep(struct TrapFrame *tf) {
    // TODO:
    // putChar('0' + tf->ebx);
    pcb[pcb_cur].sleepTime = pcb[pcb_cur].tf.ebx;
    pcb[pcb_cur].state = BLOCKED;
    schedule();
}

void sys_write(struct TrapFrame *tf) {
    //	putChar(tf->gs + '0'); 	//should be '0'
    //	putChar(tf->ds);		//should be '#'
    static int row = 0, col = 0;
    char c = '\0';

    // !!! must add offset !!!
    tf->ecx += (pcb_cur * PROC_MEMSZ);

	// ebx:file-descriptor, ecx:str, edx:len
    if (tf->ebx == 1 || tf->ebx == 2) {  // stdout & stderr
        int i;
        for (i = 0; i < tf->edx; i++) {
            c = *(char *)(tf->ecx + i);
            // putChar(c);
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
        tf->eax = tf->edx;  // return value
    } else {                // other file descriptor
        panic("sys_write not implemented");
    }
}

void syscallHandle(struct TrapFrame *tf) {
    /* 实现系统调用*/
    switch (tf->eax) {
        case SYS_exit:
            sys_exit(tf);
            break;
        case SYS_fork:
            sys_fork(tf);
            break;
        case SYS_write:
            sys_write(tf);
            break;
        case SYS_sleep:
            sys_sleep(tf);
            break;
        /**
         * TODO: add more syscall
         */
        default:
            panic("syscal not implemented");
    }
}

void timerInterruptHandle(struct TrapFrame *tf) {

//	putChar('.');

    // reduce slepp time
    int i = pcb_head;
    while (i != -1) {
        if (pcb[i].sleepTime > 0) {
            pcb[i].sleepTime--;
            if (pcb[i].sleepTime == 0) {
                pcb[i].state = RUNNABLE;
            }
        }
        i = pcb[i].next;
    }

    if (pcb_cur == -1) {
        schedule();
        return;
    }

    pcb[pcb_cur].timeCount--;
    if (pcb[pcb_cur].timeCount == 0) {
        pcb[pcb_cur].timeCount = TIMESLICE;
        pcb[pcb_cur].state = RUNNABLE;
        // putChar('x');
        schedule();
    }
}

void GProtectFaultHandle(struct TrapFrame *tf) {
    panic("GProtect Fault");
    return;
}
