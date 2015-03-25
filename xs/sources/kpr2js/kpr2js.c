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

#include "xsPlatform.h"
#include "xs.h"
#include "kpr2js.xs.h"

#if mxWindows
#define mxSeparator '\\'
#else
#define mxSeparator '/'
#endif

void kprGrammar_countLines(xsMachine* the)
{
	char* aString;
	char c;
	int i = 0;
	aString = xsToString(xsArg(0));
	while ((c = *aString)) {
		if (c <= 0x20) {
			if (c == 0x0A)
				i++;
		}
		else
			break;
		aString++;
	}
	xsResult = xsInteger(i);
}

void kpr2jsCompareFile(xsMachine* the)
{
	char *aPath1 = NULL;
	char *aPath2 = NULL;
	
	aPath1 = xsToString(xsArg(0));
	aPath2 = xsToString(xsArg(1));
#if mxWindows
	xsResult = xsInteger(_stricmp(aPath1, aPath2) == 0);
#elif mxMacOSX
	xsResult = xsInteger(strcasecmp(aPath1, aPath2) == 0);
#else
	xsResult = xsInteger(strcmp(aPath1, aPath2) == 0);	
#endif
}

void kpr2jsCompareName(xsMachine* the)
{
	char *aName1 = NULL;
	char *aName2 = NULL;
	
	aName1 = xsToString(xsArg(0));
	aName2 = xsToString(xsArg(1));
#if mxWindows
	xsResult = xsInteger(_stricmp(aName1, aName2));
#else
	xsResult = xsInteger(strcasecmp(aName1, aName2));
#endif
}

void kpr2jsCreateDirectory(xsMachine* the)
{
	char* aPath;

	aPath = xsToString(xsArg(0));
#if mxWindows
	_mkdir(aPath);
#else
	mkdir(aPath, 0744);
#endif
	xsResult = xsArg(0);
}

void kpr2jsGetPlatform(xsMachine* the)
{
	#if mxWindows
		xsResult = xsString("Windows");
	#elif mxMacOSX
		xsResult = xsString("MacOSX");
	#elif mxLinux
		xsResult = xsString("Linux");
	#elif mxSolaris
		xsResult = xsString("Solaris");
	#else
		#pragma error("need a platform")
	#endif
}

void kpr2jsLoad(xsMachine* the)
{
	char* aPath;
	FILE* aFile = NULL;

	xsTry {
		aPath = xsToString(xsArg(0));
		aFile = fopen(aPath, "r");
		xsElseError(aFile);
		xsResult = xsParse(aFile, (xsGetter)fgetc, aPath, 1, xsSourceFlag | xsDebugFlag);
		fclose(aFile);
	}
	xsCatch {
		if (aFile)
			fclose(aFile);
		xsThrow(xsException);
	}
}

void kpr2jsMakePath(xsMachine* the)
{
	char aPath[1024];
	int aLength;
	char* aString;

	aPath[0] = 0;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		aString = xsToString(xsArg(0));
		aLength = strlen(aString);
		if (aLength > 0) {
			strcat(aPath, aString);
			if (aPath[aLength - 1] != mxSeparator) {
				aPath[aLength] = mxSeparator;
				aPath[aLength + 1] = 0;
			}
		}
	}
	if (xsTypeOf(xsArg(1)) == xsStringType) {
		aString = xsToString(xsArg(1));
		strcat(aPath, aString);
	}
	if (xsTypeOf(xsArg(2)) == xsStringType) {
		aString = xsToString(xsArg(2));
		aLength = strlen(aString);
		if (aLength > 0) {
			if (aString[0] != '.')
				strcat(aPath, ".");
			strcat(aPath, aString);
		}
	}
#if mxWindows
	aString = aPath;
	while (*aString) {
		if (*aString == '/')
			*aString = '\\';
		aString++;
	}	
#endif	
	xsResult = xsString(aPath);
}

void kpr2jsReport(xsMachine* the)
{
	char *aString = NULL;
	
	aString = xsToString(xsArg(0));
	fprintf(stderr, "%s\n", aString);
}

void kpr2jsReportError(xsMachine* the)
{
	char* aPath;
	long aLine;
	int aCount;
	
	xsDebugger();
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		aPath = xsToString(xsArg(0));
		aLine = xsToInteger(xsArg(1));
	#if mxWindows
		fprintf(stderr, "%s(%ld): error: ", aPath, aLine);
	#else
		fprintf(stderr, "%s:%ld: error: ", aPath, aLine);
	#endif
	}
	fprintf(stderr, "%s!\n", xsToString(xsArg(2)));
	aCount = xsToInteger(xsGet(xsThis, xsID("errorCount")));
	xsSet(xsThis, xsID("errorCount"), xsInteger(aCount + 1));
}	

void kpr2jsReportWarning(xsMachine* the)
{
	char* aPath;
	long aLine;
	int aCount;
	
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		aPath = xsToString(xsArg(0));
		aLine = xsToInteger(xsArg(1));
	#if mxWindows
		fprintf(stderr, "%s(%ld): warning: ", aPath, aLine);
	#else
		fprintf(stderr, "%s:%ld: warning: ", aPath, aLine);
	#endif
	}
	fprintf(stderr, "%s!\n", xsToString(xsArg(2)));
	aCount = xsToInteger(xsGet(xsThis, xsID("warningCount")));
	xsSet(xsThis, xsID("warningCount"), xsInteger(aCount + 1));
}	
	
