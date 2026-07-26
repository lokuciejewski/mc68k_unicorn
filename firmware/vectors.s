.section .vectors, "a"
.align 4

.global _vectors
_vectors:
.long SV_STACK_TOP
.long _start
