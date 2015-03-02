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
#ifndef FSKCONFIG_H
#define FSKCONFIG_H

#include "Fsk.h"

/* #include <memory.h> */	/* deprecated by string.h */
#include <string.h>

#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024

#if TARGET_RT_LITTLE_ENDIAN
	#define BYTEORDER 1234
#else
	#define BYTEORDER 4321
#endif

#define HAVE_MEMMOVE

#define XMLCALL
#if TARGET_OS_MAC
	#define XMLIMPORT __attribute__ ((visibility("default")))
#else
	#define XMLIMPORT
#endif
#define XML_STATIC

#endif
