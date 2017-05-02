#include "device.h"
#include "x86.h"

extern TSS tss;
extern SegDesc gdt[NR_SEGMENTS];

int proc_head = -1, proc_next = -1;
volatile int proc_cur = -1;

void init_pcb(uint32_t entry) {
    for (int i = 0; i < MAX_PCB_NUM - 1; i++) {
        pcb[i].next = i + 1;
    }
    pcb[MAX_PCB_NUM - 1].next = -1;
    proc_next = 0;

    asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
    asm volatile("movw %ax, %ds");
    asm volatile("movw %ax, %es");
    asm volatile("movw %ax, %fs");

    current = &pcb[0];
    current->tf.ss = USEL(SEG_UDATA);
    current->tf.esp = 128 << 20;
    current->tf.eflags = 0;
    current->tf.cs = USEL(SEG_UCODE);
    current->tf.eip = entry;

    current->state = RUNNING;
    current->timeCount = 10;
    current->sleepTime = 0;
    current->pid = 0;

    asm volatile("movl %0, %%esp" ::"r"(&current->tf.eip));

    asm volatile("sti");
    asm volatile("iret");  // return to user space
}

int add_proc() {
    if (proc_next == -1) {
        return -1;
    }
    int i = proc_next;
    int j = proc_head;
    proc_next = pcb[proc_next].next;
    pcb[i].next = -1;
    pcb[i].state = RUNNABLE;
    pcb[i].timeCount = TIMESLICE;
    pcb[i].sleepTime = 0;
    pcb[i].pid = PID_START + i;
    if (proc_head == -1) {
        proc_head = i;
    } else {
        while (pcb[j].next != -1) {
            j = pcb[j].next;
        }
        pcb[j].next = i;
    }
    return i;
}
