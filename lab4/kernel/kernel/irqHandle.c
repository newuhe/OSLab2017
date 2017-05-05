#include "device.h"
#include "x86.h"

/* defined in <sys/syscall.h> */
#define SYS_exit 1
#define SYS_fork 2
#define SYS_write 4

// user defined
#define SYS_sleep       200
#define SYS_sem_init    201
#define SYS_sem_post    202
#define SYS_sem_wait    203
#define SYS_sem_destroy 204

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
    struct ProcessTable *p, *q;
    uint32_t pid = pcb_cur->pid;

    // remove from pcb list
    if (pcb_head->pid == pid) {
        p = pcb_head;
        pcb_head = pcb_head->next;
        p->next = pcb_free;
        pcb_free = p;
    } else {
        p = pcb_head, q = pcb_head->next;
        while (q != NULL) {
            if (q->pid == pid) {
                p->next = q->next;
                q->next = pcb_free;
                pcb_free = q;
                break;
            }
            p = q;
            q = q->next;
        }
    }

    pcb_cur = NULL;
    schedule();

    // should not reach here
    assert(0);
}

void sys_fork(struct TrapFrame *tf) {
    // TODO:
    struct ProcessTable *p = new_pcb();

    // copy user space memory
    int src = APP_START + NR_PCB(pcb_cur) * PROC_MEMSZ;
    int dst = APP_START + NR_PCB(p) * PROC_MEMSZ;
    for (int i = 0; i < PROC_MEMSZ; i++) {
        *((uint8_t *)dst + i) = *((uint8_t *)src + i);
    }

    // copy kernel stack
    for (int i = 0; i < KERNEL_STACK_SIZE; i++) {
        p->stack[i] = pcb_cur->stack[i];
    }

    p->tf.eax = 0;             // child process return value
    pcb_cur->tf.eax = p->pid;  // father process return value

    pcb_cur->state = RUNNABLE;

    schedule();
}

void sys_sleep(struct TrapFrame *tf) {
    // TODO:
    // putChar('0' + tf->ebx);
    pcb_cur->sleepTime = tf->ebx;
    pcb_cur->state = BLOCKED;
    schedule();
}

void sys_write(struct TrapFrame *tf) {
    //	putChar(tf->gs + '0'); 	//should be '0'
    //	putChar(tf->ds);		//should be '#'
    static int row = 0, col = 0;
    char c = '\0';

    // !!! must add offset !!!
    tf->ecx += (NR_PCB(pcb_cur) * PROC_MEMSZ);

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

// ebx: *sem  ecx: value
void sys_sem_init(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t *sem = (sem_t*)tf->ebx;
    int value = tf->ecx;

    int i = new_semaphore();
    semaphore[i].value = value;
    *sem = i;
    tf->eax = i;
}

// ebx: *sem
void sys_sem_post(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t sem = *(sem_t*)tf->ebx;
    semaphore[sem].value++;
    if (semaphore[sem].value == 0) {
        int pid = semaphore[sem].pid;
        assert(pid == 1);
        pcb[pid].state = RUNNABLE;
        pcb[pid].timeCount = TIMESLICE;
        schedule();
    }
    tf->eax = 0;
}

// ebx: *sem
void sys_sem_wait(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t sem = *(sem_t*)tf->ebx;
    semaphore[sem].value--;
    tf->eax = 0;
    if (semaphore[sem].value < 0) {
        semaphore[sem].pid = NR_PCB(pcb_cur);
        pcb_cur->state = BLOCKED;
        schedule();
    }
}

// ebx: *sem
void sys_sem_destroy(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t *sem = (sem_t*)tf->ebx;
    free_semaphore(*sem);
    tf->eax = 0;
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
        case SYS_sem_init:
            sys_sem_init(tf);
            break;
        case SYS_sem_post:
            sys_sem_post(tf);
            break;
        case SYS_sem_wait:
            sys_sem_wait(tf);
            break;
        case SYS_sem_destroy:
            sys_sem_destroy(tf);
            break;
        /**
         * TODO: add more syscall
         */
        default:
            panic("syscal not implemented");
    }
}

void timerInterruptHandle(struct TrapFrame *tf) {
    // putChar('.');

    struct ProcessTable *p = pcb_head;
    while (p != NULL) {
        if (p->sleepTime > 0) {
            --(p->sleepTime);
            if (p->sleepTime == 0) {
                p->state = RUNNABLE;
            }
        }
        p = p->next;
    }

    if (pcb_cur == NULL) {  // IDLE
        schedule();
        return;
    }

    --(pcb_cur->timeCount);
    if (pcb_cur->timeCount == 0) {
        pcb_cur->state = RUNNABLE;
        pcb_cur->timeCount = TIMESLICE;
        // putChar('x');
        schedule();
    }
    return;
}

void GProtectFaultHandle(struct TrapFrame *tf) {
    panic("GProtect Fault");
    return;
}
