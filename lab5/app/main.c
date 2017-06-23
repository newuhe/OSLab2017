#include "lib.h"
#include "types.h"

int data = 0;

int uEntry(void) {
    int fd = 0;
    int i = 0;
    char tmp = 0;

    ls("/");       // 列出"/"目录下的所有文件
    ls("/boot/");  // 列出"/boot/"目录下的所有文件
    ls("/dev/");   // 列出"/dev/"目录下的所有文件
    ls("/usr/");   // 列出"/usr/"目录下的所有文件

    printf("create /usr/test and write alphabets to it\n");
    fd = open("/usr/test", O_RDWR | O_CREAT);  // 创建文件"/usr/test"

    for (i = 0; i < 512; i++) {  // 向"/usr/test"文件中写入字母表
        tmp = (char)(i % 26 + 'A');
        write(fd, (uint8_t*)&tmp, 1);
    }

    close(fd);
    ls("/usr/");       // 列出"/usr/"目录下的所有文件
    cat("/usr/test");  // 在终端中打印"/usr/test"文件的内容

    exit();

    // should not reach here
    return 0;
}
