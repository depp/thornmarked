	.section uninit.stack

	.global _fault_thread_stack
	.space 8 * 1024
	.align 3
_fault_thread_stack:
