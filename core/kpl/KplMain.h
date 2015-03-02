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
#ifndef __KPL_MAIN_H__
#define __KPL_MAIN_H__

#include "Kpl.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	kKplMainNetwork = 		1L << 0,
	kKplMainServer = 		1L << 1,
	kKplMainNoECMAScript = 	1L << 2
};

typedef int (*KplMainProc)(void *refcon);

int KplMain(UInt32 flags, int argc, char **argv, KplMainProc kplMainProc, void *kplMainProcRefcon);

#ifdef __cplusplus
}
#endif

#endif
