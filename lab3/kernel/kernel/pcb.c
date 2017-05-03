#include "device.h"
#include "x86.h"

extern SegDesc gdt[NR_SEGMENTS];
extern TSS tss;

void IDLE() {
    asm volatile("movl %0, %%esp;" ::"i"(IDLE_STACK));
    asm volatile("sti");
    waitForInterrupt();
}

void schedule() {
    // move current process to the end of the list
    if (pcb_cur != -1) {
        int i = pcb[pcb_cur].next;
        if (i != -1) {
            pcb[pcb_cur].next = -1;
            pcb_head = i;
            while (pcb[i].next != -1) {
                i = pcb[i].next;
            }
            pcb[i].next = pcb_cur;
        }
    }

    // find next RUNNABLE process
    int pcb_pre = pcb_cur;
    pcb_cur = -1;
    int i = pcb_head;
    while (i != -1) {
        if (pcb[i].state == RUNNABLE) {
            pcb[i].state = RUNNING;
            pcb_cur = i;
            break;
        }
        i = pcb[i].next;
    }

    // current running process doesn't change, return directly
    if (pcb_pre != -1 && pcb_pre == pcb_cur) {
        putChar('0' + pcb[pcb_cur].pid - PID_START);
        return;
    }

    if (pcb_cur == -1) { // IDLE process
        putChar('~');
        IDLE();
    } else { // load next process
        putChar('0' + pcb[pcb_cur].pid - PID_START);

        // modify tss
        tss.esp0 = (uint32_t)&pcb[pcb_cur].stack[KERNEL_STACK_SIZE];
        tss.ss0 = KSEL(SEG_KDATA);

        // modify stack register
        gdt[SEG_UCODE] = SEG(STA_X | STA_R, pcb_cur * PROC_MEMSZ, 0xffffffff, DPL_USER);
        gdt[SEG_UDATA] = SEG(STA_W,         pcb_cur * PROC_MEMSZ, 0xffffffff, DPL_USER);
        asm volatile("pushl %eax"); // save eax
        asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
        asm volatile("movw %ax, %ds");
        asm volatile("movw %ax, %es");
        asm volatile("popl %eax");

        // restore process info
        asm volatile("movl %0, %%esp" ::"r"(&pcb[pcb_cur].tf));
        asm volatile("popl %gs");
        asm volatile("popl %fs");
        asm volatile("popl %es");
        asm volatile("popl %ds");
        asm volatile("popal");
        asm volatile("addl $4, %esp");
        asm volatile("addl $4, %esp");

        // return to user space
        asm volatile("iret");
    }
}

static void add_to_list(int new_pcb) {
    int j = pcb_head;
    if (pcb_head == -1) {
        pcb_head = new_pcb;
    } else {
        while (pcb[j].next != -1) {
            j = pcb[j].next;
        }
        pcb[j].next = new_pcb;
    }
}

int new_pcb() {
    if (pcb_free == -1) {
        return -1;
    }
    int i = pcb_free;
    pcb_free = pcb[pcb_free].next;
    pcb[i].next = -1;
    pcb[i].state = RUNNABLE;
    pcb[i].timeCount = TIMESLICE;
    pcb[i].sleepTime = 0;
    pcb[i].pid = PID_START + i;
    add_to_list(i);
    return i;
}

void init_pcb() {
    for (int i = 0; i < MAX_PCB_NUM; i++) {
        pcb[i].next = i + 1;
    }
    pcb[MAX_PCB_NUM - 1].next = -1;
    pcb_free = 0;
    pcb_head = -1;
    pcb_cur = -1;
}

void enter_proc(uint32_t entry) {
    int ni = new_pcb();

    pcb_cur = ni;

    asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
    asm volatile("movw %ax, %ds");
    asm volatile("movw %ax, %es");
    asm volatile("movw %ax, %fs");

    pcb[ni].tf.ss = USEL(SEG_UDATA);
    pcb[ni].tf.esp = APP_MEM_START + PROC_MEMSZ;

    asm volatile("sti");
    asm volatile("pushfl");  // %eflags
    asm volatile("cli");

    asm volatile("movl (%%esp), %0" : "=r"(pcb[ni].tf.eflags) :);

    pcb[ni].tf.cs = USEL(SEG_UCODE);
    pcb[ni].tf.eip = entry;

    pcb[ni].state = RUNNING;
    pcb[ni].timeCount = TIMESLICE;
    pcb[ni].sleepTime = 0;
    pcb[ni].pid = PID_START;

    // return to user space
    asm volatile("movl %0, %%esp" ::"r"(&pcb[ni].tf.eip));
    asm volatile("iret");
}
