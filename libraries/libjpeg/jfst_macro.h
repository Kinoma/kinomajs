.macro	defglobal name
#if __APPLE__
	.align 2
	.global	_\name
_\name:
#else
	.global \name
\name:
#endif
.endm

.macro defglobal_v7 name
#if __APPLE__
	.align 2
	.global	_\name
#else
	.func \name
	.global \name
#endif
.endm


.macro defname name
#if __APPLE__
_\name:
#else
\name:
#endif
.endm

.macro defendfunc
#if __APPLE__
	// nop
#else
	.endfunc
#endif
.endm

.macro	defend
#if __APPLE__
	// nop
#else
	.end
#endif
.endm

#if __APPLE__
.macro	submis	op1, op2, op3
	submi	\op1, \op2, \op3
.endm
#endif

.macro fpu
#if __APPLE__
	//nop
#else
	.fpu neon
#endif
.endm

.macro object_armv7a
#if __APPLE__
	//nop
#else
	.arch armv7a
	.object_arch armv7a
#endif
.endm