void kpr2jsResolvePath(xsMachine* the)
{
#if mxWindows
	char* srcPath;
	char aPath[1024];
	DWORD attributes;
	int aResult = 0;

	srcPath = xsToString(xsArg(0));
	if (_fullpath(aPath, srcPath, 1024) != NULL) {
		attributes = GetFileAttributes(aPath);
		if (attributes != 0xFFFFFFFF) {
			if (xsToInteger(xsArgc) == 1)
				aResult = 1;
			else if (xsToBoolean(xsArg(1)))
				aResult = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0; 
			else
				aResult = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1; 
		}
	}
#else
	char* srcPath;
	char aPath[PATH_MAX];
	struct stat a_stat;
	int aResult = 0;
	
	srcPath = xsToString(xsArg(0));
	if (realpath(srcPath, aPath) != NULL) {
		if (stat(aPath, &a_stat) == 0) {
			if (xsToInteger(xsArgc) == 1)
				aResult = 1;
			else if (xsToBoolean(xsArg(1)))
				aResult = S_ISDIR(a_stat.st_mode) ? 1 : 0;
			else
				aResult = S_ISREG(a_stat.st_mode) ? 1 : 0;
		}
	}
#endif
	if (aResult) {
		if (xsToBoolean(xsArg(1))) {
			aResult = strlen(aPath) - 1;
			if (aPath[aResult] == mxSeparator)
				aPath[aResult] = 0;
		}
		xsResult = xsString(aPath);
	}
}

void kpr2jsSplitPath(xsMachine* the)
{
	char *aPath;
	char *aSeparator = NULL;
	char *aDot = NULL;
	int aLength;
	char aDirectory[1024];
	char aName[1024];
	char anExtension[1024];
	
	aPath = xsToString(xsArg(0));
	aSeparator = strrchr(aPath, mxSeparator);
	if (aSeparator == NULL)
		aSeparator = aPath;
	else
		aSeparator++;
	aDot = strrchr(aSeparator, '.');
	if (aDot == NULL)
		aDot = aSeparator + strlen(aSeparator);
	aLength = aSeparator - aPath;
	strncpy(aDirectory, aPath, aLength);
	aDirectory[aLength - 1] = 0;
	aLength = aDot - aSeparator;
	strncpy(aName, aSeparator, aLength);
	aName[aLength] = 0;
	strcpy(anExtension, aDot);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	xsSet(xsResult, 0, xsString(aDirectory));
	xsSet(xsResult, 1, xsString(aName));
	xsSet(xsResult, 2, xsString(anExtension));
}

void kpr2jsCloseFile(xsMachine* the)
{
	FILE* aFile = xsGetHostData(xsThis);
	fclose(aFile);
	xsSetHostData(xsThis, NULL);
}

void kpr2jsDestroyFile(void* theData)
{
	if (theData)
		fclose(theData);
}

void kpr2jsIncludeFile(xsMachine* the)
{
	char aBuffer[1024];
	FILE* putFile = xsGetHostData(xsThis);
	FILE* getFile = fopen(xsToString(xsArg(0)), "r");
	xsElseError(getFile);
	while (fgets(aBuffer, sizeof(aBuffer), getFile))
		fputs(aBuffer, putFile);
	fclose(getFile);	
}

void kpr2jsOpenFile(xsMachine* the)
{
	FILE* aFile = fopen(xsToString(xsArg(0)), "w");
	xsElseError(aFile);
	xsSetHostData(xsThis, aFile);
}

void kpr2jsWriteFile(xsMachine* the)
{
	FILE* aFile = xsGetHostData(xsThis);
	int c = fprintf(aFile, "%s", xsToString(xsArg(0)));
	c += xsToInteger(xsGet(xsThis, xsID("charCount")));
	xsSet(xsThis, xsID("charCount"), xsInteger(c));
}

int main(int argc, char* argv[]) 
{
	xsAllocation anAllocation = {
		2048 * 1024, /* initialChunkSize */
		512 * 1024, /* incrementalChunkSize */
		1000 * 1000, /* initialHeapCount */	
		50 * 1000, /* incrementalHeapCount */
		64 * 1024, /* stackCount */
		4096, /* symbolCount */
		1993 /* symbolModulo */
	};
	xsMachine* aMachine;
	int argi;
	int result = 1;

	aMachine = xsNewMachine(&anAllocation, &kpr2jsGrammar, NULL);
	if (aMachine) {
		xsBeginHost(aMachine);
		{
			xsTry {
				xsResult = xsNewInstanceOf(xsArrayPrototype);
				for (argi = 0; argi < argc; argi++)
					xsSet(xsResult, argi, xsString(argv[argi]));
				(void)xsCall1(xsGlobal, xsID("main"), xsResult);
				result = 0;
			}
			xsCatch {
				char* aMessage = xsToString(xsGet(xsException, xsID("message")));
				if (*aMessage)
					fprintf(stderr, "### %s\n", aMessage);
				else
					fprintf(stderr, "### XML error(s)\n");
			}
		}
		xsEndHost(aMachine);
		xsDeleteMachine(aMachine);
	}
	else
		fprintf(stderr, "### Cannot allocate machine!\n");

	return result;
}

