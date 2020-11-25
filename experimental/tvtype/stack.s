# Stacks for threads and SP tasks.
	.section uninit.stack

	.global _boot_stack
	.global _main_thread_stack
	.space 8 * 1024
	.align 3
_boot_stack:
_main_thread_stack:

	.global _idle_thread_stack
	.space 8 * 1024
	.align 3
_idle_thread_stack:
