# Entry point for the ROM image.
	.section .text.entry
	.global _start
_start:

	# Zero BSS.
	la	$a0, _bss_start
	la	$a1, _bss_end
	# 32-byte chunks.
	b	Lzero_btest
	subu	$a2, $a1, $a0
Lzero_bstart:
	sw	$zero, 4($a0)
	sw	$zero, 8($a0)
	sw	$zero, 12($a0)
	sw	$zero, 16($a0)
	sw	$zero, 20($a0)
	sw	$zero, 24($a0)
	sw	$zero, 28($a0)
	addiu	$a0, $a0, 32
	subu	$a2, $a1, $a0
Lzero_btest:
	slt	$a3, $a2, 32
	beql	$a3, $zero, Lzero_bstart
	sw	$zero, 0($a0)
	# 4-byte chunks.
	beq	$a0, $a1, Lzero_done
	nop
	sw	$zero, ($a0)
Lzero_word:
	addiu	$a0, 4
	bnel	$a0, $a1, Lzero_word
	sw	$zero, ($a0)
Lzero_done:

	# Set up the stack
	la	$sp, _boot_stack

	# Jump to C entry point.
	j	boot
	nop
