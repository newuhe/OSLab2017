.code32

.global start
start:
movl $((80*5+0)*2), %edi                #在第5行第0列打印
movb $0x0c, %ah                         #黑底红字
movb $42, %al                           #42为H的ASCII码
movw %ax, %gs:(%edi)                    #写显存

loop:
    jmp loop
