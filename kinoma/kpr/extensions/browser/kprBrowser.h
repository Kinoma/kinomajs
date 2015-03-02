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
#ifndef __KPRBROWSER__
#define __KPRBROWSER__

#include "kpr.h"
#include "FskBrowser.h"

struct KprBrowserStruct {
	KprSlotPart;
	KprContentPart;
	
	Boolean _layered;
	Boolean _attached;
	FskBrowser _browser;
};

FskAPI(FskErr) KprBrowserNew(KprBrowser *self, KprCoordinates coordinates);
FskAPI(char *) KprBrowserGetURL(KprBrowser self);
FskAPI(void) KprBrowserSetURL(KprBrowser self, char* url);
FskAPI(void) KprBrowserReload(KprBrowser self);
FskAPI(void) KprBrowserBack(KprBrowser self);
FskAPI(void) KprBrowserForward(KprBrowser self);
FskAPI(char *) KprBrowserEvaluateScript(KprBrowser self, char *script);

#endif
