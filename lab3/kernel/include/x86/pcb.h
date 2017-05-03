#ifndef __X86_PCB_H__
#define __X86_PCB_H__

#include <common.h>
#include "memory.h"

#define KERNEL_STACK_SIZE (16 << 10)  // kernel stack size (16KB)

#define PROC_MEMSZ (1 << 16)
#define APP_MEM_START 0X200000

#define MAX_PCB_NUM 20  // PCB size

#define TIMESLICE 10

#define PID_START 1000

#define IDLE_STACK 0x200000

// process state
enum { BLOCKED, DEAD, RUNNING, RUNNABLE };

struct ProcessTable {
    union {
        uint8_t stack[KERNEL_STACK_SIZE];  // kernel stack
        struct {
            uint8_t pad[KERNEL_STACK_SIZE - sizeof(struct TrapFrame)];
            struct TrapFrame tf;
        } __attribute__((packed));
    };
    int state;
    int timeCount;
    int sleepTime;
    uint32_t pid;
    int next;
};

struct ProcessTable pcb[MAX_PCB_NUM];

int pcb_head;  // allocated pcb list head
int pcb_free;  // free pcb list head
int pcb_cur;   // current runnning process

void init_pcb();
void enter_proc(uint32_t entry);
void schedule();
int new_pcb();

#endif
