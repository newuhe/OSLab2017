#include "common.h"
#include "device.h"
#include "x86.h"

void kEntry(void) {
    initSerial();  // initialize serial port
    initIdt();     // initialize idt
    initIntr();    // iniialize 8259a
    initTimer();   // iniialize 8253 timer
    initSeg();     // initialize gdt, tss
    init_pcb();    // initialize process table
    init_fs();     // initialize file system
    init_vga();    // clear screen
    init_semaphore();
    loadUMain();   // load user program, enter user space

    while (1)
        ;

    assert(0);  // shoud not reach here
}
