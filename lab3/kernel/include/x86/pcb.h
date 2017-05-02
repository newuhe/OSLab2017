#ifndef __X86_PCB_H__
#define __X86_PCB_H__

enum { BLOCKED, DEAD, RUNNING, RUNNABLE };


#include <common.h>

#define MAX_STACK_SIZE (4 << 10)  // kernel stack size (4KB 4096B)
#define MAX_PCB_NUM    20         // PCB size

#define TIMESLICE 10

#define PID_START 1000

typedef struct stackframe{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, xxx, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, esp, ss;
} stackframe;

struct ProcessTable {
    uint32_t stack[MAX_STACK_SIZE]; // kernel stack
    struct stackframe tf;
	int save[100];
    int state;
    int timeCount;
    int sleepTime;
    uint32_t pid;
    int next;
};

struct ProcessTable *current;	// current running process

struct ProcessTable pcb[MAX_PCB_NUM];

void init_pcb(uint32_t entry);


#endif
