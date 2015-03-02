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
#include "FskExtensions.h"
#include "FskECMAScript.h"
#include "FskEnvironment.h"
#include "FskUtilities.h"
#if TARGET_OS_ANDROID
#include "FskThread.h"
#include "FskHardware.h"
#endif

#if !FSK_EXTENSION_EMBED
#include "FskSSL.xs.h"

FskExport(xsGrammar *) fskGrammar = &FskSSLGrammar;
#endif /* !FSK_EXTENSION_EMBED */

FskExport(FskErr) FskSSL_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}

FskExport(FskErr) FskSSL_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

/* why this has to be a xs function, not just called from fskLoad()? Because fskLoad can be called after <program> */
void xs_FskSSL_getCertPath(xsMachine *the)
{
	char *capath;

#if TARGET_OS_ANDROID
	extern void unpackAndroid();

	unpackAndroid();
	capath = gAndroidCallbacks->getStaticDataDirCB();
#else
	capath = FskEnvironmentGet("applicationPath");
#endif
	xsResult = xsString(capath);
}
