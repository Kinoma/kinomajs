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
#include "xsPlatform.h"
#include "xs.h"

#include "kprconfig.xs.h"

#if mxMacOSX || mxLinux
	#include <dirent.h>
	#include <netdb.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/unistd.h>
	#include <sys/socket.h>
	#include <string.h>
	#include <unistd.h>
#endif
#if mxWindows
	#include <direct.h>
	#include <process.h>
#endif

#if mxWindows
#define mxSeparator '\\'
#else
#define mxSeparator '/'
#endif

#define mxPropertyModulo 1993
xsSlot* gxProperties = NULL;

void xsToolCompareFile(xsMachine* the)
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

void xsToolCompareName(xsMachine* the)
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

void xsToolCreateDirectory(xsMachine* the)
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

void xsToolExecute(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	char** parameters= NULL;
	parameters = malloc(sizeof(char *)*(c + 1));
	for (i = 0; i < c; i++)
		parameters[i] = xsToString(xsArg(i));
	parameters[c] = NULL;
	fflush(NULL);
#if mxWindows
	if (_spawnvp(P_WAIT, xsToString(xsArg(0)), parameters) < 0)
		fprintf(stderr, "### Cannot execute nmake!\n");
#else
	execvp(xsToString(xsArg(0)), parameters);
#endif
}

void xsToolGetEnvironmentValue(xsMachine* the)
{
	char* aName = xsToString(xsArg(0));
	char* aValue = getenv(aName);
	if (aValue)
		xsResult = xsString(aValue);
}

void xsToolGetIPAddress(xsMachine* the)
{
	char hostname[128];
	struct hostent *he;
	gethostname(hostname, sizeof(hostname));
	he = gethostbyname(hostname);
	if (he)
		xsResult = xsString(inet_ntoa(*(struct in_addr*)he->h_addr));
}

void xsToolGetPlatform(xsMachine* the)
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
	
void xsToolInsertProperty(xsMachine* the)
{
	xsStringValue aString;
	xsIntegerValue aSum;

	aString = xsToString(xsArg(0));
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	xsResult = gxProperties[aSum % mxPropertyModulo];
	if (xsTypeOf(xsResult) == xsReferenceType)
		(void)xsCall3(xsResult, xsID("insertProperty"), xsArg(0), xsArg(1), xsArg(2));
	else {
		gxProperties[aSum % mxPropertyModulo] = xsArg(1);
		xsSet(xsArg(1), xsID("qualifiedName"), xsArg(0));
	}	
}

void xsToolIsKPR(xsMachine* the)
{
	char* aPath;
	FILE* aFile = NULL;
	size_t aSize = 1024;
	char aBuffer[1024];

	aPath = xsToString(xsArg(0));
	aFile = fopen(aPath, "r");
	xsElseError(aFile);
	aSize = fread(aBuffer, 1, aSize, aFile);
	if (aSize) {
		aBuffer[aSize - 1] = 0;
		xsResult = (strstr(aBuffer, "xmlns=\"http://www.kinoma.com/kpr/1\"") && !strstr(aBuffer, "<application") && !strstr(aBuffer, "<device")) ? xsTrue : xsFalse;
	}
	else
		xsResult = xsFalse;

	fclose(aFile);
}
	
void xsToolJoinPath(xsMachine* the)
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

void xsToolListDirectory(xsMachine* the)
{
#if mxWindows
	xsStringValue path, name = NULL;
	UINT32 length, index;
	UINT16 *pathW = NULL;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW findData;

	xsTry {
		xsVars(1);
		xsResult = xsNewInstanceOf(xsArrayPrototype);
	
		path = xsToString(xsArg(0));
		length = strlen(path);
		pathW = malloc((length + 3) * 2);
		xsElseError(pathW);
		MultiByteToWideChar(CP_UTF8, 0, path, length + 1, pathW, length + 1);
		for (index = 0; index < length; index++) {
			if (pathW[index] == '/')
				pathW[index] = '\\';
		}
		pathW[length] = '\\';
		pathW[length + 1] = '*';
		pathW[length + 2] = 0;
		findHandle = FindFirstFileW(pathW, &findData);
		if (findHandle != INVALID_HANDLE_VALUE) {
			do {
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
					!wcscmp(findData.cFileName, L".") ||
					!wcscmp(findData.cFileName, L".."))
					continue;
				length = wcslen(findData.cFileName);
				name = malloc((length + 1) * 2);
				xsElseError(name);
				WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, length + 1, name, length + 1, NULL, NULL);
				xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsVar(0), xsID("name"), xsString(name));
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					xsSet(xsVar(0), xsID("directory"), xsTrue);
				else
					xsSet(xsVar(0), xsID("directory"), xsFalse);
				xsCall1(xsResult, xsID("push"), xsVar(0));
				free(name);
				name = NULL;
			} while (FindNextFileW(findHandle, &findData));
		}
	}
	xsCatch {
	}
	if (name)
		free(name);
	if (findHandle != INVALID_HANDLE_VALUE)
		FindClose(findHandle);
	if (pathW)
		free(pathW);
