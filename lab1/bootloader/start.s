.code16
.global start
start:
	cli                             #关闭中断
	inb $0x92, %al                  #启动A20总线
	orb $0x02, %al
	outb %al, $0x92
	data32 addr32 lgdt gdtDesc      #加载GDTR
	movl %cr0, %eax                 #启动保护模式
	orb $0x01, %al
	movl %eax, %cr0
	data32 ljmp $0x08, $start32     #长跳转切换至保护模式

.code32
start32:
	# ...                           #初始化DS ES FS GS SS 初始化栈顶指针ESP
	# Set up the protected-mode data segment registers
	movw    $(2 << 3), %ax
	movw    %ax, %ds          # %DS = %AX
	movw    %ax, %es          # %ES = %AX
	movw    %ax, %ss          # %SS = %AX
	movw    %ax, %fs          # %FS = %AX
	movw    $(3 << 3), %ax
	movw    %ax, %gs          # %GS = %AX

	# Set up a stack for C code.
	movl $0, %ebp
	movl $(128 << 20), %esp

	jmp bootMain                    #跳转至bootMain函数 定义于boot.c


.p2align 2
gdt:
	.word 0,0                       #GDT第一个表项必须为空
	.byte 0,0,0,0

	.word 0xffff,0                  #代码段描述符
	.byte 0,0x9a,0xcf,0

	.word 0xffff,0                  #数据段描述符
	.byte 0,0x92,0xcf,0

	.word 0xffff,0x8000             #视频段描述符
	.byte 0x0b,0x92,0xcf,0

gdtDesc:
	.word (gdtDesc - gdt -1)
	.long gdt
