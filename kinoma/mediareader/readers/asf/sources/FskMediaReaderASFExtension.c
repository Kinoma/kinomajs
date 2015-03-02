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
#define __FSKMEDIAREADER_PRIV__
#include "FskECMAScript.h"
#include "FskMediaReader.h"

#if !FSK_EXTENSION_EMBED
	#include "FskMediaReaderASF.xs.h"
	FskExport(xsGrammar *) fskGrammar = &FskMediaReaderASFGrammar;
#endif

extern FskMediaReaderDispatch gMediaReaderASFFile;
extern FskMediaReaderDispatch gMediaReaderASFMMSOverHTTP;

FskExport(FskErr) FskMediaReaderASF_fskLoad(FskLibrary library)
{
	FskMediaReaderInstall(&gMediaReaderASFFile);
	FskMediaReaderInstall(&gMediaReaderASFMMSOverHTTP);
	return kFskErrNone;
}

FskExport(FskErr) FskMediaReaderASF_fskUnload(FskLibrary library)
{
	FskMediaReaderUninstall(&gMediaReaderASFFile);
	FskMediaReaderUninstall(&gMediaReaderASFMMSOverHTTP);
	return kFskErrNone;
}
