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
#include "cryptTypes.h"
#include <stdlib.h>

void
xs_srand_constructor(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 1)
		srandom(xsToInteger(xsArg(0)));
}

void
xs_srand_get(xsMachine *the)
{
	int nbytes = 1;
	unsigned char *p;
	int i;

	if (xsToInteger(xsArgc) > 0)
		nbytes = xsToInteger(xsArg(0));
	xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(nbytes));
	p = (unsigned char *)xsGetHostData(xsResult);
	for (i = 0; i < nbytes; i++)
		*p++ = (unsigned char)random();
}
