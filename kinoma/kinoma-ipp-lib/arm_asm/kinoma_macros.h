//
//     Copyright (C) 2010-2015 Marvell International Ltd.
//     Copyright (C) 2002-2010 Kinoma, Inc.
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//
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
