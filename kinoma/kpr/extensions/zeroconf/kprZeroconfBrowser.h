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
#ifndef __KPRZEROCONFBROWSER__
#define __KPRZEROCONFBROWSER__

typedef struct KprZeroconfBrowserStruct KprZeroconfBrowserRecord, *KprZeroconfBrowser;

typedef void (*KprZeroconfBrowserServiceUpCallback)(KprZeroconfBrowser self, KprZeroconfServiceInfo serviceInfo);
typedef void (*KprZeroconfBrowserServiceDownCallback)(KprZeroconfBrowser self, KprZeroconfServiceInfo serviceInfo);

struct KprZeroconfBrowserStruct {
	KprZeroconfCommonPart;
	char* authority;
	char* domain;
	KprZeroconfBrowserServiceUpCallback serviceUpCallback;
	KprZeroconfBrowserServiceDownCallback serviceDownCallback;
	FskInstrumentedItemDeclaration
};

FskErr KprZeroconfBrowserNew(KprZeroconfBrowser *it, const char* serviceType);
void KprZeroconfBrowserDispose(KprZeroconfBrowser self);
KprZeroconfBrowser KprZeroconfBrowserFind(KprZeroconfBrowser self, const char* serviceType);
void KprZeroconfBrowserServiceUp(KprZeroconfBrowser self, KprZeroconfServiceInfo serviceInfo);
void KprZeroconfBrowserServiceDown(KprZeroconfBrowser self, KprZeroconfServiceInfo serviceInfo);
FskErr KprZeroconfBrowserStart(KprZeroconfBrowser self);
FskErr KprZeroconfBrowserStop(KprZeroconfBrowser self);

FskErr KprZeroconfPlatformBrowserNew(KprZeroconfBrowser self);
void KprZeroconfPlatformBrowserDispose(KprZeroconfBrowser self);
FskErr KprZeroconfPlatformBrowserStart(KprZeroconfBrowser self);
FskErr KprZeroconfPlatformBrowserStop(KprZeroconfBrowser self);

#endif
