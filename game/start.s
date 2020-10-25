# Entry point for the ROM image.
	.section .text.entry
	.global _start
_start:

	# Set up the stack
	la	$sp, _boot_stack

	# Zero BSS.
	la	$a0, _bss_start
	la	$a1, _bss_end
	jal	bzero
	subu	$a1, $a1, $a0

	# Jump to C entry point.
	j	boot
	nop
