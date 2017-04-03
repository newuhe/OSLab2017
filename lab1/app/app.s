.code32

.global start
start:
movb $0x0c, %ah                         #黑底红字

movl $((80*5+0)*2), %edi                #在第5行第0列打印
movb $72, %al                           #42为H的ASCII码
movw %ax, %gs:(%edi)                    #写显存

movl $((80*5+1)*2), %edi
movb $101, %al
movw %ax, %gs:(%edi)

movl $((80*5+2)*2), %edi
movb $108, %al
movw %ax, %gs:(%edi)

movl $((80*5+3)*2), %edi
movb $108, %al
movw %ax, %gs:(%edi)

movl $((80*5+4)*2), %edi
movb $111, %al
movw %ax, %gs:(%edi)

movl $((80*5+5)*2), %edi
movb $44, %al
movw %ax, %gs:(%edi)

movl $((80*5+6)*2), %edi
movb $0x20, %al
movw %ax, %gs:(%edi)

movl $((80*5+7)*2), %edi
movb $119, %al
movw %ax, %gs:(%edi)

movl $((80*5+8)*2), %edi
movb $111, %al
movw %ax, %gs:(%edi)

movl $((80*5+9)*2), %edi
movb $114, %al
movw %ax, %gs:(%edi)

movl $((80*5+10)*2), %edi
movb $108, %al
movw %ax, %gs:(%edi)

movl $((80*5+11)*2), %edi
movb $100, %al
movw %ax, %gs:(%edi)

movl $((80*5+12)*2), %edi
movb $33, %al
movw %ax, %gs:(%edi)

loop:
    jmp loop
