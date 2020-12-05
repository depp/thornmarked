# Stacks for threads and SP tasks.
	.section uninit.stack

	.global _main_thread_stack
	.space 8 * 1024
	.align 3
_main_thread_stack:

	.global _scheduler_thread_stack
	.space 8 * 1024
	.align 3
_scheduler_thread_stack:
