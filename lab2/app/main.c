#include "lib.h"
#include "types.h"

void asm_print(int row, int col, char c) {
	asm ("movl %0, %%edi;"			: :"r"(((80 * row + col) * 2))  :"%edi"); // 写在屏幕的第5行第0列
	asm ("movw %0, %%eax;"			: :"r"(0x0c00 | c) 				:"%eax"); // 0x0黑底,0xc红字,字母ASCII码
	asm ("movw %%ax, %%gs:(%%edi);" : : 							:"%edi"); // 写入显存
	while(1);
}

int uEntry(void) {
	asm_print(5, 1, 'x');

	printf("printf test begin...\n");
	printf("the answer should be:\n");
	printf("#######################################################\n");
	printf("Hello, welcome to OSlab! I'm the body of the game. ");
	printf("Bootblock loads me to the memory position of 0x100000, and Makefile also tells me that I'm at the location of 0x100000. ");
	printf("~!@#/(^&*()_+`1234567890-=...... ");
	printf("Now I will test your printf: ");
	printf("1 + 1 = 2, 123 * 456 = 56088\n0, -1, -2147483648, -1412505855, -32768, 102030\n0, ffffffff, 80000000, abcdef01, ffff8000, 18e8e\n");
	printf("#######################################################\n");
	printf("your answer:\n");
	printf("=======================================================\n");
	printf("%s %s%scome %co%s", "Hello,", "", "wel", 't', " ");
	printf("%c%c%c%c%c! ", 'O', 'S', 'l', 'a', 'b');
	printf("I'm the %s of %s. %s 0x%x, %s 0x%x. ", "body", "the game", "Bootblock loads me to the memory position of", 0x100000, "and Makefile also tells me that I'm at the location of", 0x100000);
	printf("~!@#/(^&*()_+`1234567890-=...... ");
	printf("Now I will test your printf: ");
	printf("%d + %d = %d, %d * %d = %d\n", 1, 1, 1 + 1, 123, 456, 123 * 456);
	printf("%d, %d, %d, %d, %d, %d\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
	printf("%x, %x, %x, %x, %x, %x\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
	printf("=======================================================\n");
	printf("Test end!!! Good luck!!!\n");

	while(1);
	return 0;
}
