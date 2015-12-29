@
@     Copyright (C) 2010-2015 Marvell International Ltd.
@     Copyright (C) 2002-2010 Kinoma, Inc.
@
@     Licensed under the Apache License, Version 2.0 (the "License");
@     you may not use this file except in compliance with the License.
@     You may obtain a copy of the License at
@
@      http://www.apache.org/licenses/LICENSE-2.0
@
@     Unless required by applicable law or agreed to in writing, software
@     distributed under the License is distributed on an "AS IS" BASIS,
@     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@     See the License for the specific language governing permissions and
@     limitations under the License.
@
	.section .data
	.align 2
	.global ext_stubs
ext_stubs:
	.word	0
	.global num_ext_stubs
num_ext_stubs:
	.word	0

	.thumb
	.thumb_func
	.section .text
	.align 2
	.global	_call_stub
_call_stub:
	ldr	r1, =ext_stubs
	ldr	r1, [r1]
	lsl	r0, r0, #2
	add	r1, r0
	ldr	r1, [r1]
	mov	ip, r1
	pop	{r0, r1}
	bx	ip
