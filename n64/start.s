# Entry point for the ROM image.
	.section .text.entry
	.global _start
_start:

	# Set up the stack
	la	$sp, _boot_stack

	# Set up global pointer
	la	$gp, _gp

	# Zero BSS.
	la	$a0, _bss_start
	la	$a1, _bss_end
	subu	$a1, $a1, $a0
	jal	bzero

	# Jump to C entry point.
	j	boot
