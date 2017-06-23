#include "device.h"
#include "x86.h"

/* defined in <sys/syscall.h> */
#define SYS_exit 1
#define SYS_fork 2
#define SYS_write 4

// user defined
#define SYS_sleep 200
#define SYS_sem_init 201
#define SYS_sem_post 202
#define SYS_sem_wait 203
#define SYS_sem_destroy 204
#define SYS_open 205
#define SYS_read 206
#define SYS_lseek 208
#define SYS_close 209
#define SYS_remove 210
#define SYS_ls 211
#define SYS_cat 212

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
        uint8_t *buf = (uint8_t *)(tf->ecx);
        int count = tf->edx;
        my_write(FCB_list[tf->ebx].inode, buf, count, FCB_list[tf->ebx].offset);
        FCB_list[tf->ebx].offset += count;

        tf->eax = count;
    }
}

// ebx: *sem  ecx: value
void sys_sem_init(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t *sem = (sem_t *)tf->ebx;
    int value = tf->ecx;

    int i = new_semaphore();
    semaphore[i].value = value;
    *sem = i;
    tf->eax = i;
}

// ebx: *sem
void sys_sem_post(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t sem = *(sem_t *)tf->ebx;
    semaphore[sem].value++;
    if (semaphore[sem].value == 0) {
        int pid = semaphore[sem].pid;
        assert(pid == 1);
        pcb[pid].state = RUNNABLE;
        pcb[pid].timeCount = TIMESLICE;
        pcb_cur->state = RUNNABLE;
        schedule();
    }
    tf->eax = 0;
}

// ebx: *sem
void sys_sem_wait(struct TrapFrame *tf) {
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);
    sem_t sem = *(sem_t *)tf->ebx;
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
    sem_t *sem = (sem_t *)tf->ebx;
    free_semaphore(*sem);
    tf->eax = 0;
}

void sys_open(struct TrapFrame *tf) {
    // !!! must add offset !!!
    int FCB_id;
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);

    prints("[kernel] open ");
    prints((const char *)(tf->ebx));
    printc('\n');

    FCB_id = allocFCB();
    FCB_list[FCB_id].offset = 0;
    FCB_list[FCB_id].inode = my_create_file((const char *)(tf->ebx));

    tf->eax = FCB_id;
}

void sys_read(struct TrapFrame *tf) {
    uint8_t *buf = (uint8_t *)(tf->ecx);
    int count = tf->edx;
    my_read(FCB_list[tf->ebx].inode, buf, count, FCB_list[tf->ebx].offset);
    FCB_list[tf->ebx].offset += count;

    tf->eax = count;
}

void sys_lseek(struct TrapFrame *tf) {
    if (tf->edx == 0)
        FCB_list[tf->ebx].offset = 0;
    else if (tf->edx == 2)
        FCB_list[tf->ebx].offset = my_file_size("/usr/test");
    FCB_list[tf->ebx].offset += tf->ecx;
}

void sys_close(struct TrapFrame *tf) { freeFCB(tf->ebx); }

void sys_remove(struct TrapFrame *tf) {
    // not implemented
}

void sys_ls(struct TrapFrame *tf) {
    // !!! must add offset !!!
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);

    prints("[kernel] ls ");
    prints((const char *)(tf->ebx));
    printc('\n');

    video_prints("[kernel] ls ");
    video_prints((const char *)(tf->ebx));
    video_printc('\n');

    my_ls((const char *)(tf->ebx));
}

void sys_cat(struct TrapFrame *tf) {
    // !!! must add offset !!!
    tf->ebx += (NR_PCB(pcb_cur) * PROC_MEMSZ);

    prints("[kernel] cat ");
    prints((const char *)(tf->ebx));
    printc('\n');

    video_prints("[kernel] cat ");
    video_prints((const char *)(tf->ebx));
    video_printc('\n');

    my_cat((const char *)(tf->ebx));
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
        case SYS_open:
            sys_open(tf);
            break;
        case SYS_read:
            sys_read(tf);
            break;
        case SYS_lseek:
            sys_lseek(tf);
            break;
        case SYS_close:
            sys_close(tf);
            break;
        case SYS_remove:
            sys_remove(tf);
            break;
        case SYS_ls:
            sys_ls(tf);
            break;
        case SYS_cat:
            sys_cat(tf);
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
return;
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
