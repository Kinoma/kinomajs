/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef _KINOMA_IPP_ENV_
#define _KINOMA_IPP_ENV_

#ifdef __FSK_LAYER__
	#include "Fsk.h"
	#if TARGET_RT_BIG_ENDIAN
		#ifndef TARGET_OS_MACOSX_X86
			#define _BIG_ENDIAN_ 1
		#endif
	#endif

	#include "FskMemory.h"
	#define malloc(sz)	(void *)FskMemPtrAlloc(sz)
	#define free(p)	FskMemPtrDispose(p)
#endif

#endif	//_KINOMA_IPP_ENV_
