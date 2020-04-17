	.file	"add.c"
	.option nopic
	.globl	result
	.data
	.align	3
	.type	result, @object
	.size	result, 40
result:
	.word	1
	.word	2
	.word	3
	.word	4
	.word	5
	.word	6
	.word	7
	.word	8
	.word	9
	.word	10
	.text
	.align	1
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-32
	sd	s0,24(sp)
	addi	s0,sp,32
	sw	zero,-20(s0)
	sw	zero,-20(s0)
	j	.L2
.L3:
	lui	a5,%hi(result)
	lw	a4,-20(s0)
	slli	a4,a4,2
	addi	a5,a5,%lo(result)
	add	a5,a4,a5
	lw	a5,0(a5)
	addiw	a5,a5,10
	sext.w	a4,a5
	lui	a5,%hi(result)
	lw	a3,-20(s0)
	slli	a3,a3,2
	addi	a5,a5,%lo(result)
	add	a5,a3,a5
	sw	a4,0(a5)
	lw	a5,-20(s0)
	addiw	a5,a5,1
	sw	a5,-20(s0)
.L2:
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,4
	ble	a4,a5,.L3
	li	a5,5
	sw	a5,-20(s0)
	j	.L4
.L5:
	lui	a5,%hi(result)
	lw	a4,-20(s0)
	slli	a4,a4,2
	addi	a5,a5,%lo(result)
	add	a5,a4,a5
	lw	a5,0(a5)
	addiw	a5,a5,-5
	sext.w	a4,a5
	lui	a5,%hi(result)
	lw	a3,-20(s0)
	slli	a3,a3,2
	addi	a5,a5,%lo(result)
	add	a5,a3,a5
	sw	a4,0(a5)
	lw	a5,-20(s0)
	addiw	a5,a5,1
	sw	a5,-20(s0)
.L4:
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,9
	ble	a4,a5,.L5
	li	a5,0
	mv	a0,a5
	ld	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	main, .-main
	.ident	"GCC: (GNU) 7.2.0"
