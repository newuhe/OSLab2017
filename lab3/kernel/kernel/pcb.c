#include "device.h"
#include "x86.h"

extern SegDesc gdt[NR_SEGMENTS];
extern TSS tss;

void IDLE() {
    asm volatile("sti");
    asm volatile("hlt");
    assert(0);
    // waitForInterrupt();
}

void _switch_ip() { ; }
void schedule() {
    // move current process to the end of list
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

    // putChar('a' + pcb[pcb_cur].pid - PID_START);
    if (pcb_pre != -1 && pcb_pre == pcb_cur) {  // running process not change
        return;
    }

    if (pcb_cur != -1) {

        // modify tss
        tss.esp0 = (uint32_t)&pcb[pcb_cur].stack[KERNEL_STACK_SIZE];
        tss.ss0 = KSEL(SEG_KDATA);

        // modify stack register
        gdt[SEG_UCODE] =
            SEG(STA_X | STA_R, pcb_cur * PROC_MEMSZ, 0xffffffff, DPL_USER);
        gdt[SEG_UDATA] = SEG(STA_W, pcb_cur * PROC_MEMSZ, 0xffffffff, DPL_USER);
        asm volatile("pushl %eax");
        asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
        asm volatile("movw %ax, %ds");
        asm volatile("movw %ax, %es");
        asm volatile("popl %eax");

        asm volatile("movl %0, %%esp" ::"r"(&pcb[pcb_cur].tf));
        asm volatile("popl %gs");
        asm volatile("popl %fs");
        asm volatile("popl %es");
        asm volatile("popl %ds");
        asm volatile("popal");
        asm volatile(
            "addl $4, %esp");  // interrupt vector is on top of kernel stack
        asm volatile("addl $4, %esp");  // error code is on top of kernel stack



        asm volatile("iret");           // return to user space
    } else {
        IDLE();
    }
    /*
        if (pcb_pre != -1) {  // save pcb_pre state
            asm volatile(
                "pushfl;"
                "pushal;"
                "movl %%ebp, %[prebp];"
                "movl %%esp, %[presp];"
                "movl $1f, %[preip];"
                : [presp] "=m"(pcb[pcb_pre].sp), [preip] "=m"(pcb[pcb_pre].ip),
                  [prebp] "=m"(pcb[pcb_pre].bp)
                :
                : "memory");
        }
        if (pcb_cur != -1) {  // restore pnext state
            // set gdt, tss for proc
            gdt[SEG_KSTAK] = SEG(STA_W, pcb_cur * sizeof(struct ProcessTable),
                                 0xffffffff, DPL_KERN);
            asm volatile("movw %%ax, %%ss;" ::"a"(KSEL(SEG_KSTAK)));
            gdt[SEG_UCODE] =
                SEG(STA_X | STA_R, pcb_cur * PROC_MEMSZ, 0xffffffff, DPL_USER);
            gdt[SEG_UDATA] = SEG(STA_W, pcb_cur * PROC_MEMSZ, 0xffffffff,
       DPL_USER);
            asm volatile(
                "movl %[cursp], %%esp;"  // switch stack
                "movl %[curbp], %%ebp;"
                "pushl %[curip];"
                "jmp _switch_ip;"  // set eip to curip
                "1:"               // switch to next proc
                "popal;"
                "popfl;"
                :
                : [cursp] "m"(pcb[pcb_cur].sp), [curip] "m"(pcb[pcb_cur].ip),
                  [curbp] "m"(pcb[pcb_cur].bp)
                : "memory");
        } else {  // idle thread
            IDLE();
        }
    */
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
    /*
        cur_pcb_num = 0;
        current = 0;
        cur_pcb_num++;
        int i = cur_pcb_num;
        pcb[i].state = RUNNABLE;
        pcb[i].timeCount = TIMESLICE;
        pcb[i].sleepTime = 0;
        pcb[i].pid = PID_START + i;
        // ni should be 0
        ni = 0;
    */

    asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
    asm volatile("movw %ax, %ds");
    asm volatile("movw %ax, %es");
    asm volatile("movw %ax, %fs");

    asm volatile("sti");

    current = &pcb[ni];
    current->tf.ss = USEL(SEG_UDATA);
    current->tf.esp = APP_MEM_START + PROC_MEMSZ;

    asm volatile("pushfl");  // %eflags
    asm volatile("movl (%%esp), %0" : "=r"(current->tf.eflags) :);

    current->tf.cs = USEL(SEG_UCODE);
    current->tf.eip = entry;

    current->state = RUNNING;
    current->timeCount = 10;
    current->sleepTime = 0;
    current->pid = 0;

    asm volatile("movl %0, %%esp" ::"r"(&current->tf.eip));

    //    gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0x100000,       0xffffffff,
    //    DPL_USER);
    //    gdt[SEG_UDATA] = SEG(STA_W,         0x100000,       0xffffffff,
    //    DPL_USER);
    //    asm volatile("movl %0, %%eax":: "r"(USEL(SEG_UDATA)));
    //    asm volatile("movw %ax, %ds");
    //    asm volatile("movw %ax, %es");

    asm volatile("iret");  // return to user space
}
