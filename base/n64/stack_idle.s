	.section uninit.stack

	.global _idle_thread_stack
	.space 8 * 1024
	.align 3
_idle_thread_stack:
