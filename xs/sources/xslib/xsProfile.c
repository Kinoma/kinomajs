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
#include "xsAll.h"

#ifdef mxProfile

static void fxRecordProfiling(txMachine* the, txInteger theFlag, txInteger theID);
static int fxSubtractTV(c_timeval *result, c_timeval *x, c_timeval *y);
static void fxWriteProfileBOM(txMachine* the);
static void fxWriteProfileGrammar(txMachine* the, txSlot* theProperty, txSlot* theList);
static void fxWriteProfileGrammarArray(txMachine* the, txSlot* theProperty, txSlot* theList);
static void fxWriteProfileProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txInteger theIndex);
static void fxWriteProfileRecords(txMachine* the);
static void fxWriteProfileSymbols(txMachine* the);

enum {
	XS_PROFILE_BEGIN = 0x40000000,
	XS_PROFILE_END = 0x80000000,
	XS_PROFILE_SECOND = 0xC0000000
};

void fxBeginFunction(txMachine* the)
{
	txSlot* aSlot;
	
	if (!the->profileFile)
		return;
	aSlot = the->frame + 2;
	if (!mxIsReference(aSlot))
		return;
	aSlot = fxGetInstance(the, aSlot);
	if (!mxIsFunction(aSlot))
		return;
	aSlot = aSlot->next->next->next->next;
	if (aSlot->kind != XS_INTEGER_KIND)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_BEGIN, aSlot->value.integer);
}

void fxBeginGC(txMachine* the)
{
	if (!the->profileFile)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_BEGIN, 0);
}

void fxDoCallback(txMachine* the, txCallback address)
{
	mxTry(the) {
		if (the->profileFile)
			fxBeginFunction(the);
		(*address)(the);
		if (the->profileFile)
			fxEndFunction(the);
	}
	mxCatch(the) {
		if (the->profileFile)
			fxEndFunction(the);
		fxJump(the);
	}
}

void fxEndFunction(txMachine* the)
{
	txSlot* aSlot;
	
	if (!the->profileFile)
		return;
	aSlot = the->frame + 2;
	if (!mxIsReference(aSlot))
		return;
	aSlot = fxGetInstance(the, aSlot);
	if (!mxIsFunction(aSlot))
		return;
	aSlot = aSlot->next->next->next->next;
	if (aSlot->kind != XS_INTEGER_KIND)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_END, aSlot->value.integer);
	if (the->frame->next == NULL)
		fxWriteProfileRecords(the);
}

void fxEndGC(txMachine* the)
{
	if (!the->profileFile)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_END, 0);
}

void fxRecordProfiling(txMachine* the, txInteger theFlag, txInteger theID)
{
	txProfileRecord* aRecord = the->profileCurrent;
	c_timeval deltaTV;
#if mxWindows
	LARGE_INTEGER counter;
	LARGE_INTEGER delta;
	LARGE_INTEGER million;

	QueryPerformanceCounter(&counter);
	delta.QuadPart = counter.QuadPart - the->profileCounter.QuadPart;
	if (delta.QuadPart <= 0)
		delta.QuadPart = 1;
	million.HighPart = 0;
	million.LowPart = 1000000;
	delta.QuadPart = (million.QuadPart * delta.QuadPart) / the->profileFrequency.QuadPart;
	deltaTV.tv_sec = (long)(delta.QuadPart / million.QuadPart);
	deltaTV.tv_usec = (long)(delta.QuadPart % million.QuadPart);
	the->profileCounter = counter;
#else
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	fxSubtractTV(&deltaTV, &tv, &(the->profileTV));
	the->profileTV = tv;
#endif
	if (deltaTV.tv_sec) {
		aRecord->profileID = deltaTV.tv_sec;
		aRecord->delta = XS_PROFILE_SECOND;
		aRecord++;
		if (aRecord == the->profileTop) {
			the->profileCurrent = aRecord;
			fxWriteProfileRecords(the);
			aRecord = the->profileCurrent;
		}
	}
	aRecord->profileID = theID;
	aRecord->delta = theFlag | deltaTV.tv_usec;
	aRecord++;
	if (aRecord == the->profileTop) {
		the->profileCurrent = aRecord;
		fxWriteProfileRecords(the);
		aRecord = the->profileCurrent;
	}
	the->profileCurrent = aRecord;
}

int fxSubtractTV(c_timeval *result, c_timeval *x, c_timeval *y)
{
  /* Perform the carry for the later subtraction by updating Y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }
     
  /* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
     
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void fxWriteProfileBOM(txMachine* the)
{
	txID BOM = 0xFF00;
	
	fxWriteProfileFile(the, &BOM, sizeof(BOM));
}

void fxWriteProfileGrammar(txMachine* the, txSlot* theProperty, txSlot* theList)
{
	txSlot* anInstance;
	
	while (theProperty) {
		if ((theProperty->kind == XS_REFERENCE_KIND) || (theProperty->kind == XS_ALIAS_KIND)) {
			anInstance = fxGetInstance(the, theProperty);
			if (((anInstance->flag & XS_VALUE_FLAG) == 0) || (anInstance->next->kind != XS_ARRAY_KIND))
				fxWriteProfileProperty(the, theProperty, theList, -1);
		}
		theProperty = theProperty->next;
	}
}

void fxWriteProfileGrammarArray(txMachine* the, txSlot* theProperty, txSlot* theList)
{
	txSlot* anInstance;
	
	while (theProperty) {
		if ((theProperty->kind == XS_REFERENCE_KIND) || (theProperty->kind == XS_ALIAS_KIND)) {
			anInstance = fxGetInstance(the, theProperty);
			if ((anInstance->flag & XS_VALUE_FLAG) && (anInstance->next->kind == XS_ARRAY_KIND))
				fxWriteProfileProperty(the, theProperty, theList, -1);
		}
		theProperty = theProperty->next;
	}
}

#define mxNameSize 256

int TABS = 0;
int TAB;
void fxWriteProfileProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txInteger theIndex)
{
	txSlot* anInstance;
	txSlot* aSlot;
	char aName[mxNameSize];
	txSlot aSymbol;
	txSlot* aProperty;
	txInteger aCount;
	txInteger anIndex;
	
	if ((theProperty->kind != XS_REFERENCE_KIND) && (theProperty->kind != XS_ALIAS_KIND))
		return;
	anInstance = fxGetInstance(the, theProperty);
	if (anInstance->flag & XS_MARK_FLAG)
		return;
	anInstance->flag |= XS_MARK_FLAG;
	
	if (theProperty->flag & XS_SANDBOX_FLAG)
		c_strcpy(aName, ".sandbox");
	else
		aName[0] = 0;
	if (theIndex >= 0) {
		c_strcat(aName, "[");
		fxIntegerToString(theIndex, aName + 1, mxNameSize - 3);
		c_strcat(aName, "]");
	}	
	else {
		aSlot = fxGetSymbol(the, theProperty->ID);
		if (aSlot) {
			c_strcat(aName, ".");
			c_strncat(aName, aSlot->value.symbol.string, mxNameSize - c_strlen(aName) - 1);
		}
		else
			c_strcat(aName, "[]");
	}
	aName[mxNameSize - 1] = 0;
	theList->value.list.last->value.symbol.string = aName;
		
	aSymbol.next = C_NULL;	
	aSymbol.value.symbol.string = C_NULL;	
	aSymbol.value.symbol.sum = 0;	

	anInstance->value.instance.garbage = theList->value.list.last;
	theList->value.list.last->next = &aSymbol;
	theList->value.list.last = &aSymbol;

	aProperty = anInstance->next;
	if (anInstance->flag & XS_VALUE_FLAG) {
		switch (aProperty->kind) {
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
			aSlot = aProperty->next->next;
			if ((aSlot->value.alias != mxObjectPrototype.value.alias) 
					/*|| (anInstance == fxGetInstance(the, fxGetProperty(the, mxGlobal.value.reference, fxID(the, "Object"))))*/) {
				aSlot->ID = the->prototypeID;
				fxWriteProfileProperty(the, aSlot, theList, -1);
				aSlot->ID = XS_NO_ID;
			}
			aSlot = aProperty->next->next->next;
			if (aSlot->kind == XS_INTEGER_KIND) {
				fxWriteProfileFile(the, &(aSlot->value.integer), sizeof(txInteger));
				aSlot = theList->value.list.first;
				while (aSlot) {
					if (aSlot->value.symbol.string) {
						fxWriteProfileFile(the, aSlot->value.symbol.string, c_strlen(aSlot->value.symbol.string));
					}
					aSlot = aSlot->next;
				}
				fxWriteProfileFile(the, &(aName[mxNameSize - 1]), sizeof(char));
			}
			aProperty = aProperty->next->next->next;
			break;
		case XS_ARRAY_KIND:
			aSlot = aProperty->value.array.address;
			aCount = aProperty->value.array.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				if (aSlot->ID)
					fxWriteProfileProperty(the, aSlot, theList, anIndex);
				aSlot++;
			}
			aProperty = aProperty->next;
			break;
		default:
			aProperty = aProperty->next;
			break;
		}
	}
	
	fxWriteProfileGrammar(the, aProperty, theList);
	fxWriteProfileGrammarArray(the, aProperty, theList);
	
	theList->value.list.last = anInstance->value.instance.garbage;
	theList->value.list.last->next = C_NULL;
	theList->value.list.last->value.symbol.string = C_NULL;
	anInstance->value.instance.garbage = C_NULL;
}

