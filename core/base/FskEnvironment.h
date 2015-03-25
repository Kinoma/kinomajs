/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#ifndef __FSKENVIRONMENT__
#define __FSKENVIRONMENT__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif

FskErr FskEnvironmentInitialize(void);
void FskEnvironmentTerminate(void);

FskAPI(char *)FskEnvironmentGet(char *name);
FskAPI(void) FskEnvironmentSet(const char *name, const char *value);

FskAPI(char *)FskEnvironmentApply(char *input);		// returns NULL if no changes
FskAPI(char *)FskEnvironmentDoApply(char *input);	// returns input if no changes. if changes, dispose input and return new string.

#ifdef __cplusplus
}
#endif

#endif