#else
    DIR* dir;
	char path[1024];
	int length;

	xsVars(1);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	dir = opendir(xsToStringBuffer(xsArg(0), path, sizeof(path) - 1));
	length = strlen(path);
	path[length] = '/';
	length++;
	if (dir) {
		struct dirent *ent;
		while ((ent = readdir(dir))) {
			struct stat a_stat;
			if (ent->d_name[0] == '.')
				continue;
			strcpy(path + length, ent->d_name);
			if (!stat(path, &a_stat)) {
				xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsVar(0), xsID("name"), xsString(ent->d_name));
				if (S_ISDIR(a_stat.st_mode))
					xsSet(xsVar(0), xsID("directory"), xsTrue);
				else
					xsSet(xsVar(0), xsID("directory"), xsFalse);
				(void)xsCall1(xsResult, xsID("push"), xsVar(0));
			}
		}
		closedir(dir);
	}
#endif
}

void xsToolLoad(xsMachine* the)
{
	char* aPath;
	FILE* aFile = NULL;
	xsFlag flags;

	aPath = xsToString(xsArg(0));
	aFile = fopen(aPath, "r");
	xsElseError(aFile);
	flags = xsNoMixtureFlag | xsDebugFlag;
	if (xsToInteger(xsArgc) > 1)
		flags = xsToInteger(xsArg(1));
	xsResult = xsParse(aFile, (xsGetter)fgetc, aPath, 1, flags);
	fclose(aFile);
}


void xsToolReadString(xsMachine* the)
{
	char* aPath;
	FILE* aFile;
	size_t aSize;
	char *aBuffer;

	aPath = xsToString(xsArg(0));
#if mxWindows
	{
		char* aSlash;
		aSlash = aPath;
		while (*aSlash) {
			if (*aSlash == '/')
				*aSlash = '\\';
			aSlash++;
		}
	}
#endif
	aFile = fopen(aPath, "rb");
	if (aFile) {
		fseek(aFile, 0, SEEK_END);
		aSize = ftell(aFile);
		fseek(aFile, 0, SEEK_SET);
		aBuffer = malloc(aSize + 1);
		if (aBuffer) {
			aSize = fread(aBuffer, 1, aSize, aFile);
			aBuffer[aSize] = 0;
			xsResult = xsString(aBuffer);
			free(aBuffer);
		}
		fclose(aFile);
	}
}

void xsToolReplacePath(xsMachine* the)
{
	char* aName = xsToString(xsArg(1));
	char* aValue = getenv(aName);
	if (aValue)
		xsResult = xsString(aValue);
	else
		xsResult = xsArg(1);
}

void xsToolReport(xsMachine* the)
{
	char *aString = NULL;
	
	aString = xsToString(xsArg(0));
	fprintf(stderr, "%s\n", aString);
}

