#include "x86.h"

#define TIMER_PORT 0x40
#define FREQ_8253 1193182
#define HZ 100

/* 初始化8253可编程计时器：
 * 中断向量为0x20 */

 void initTimer() {
     int counter = FREQ_8253 / HZ;
     outByte(TIMER_PORT + 3, 0x34);
     outByte(TIMER_PORT + 0, counter % 256);
     outByte(TIMER_PORT + 0, counter / 256);
 }
