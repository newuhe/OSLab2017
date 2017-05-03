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
    struct ProcessTable *p;

    // move current process to the end of the list
    if (pcb_cur != NULL) {
        p = pcb_cur->next;
        if (p != NULL) {
            pcb_cur->next = NULL;
            pcb_head = p;
            while (p->next != NULL) {
                p = p->next;
            }
            p->next = pcb_cur;
        }
    }

    // find next RUNNABLE process
    pcb_cur = NULL;
    p = pcb_head;
    while (p != NULL) {
        if (p->state == RUNNABLE) {
            p->state = RUNNING;
            pcb_cur = p;
            break;
        }
        p = p->next;
    }

    // load next process
    if (pcb_cur == NULL) { // IDLE process
        putChar('~');
        IDLE();
    } else {
        putChar('0' + pcb_cur->pid - PID_START);

        // modify tss
        tss.esp0 = (uint32_t)&(pcb_cur->stack[KERNEL_STACK_SIZE]);
        tss.ss0  = KSEL(SEG_KDATA);

        // modify stack register
        gdt[SEG_UCODE] = SEG(STA_X | STA_R, NR_PCB(pcb_cur) * PROC_MEMSZ, 0xffffffff, DPL_USER);
        gdt[SEG_UDATA] = SEG(STA_W,         NR_PCB(pcb_cur) * PROC_MEMSZ, 0xffffffff, DPL_USER);
        asm volatile("pushl %eax"); // save eax
        asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
        asm volatile("movw %ax, %ds");
        asm volatile("movw %ax, %es");
        asm volatile("popl %eax");

        // restore process info
        asm volatile("movl %0, %%esp" ::"r"(&pcb_cur->tf));
        asm volatile("popl %gs");
        asm volatile("popl %fs");
        asm volatile("popl %es");
        asm volatile("popl %ds");
        asm volatile("popal");  // Attention! will change all registers
        asm volatile("addl $4, %esp");
        asm volatile("addl $4, %esp");

        // return to user space
        asm volatile("iret");
    }
}

static void add_to_list(struct ProcessTable *new_pcb) {
    if (pcb_head == NULL) {
        pcb_head = new_pcb;
    } else {
        struct ProcessTable * p = pcb_head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = new_pcb;
    }
}

struct ProcessTable * new_pcb() {
    if (pcb_free == NULL) {
        assert(0);      // should not reach here
        return NULL;
    }
    struct ProcessTable * p = pcb_free;
    pcb_free = pcb_free->next;
    p->next = NULL;
    p->sleepTime = 0;
    p->pid = PID_START + NR_PCB(p);
    p->state = RUNNABLE;
    p->timeCount = TIMESLICE;
    add_to_list(p);
    return p;
}

void init_pcb() {
    for (int i = 0; i < MAX_PCB_NUM - 1; i++) {
        pcb[i].next = &pcb[i + 1];
    }
    pcb[MAX_PCB_NUM - 1].next = NULL;
    pcb_free = &pcb[0];
    pcb_head = NULL;
    pcb_cur = NULL;
}

void enter_proc(uint32_t entry) {
    struct ProcessTable * p = new_pcb();

    pcb_cur = p;

    asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
    asm volatile("movw %ax, %ds");
    asm volatile("movw %ax, %es");
    asm volatile("movw %ax, %fs");

    p->tf.ss = USEL(SEG_UDATA);
    p->tf.esp = APP_START + PROC_MEMSZ;

    asm volatile("sti");
    asm volatile("pushfl");  // %eflags
    asm volatile("cli");

    asm volatile("movl (%%esp), %0" : "=r"(p->tf.eflags) :);

    p->tf.cs = USEL(SEG_UCODE);
    p->tf.eip = entry;

    p->state = RUNNING;
    p->timeCount = TIMESLICE;
    p->sleepTime = 0;
    p->pid = PID_START;

    // return to user space
    asm volatile("movl %0, %%esp" ::"r"(&p->tf.eip));
    asm volatile("iret");
}