void xsToolReportError(xsMachine* the)
{
	char* aPath;
	long aLine;
	int aCount;
	
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

void xsToolReportWarning(xsMachine* the)
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
	
void xsToolResolvePath(xsMachine* the)
{
#if mxWindows
	char* srcPath;
	char* aSlash;
	char aPath[1024];
	DWORD attributes;
	int aResult = 0;

	srcPath = xsToString(xsArg(0));
	aSlash = srcPath;
	while (*aSlash) {
		if (*aSlash == '/')
			*aSlash = '\\';
		aSlash++;
	}	
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

void xsToolSplitPath(xsMachine* the)
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
	if (aLength)
		aDirectory[aLength - 1] = 0;
	else
		aDirectory[0] = 0;
	aLength = aDot - aSeparator;
	strncpy(aName, aSeparator, aLength);
	aName[aLength] = 0;
	strcpy(anExtension, aDot);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	xsSet(xsResult, 0, xsString(aDirectory));
	xsSet(xsResult, 1, xsString(aName));
	xsSet(xsResult, 2, xsString(anExtension));
}

void xsToolSearchProperty(xsMachine* the)
{
	xsStringValue aString;
	xsIntegerValue aSum;

	aString = xsToString(xsArg(0));
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	xsResult = gxProperties[aSum % mxPropertyModulo];
	if (xsTypeOf(xsResult) == xsReferenceType)
		xsResult = xsCall1(xsResult, xsID("searchProperty"), xsArg(0));
}

void xsToolWriteString(xsMachine* the)
{
	char* aPath;
	FILE* aFile;
	char *aBuffer;
	size_t aSize;

	aPath = xsToString(xsArg(0));
#if mxWindows
	{
		char* aSlash;
		aSlash = aPath;
		while (*aSlash) {
			if (*aSlash == '/')
				*aSlash = '\\';
			aSlash++;
		}
	}
#endif
	aFile = fopen(aPath, "wb");
	if (aFile) {
		aBuffer = xsToString(xsArg(1));
		aSize = strlen(aBuffer);
		fwrite(aBuffer, 1, aSize, aFile);
		fclose(aFile);
	}
}

void xsToolCloseFile(xsMachine* the)
{
	FILE* aFile = xsGetHostData(xsThis);
	fclose(aFile);
	xsSetHostData(xsThis, NULL);
}

void xsToolDestroyFile(void* theData)
{
	if (theData)
		fclose(theData);
}

void xsToolIncludeFile(xsMachine* the)
{
	char aBuffer[1024];
	FILE* putFile = xsGetHostData(xsThis);
	FILE* getFile = fopen(xsToString(xsArg(0)), "r");
	xsElseError(getFile);
	while (fgets(aBuffer, sizeof(aBuffer), getFile))
		fputs(aBuffer, putFile);
	fclose(getFile);	
}

void xsToolOpenFile(xsMachine* the)
{
	FILE* aFile;
	int argc = xsToInteger(xsArgc);
	char *path = xsToString(xsArg(0));
	if (argc > 1)
		aFile = fopen(path, xsToString(xsArg(1)));
	else
		aFile = fopen(path, "w");
	xsElseError(aFile);
	xsSetHostData(xsThis, aFile);
}

void xsToolWriteFile(xsMachine* the)
{
	FILE* aFile = xsGetHostData(xsThis);
	fprintf(aFile, "%s", xsToString(xsArg(0)));
}


void xsToolFileFixUpPath(xsMachine *the)
{
#if mxWindows
	char	*path;
	UINT32	index;

	if (xsToInteger(xsArgc) > 0) {
		path = xsToString(xsArg(0));

		for (index = 0; path[index] != 0; index++) {
			if (path[index] == '/')
				path[index] = '\\';
		}
	}
#endif
}

static const char *gKPRUsage = "kprconfig [manifest_file] [options]\n\
  <manifest_file>\n\
\tThe manifest file. The default is manifest.xml in the current directory.\n\
  -p <platform>\n\
\tandroid, iphone, linux, mac or win. The platform is case insensitive and supports several synonyms. The default is the platform running kprconfig.\n\
  -c <configuration>\n\
\tdebug or release. The configuration is case insensitive. The default is release.\n\
  -a <application>\n\
\tThe application. The default is the name of the directory that contains the manifest.\n\
  -o <directory>\n\
\tThe output directory. The directory where to create binary and temporary files is based on the output directory, the platform, the configuration and the application. The default is F_HOME.\n\
  -b <directory>\n\
\tThe directory where to create binary files. Overrides the -o option for binary files.\n\
  -t <directory>\n\
\tThe directory where to create temporary files. Overrides the -o option for temporary files.\n\
  -m [target]\n\
\tkprconfig executes make or nmake. The target can be clean or all, if omitted it is all by default.\n\
  -v\n\
\tVerbose.\n\
  -i\n\
\tInstrument. Adding/removing that option recompiles all C files. \n\
  -l\n\
\tLog memory usage. Adding/removing that option recompiles all C files.\n\
  -x\n\
\tCreate a CMake project. \n\
  -X\n\
\tEnable SUPPORT_XS_DEBUG. \n\
  -d\n\
\tDebug mode. Same as \'-c debug\'\n\
  -j <jobs> \n\
\tBuild application with <jobs> job simultaneously. \n\
  --help\n\
\tThis help information.\n\
\n";
	

void printHelp(void){
	printf("%s", gKPRUsage);
}

int main(int argc, char* argv[]) 
{
	xsAllocation anAllocation = {
		2048 * 1024, /* initialChunkSize */
		512 * 1024, /* incrementalChunkSize */
		1000 * 1000, /* initialHeapCount */	
		50 * 1000, /* incrementalHeapCount */
		1024, /* stackCount */
		8192, /* symbolCount */
		1993 /* symbolModulo */
	};
	xsMachine* aMachine;
	int argi;
	int result = 1;

	for (argi = 0; argi < argc; argi++){
		result = strncmp(argv[argi], "--help", strlen("--help"));
		if(!result) {
			printHelp();
			exit(0);
		}
	}
	
	result = 1;

	gxProperties = calloc(mxPropertyModulo, sizeof(xsSlot));
	if (gxProperties) {
		aMachine = xsNewMachine(&anAllocation, &kprconfigGrammar, NULL);
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
					fprintf(stderr, "### %s\n", aMessage);
				}
			}
			xsEndHost(aMachine);
			xsDeleteMachine(aMachine);
		}
		else
			fprintf(stderr, "### Cannot allocate machine!\n");
	}
	else {
		fprintf(stderr, "### Cannot allocate properties!\n");
	}

	return result;
}