void fxWriteProfileRecords(txMachine* the)
{
	fxWriteProfileFile(the, the->profileBottom, (the->profileCurrent - the->profileBottom) * sizeof(txProfileRecord));
	the->profileCurrent = the->profileBottom;
}

void fxWriteProfileSymbols(txMachine* the)
{
	char aName[255];
	txInteger aProfileID;
	txSlot aSymbol;
	txSlot aList;
	txSlot* anInstance;
	txSlot* aProperty;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	
	fxWriteProfileFile(the, &(the->profileID), sizeof(txInteger));
	c_strcpy(aName, "(gc)");
	aProfileID = 0;
	fxWriteProfileFile(the, &aProfileID, sizeof(txInteger));
	fxWriteProfileFile(the, aName, c_strlen(aName) + 1);

	aSymbol.next = C_NULL;	
	aSymbol.value.symbol.string = C_NULL;	
	aSymbol.value.symbol.sum = 0;
		
	aList.value.list.first = &aSymbol; 
	aList.value.list.last = &aSymbol; 

	anInstance = mxGlobal.value.reference;
	anInstance->flag |= XS_MARK_FLAG; 
	
	aProperty = anInstance->next->next;
	fxWriteProfileGrammar(the, aProperty, &aList);
	fxWriteProfileGrammarArray(the, aProperty, &aList);

	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			bSlot->flag &= ~XS_MARK_FLAG; 
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	if (the->sharedMachine) {
		aSlot = the->sharedMachine->firstHeap;
		while (aSlot) {
			bSlot = aSlot + 1;
			cSlot = aSlot->value.reference;
			while (bSlot < cSlot) {
				bSlot->flag &= ~XS_MARK_FLAG; 
				bSlot++;
			}
			aSlot = aSlot->next;
		}
	}
}

#endif

txS1 fxIsProfiling(txMachine* the)
{
#ifdef mxProfile
	return (the->profileFile) ? 1 : 0;
#else
	return 0;
#endif
}

void fxStartProfiling(txMachine* the)
{
#ifdef mxProfile
	txSlot* aFrame;
	txInteger frameCount;
	txSlot** frames;

	if (the->profileFile)
		return;
	fxOpenProfileFile(the, "xsprofile.records.out");
	fxWriteProfileBOM(the);
#if mxWindows
	QueryPerformanceFrequency(&the->profileFrequency);
	QueryPerformanceCounter(&the->profileCounter);
#else
	c_gettimeofday(&(the->profileTV), NULL);
#endif	
	frameCount = 0;
	aFrame = the->frame;
	while (aFrame) {
		frameCount++;
		aFrame = aFrame->next;
	}
	frames = c_malloc(frameCount * sizeof(txSlot*));
	if (!frames)
		return;
	frameCount = 0;
	aFrame = the->frame;
	while (aFrame) {
		frames[frameCount] = aFrame;
		frameCount++;
		aFrame = aFrame->next;
	}
	aFrame = the->frame;
	while (frameCount > 0) {
		frameCount--;
		the->frame = frames[frameCount];
		fxBeginFunction(the);
	}
    c_free(frames);
	the->frame = aFrame;
#endif
}

void fxStopProfiling(txMachine* the)
{
#ifdef mxProfile
	txSlot* aFrame;

	if (!the->profileFile)
		return;
		
	aFrame = the->frame;
	while (the->frame) {
		fxEndFunction(the);
		the->frame = the->frame->next;
	}
	the->frame = aFrame;
	fxRecordProfiling(the, 	0, -1);
	fxWriteProfileRecords(the);
	fxCloseProfileFile(the);
	fxOpenProfileFile(the, "xsprofile.symbols.out");
	fxWriteProfileBOM(the);
	fxWriteProfileSymbols(the);
	fxCloseProfileFile(the);
#endif
}
