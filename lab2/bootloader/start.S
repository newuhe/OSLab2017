.code16

.global start
start:

.code32
start32:

.p2align 2
gdt: # 8 bytes for each table entry, at least 1 entry
	.word 0,0 # empty entry
	.byte 0,0,0,0

gdtDesc: # 6 bytes in total
	.word (gdtDesc - gdt -1)
	.long gdt
