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
#include "xs.h"

#include "kxMarkup.h"

#include "FskList.h"
#include "FskMemory.h"
#include "FskString.h"

enum {
	kKinomaPlaylistCanIndex = 1,
	kKinomaPlaylistCanViewEntries
};

typedef struct asxRefRecord asxRefRecord;
typedef struct asxRefRecord *asxRef;

struct asxRefRecord {
	asxRef				next;

	char				url[1];
};

typedef struct asxEntryRecord asxEntryRecord;
typedef struct asxEntryRecord *asxEntry;

struct asxEntryRecord {
	asxEntry			next;

	asxRef				ref;

	char				*title;
	char				*author;
	char				*copyright;
	UInt32				index;
	double				startTime;
	double				duration;
	Boolean				haveStartTime;
	Boolean				haveDuration;
	Boolean				clientSkip;
};

typedef struct {
	UInt32			asxDepth;
	long			characterEncoding;

	asxEntry		entry;

	asxEntry		currentEntry;

	char			**text;
	UInt32			textSize;

	char			*title;
	char			*author;
	char			*copyright;

	Boolean			inRepeat;
	UInt32			repeatStartIndex;
	UInt32			repeatStopIndex;
	SInt32			repeatCount;

	UInt32			capabilities;
} asxParserRecord, *asxParser;

static FskErr parseASX(char *buffer, SInt32 bufferSize, asxParser parser);
static void asxStartTag(void *state, char *tagName, txAttribute *attributes);
static void asxStopTag (void *state, char *tagName);
static void asxText(void *state, char *text);
static long asxGetCharacterEncoding(void *state);

FskErr parseASX(char *buffer, SInt32 bufferSize, asxParser list)
{
	txMarkupCallbacks cb;
	FskMemSet(&cb, 0, sizeof(cb));

	cb.processStartTag = asxStartTag;
	cb.processStopTag = asxStopTag;
	cb.processText = asxText;
	cb.processGetCharacterEncoding = asxGetCharacterEncoding;
	list->asxDepth = 0;
	list->characterEncoding = charEncodingUnknown;

	// We must assume that ASX play lists are not indexable or viewable unless proven otherwise.
	// Many audio streaming servers deliver multiple-entry ASX play lists with alternate URLs.
	// In this case the server does not provide any additional playlist attributes telling the
	// client that the play list cannot be viewed.
	// The playlist capabilities can be overriden with attributes parsed from the play list.
	list->capabilities = 0;
	
	kxParseMarkupBuffer(buffer, bufferSize, NULL, 0, &cb, list);

	list->currentEntry = list->entry;
	
	return kFskErrNone;
}

void asxStartTag(void *state, char *tagName, txAttribute *attributes)
{
	asxParser list = state;
	UInt32 asxDepth;

	list->asxDepth += 1;

	asxDepth = list->asxDepth;
	if (list->inRepeat)
		asxDepth -= 1;

	if ((1 == asxDepth) && (0 == FskStrCompareCaseInsensitive("asx", tagName))) {
		for (; attributes->name && attributes->value; attributes++) {
			if (0 == FskStrCompareCaseInsensitive("kinoma_user_visible", attributes->name)) {
				if (0 == FskStrCompareCaseInsensitive("true", attributes->value)) {
					list->capabilities = (kKinomaPlaylistCanIndex + kKinomaPlaylistCanViewEntries);
				}
				break;
			}
		}
	}
	if ((2 == asxDepth) && (0 == FskStrCompareCaseInsensitive("entry", tagName))) {
		asxEntry entry;
		Boolean clientSkip = true;

		for (; attributes->name && attributes->value; attributes++) {
			if (0 == FskStrCompareCaseInsensitive("clientskip", attributes->name)) {
				if (0 == FskStrCompareCaseInsensitive("yes", attributes->value))
					list->capabilities = kKinomaPlaylistCanIndex;
				else
				if (0 == FskStrCompareCaseInsensitive("no", attributes->value))
					clientSkip = false;
				break;
			}
		}

		if (kFskErrNone == FskMemPtrNewClear(sizeof(asxEntryRecord), &entry)) {
			entry->index = FskListCount(list->entry);
			entry->clientSkip = clientSkip;
			FskListAppend((FskList *)&list->entry, entry);
		}
		list->currentEntry = entry;
	}
	if ((2 == asxDepth) && (0 == FskStrCompareCaseInsensitive("repeat", tagName))) {
		if (false == list->inRepeat) {		// only one repeat allowed, as per spec
			list->inRepeat = true;
			list->repeatStartIndex = FskListCount((FskList)list->entry);
			list->repeatCount = kFskUInt32Max;			// infinite
			for (; attributes->name && attributes->value; attributes++) {
				if (0 == FskStrCompareCaseInsensitive("count", attributes->name)) {
					list->repeatCount = FskStrToNum(attributes->value);
					break;
				}
			}
		}
	}
	else if (((2 == asxDepth) || (3 == asxDepth)) && (0 == FskStrCompareCaseInsensitive("title", tagName))) {
		if (2 == asxDepth)
			list->text = (NULL == list->title) ? &(list->title) : NULL;
		else
		if (list->currentEntry && (NULL == list->currentEntry->title))
			list->text = &(list->currentEntry->title);
	}
	else if (((2 == asxDepth) || (3 == asxDepth)) && (0 == FskStrCompareCaseInsensitive("author", tagName))) {
		if (2 == asxDepth)
			list->text = (NULL == list->author) ? &(list->author) : NULL;
		else if (list->currentEntry && (NULL == list->currentEntry->author))
			list->text = &(list->currentEntry->author);
	}
	else if (((2 == asxDepth) || (3 == asxDepth)) && (0 == FskStrCompareCaseInsensitive("copyright", tagName))) {
		if (2 == asxDepth)
			list->text = (NULL == list->copyright) ? &(list->copyright) : NULL;
		else if (list->currentEntry && (NULL == list->currentEntry->copyright))
			list->text = &(list->currentEntry->copyright);
	}
	else if ((2 == asxDepth) && (0 == FskStrCompareCaseInsensitive("entryref", tagName))) {
		for (; attributes->name && attributes->value; attributes++) {
			if (0 == FskStrCompareCaseInsensitive("href", attributes->name)) {
				asxEntry entry;

				if (kFskErrNone == FskMemPtrNewClear(sizeof(asxEntryRecord), &entry)) {
					asxRef ref;
					entry->index = FskListCount(list->entry);
					entry->clientSkip = true;
					FskListAppend((FskList *)&list->entry, entry);

					if (kFskErrNone == FskMemPtrNewClear(FskStrLen(attributes->value) + sizeof(asxRefRecord), &ref)) {
						FskStrCopy(ref->url, attributes->value);
						if (0 != FskStrStr(ref->url, "avvenu"))
							list->capabilities = (kKinomaPlaylistCanIndex + kKinomaPlaylistCanViewEntries);
						FskListAppend((FskList *)&entry->ref, ref);
					}
				}

				break;
			}
		}
	}
	else
	if ((3 == asxDepth) && (0 == FskStrCompareCaseInsensitive("ref", tagName)) && list->currentEntry) {
		for (; attributes->name && attributes->value; attributes++) {
			if (0 == FskStrCompareCaseInsensitive("href", attributes->name)) {
				asxRef ref;

				if (kFskErrNone == FskMemPtrNewClear(FskStrLen(attributes->value) + sizeof(asxRefRecord), &ref)) {
					FskStrCopy(ref->url, attributes->value);
					if (0 != FskStrStr(ref->url, "avvenu"))
						list->capabilities = (kKinomaPlaylistCanIndex + kKinomaPlaylistCanViewEntries);
					FskListAppend((FskList *)&list->currentEntry->ref, ref);
				}
			}
		}
	}
	else
	if ((3 == asxDepth) && ((0 == FskStrCompareCaseInsensitive("duration", tagName)) || (0 == FskStrCompareCaseInsensitive("starttime", tagName))) && list->currentEntry) {
		for (; attributes->name && attributes->value; attributes++) {
			if (0 == FskStrCompareCaseInsensitive("value", attributes->name)) {
				double numbers[3];
				double value = 0.0;
				UInt32 fields = 0;
				char *p = attributes->value;

				for (fields = 0; (fields < 3); p += 1) {
					numbers[fields++] = FskStrToD(p, &p);
					if (0 == *p)
						break;
				}
				switch (fields) {
					case 1:
						value = numbers[0];
						break;
					case 2:
						value = (numbers[0] * 60.0) + numbers[1];
						break;
					case 3:
						value = (numbers[0] * 60.0 * 60.0) + (numbers[1] * 60.0) + numbers[2];
						break;
				}
				
				if (0 == FskStrCompareCaseInsensitive("starttime", tagName)) {
					list->currentEntry->startTime = value;
					list->currentEntry->haveStartTime = true;
				}
				else {
					list->currentEntry->duration = value;
					list->currentEntry->haveDuration = true;
				}
			}
		}
	}
	else
	if ((2 == asxDepth) && (0 == FskStrCompareCaseInsensitive("param", tagName))) {
		char *name = NULL, *value = NULL;

		for (; attributes->name && attributes->value; attributes++) {
			if (0 == FskStrCompareCaseInsensitive("name", attributes->name))
				name = attributes->value;
			else
			if (0 == FskStrCompareCaseInsensitive("value", attributes->name))
				value = attributes->value;
		}

		if ((NULL != name) && (NULL != value)) {
			if (0 == FskStrCompareCaseInsensitive(name, "encoding")) {
				if (0 == FskStrCompareCaseInsensitive(value, "utf-8"))
					list->characterEncoding = charEncodingUTF8;
			}
			else
			if (0 == FskStrCompareCaseInsensitive(name, "AllowShuffle")) {
				if (0 == FskStrCompareCaseInsensitive(value, "true"))
					list->capabilities = kKinomaPlaylistCanIndex | kKinomaPlaylistCanViewEntries;
			}
		}
	}
}

void asxStopTag(void *state, char *tagName)
{
	asxParser list = state;

	if ((2 == list->asxDepth) && (0 == FskStrCompareCaseInsensitive("repeat", tagName))) {
		if (list->inRepeat) {
			list->inRepeat = false;
			list->repeatStopIndex = FskListCount((FskList)list->entry);
		}
	}

	list->asxDepth -= 1;
	
	list->text = NULL;
	list->textSize = 0;
}

void asxText(void *state, char *text)
{
	asxParser list = state;

	if (list->text) {
		UInt32 textLen = FskStrLen(text);
		if (kFskErrNone == FskMemPtrRealloc(list->textSize + textLen + 1, (FskMemPtr *)list->text)) {
			FskStrCopy(*(list->text) + list->textSize, text);
			list->textSize += textLen;
		}
	}
}

long asxGetCharacterEncoding(void *state)
{
	asxParser list = state;
	return list->characterEncoding;
}


void xs_asx_parse(xsMachine *the)
{
	FskErr err;
	void *data = xsGetHostData(xsArg(0));
	SInt32 dataSize = xsToInteger(xsGet(xsArg(0), xsID("length")));
	SInt32 count, index;
	asxParserRecord list;
	asxEntry walker;

	xsVars(5);

	FskMemSet(&list, 0, sizeof(list));

	err = parseASX(data, dataSize, &list);
	if (err)
		xsThrow(xsErrorPrototype);

	count = FskListCount(list.entry);

	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("asx")), xsID("list")));
	xsSet(xsResult, xsID("length"), xsInteger(count));

	if (kKinomaPlaylistCanViewEntries & list.capabilities)
		xsSet(xsResult, xsID("visible"), xsTrue);

	if (list.title)
		xsSet(xsResult, xsID("title"), xsString(list.title));
	if (list.author)
		xsSet(xsResult, xsID("author"), xsString(list.author));
	if (list.copyright)
		xsSet(xsResult, xsID("copyright"), xsString(list.copyright));

	for (walker = list.entry, index = 0; NULL != walker; walker = walker->next, index += 1) {
		asxRef refWalker;

		xsVar(0) = xsNewInstanceOf(xsGet(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("asx")), xsID("entry")));
		xsSet(xsResult, (xsIndex)index, xsVar(0));

		if (walker->title)
			xsSet(xsVar(0), xsID("title"), xsString(walker->title));
		if (walker->author)
			xsSet(xsVar(0), xsID("author"), xsString(walker->author));
		if (walker->copyright)
			xsSet(xsVar(0), xsID("copyright"), xsString(walker->copyright));
		if (walker->haveStartTime)
			xsSet(xsVar(0), xsID("start"), xsNumber(walker->startTime));
		if (walker->haveDuration)
			xsSet(xsVar(0), xsID("duration"), xsNumber(walker->duration));
		if (!walker->clientSkip)
			xsSet(xsVar(0), xsID("skip"), xsFalse);

		xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
		xsSet(xsVar(0), xsID("uri"), xsVar(1));
		for (refWalker = walker->ref; NULL != refWalker; refWalker = refWalker->next)
			xsCall1_noResult(xsVar(1), xsID("push"), xsString(refWalker->url));
	}

	while (list.entry) {
		asxEntry nextEntry = list.entry->next;

		while (list.entry->ref) {
			asxRef nextRef = list.entry->ref->next;
			FskMemPtrDispose(list.entry->ref);
			list.entry->ref = nextRef;	
		}

		FskMemPtrDispose(list.entry->title);
		FskMemPtrDispose(list.entry->author);
		FskMemPtrDispose(list.entry->copyright);
		FskMemPtrDispose(list.entry);
		list.entry = nextEntry;
	}

	FskMemPtrDispose(list.title);
	FskMemPtrDispose(list.author);
	FskMemPtrDispose(list.copyright);
}



static void justText(void *state, char *text);
static void stripStartTag(void *state, char *tagName, txAttribute *attributes);
static FskErr stripTags(char *in, SInt32 inSize, char **out, SInt32 *outSize);

/*
	lifted from kinoma player 4
*/


FskErr stripTags(char *in, SInt32 inSize, char **out, SInt32 *outSize)
{
	FskErr err;
	txMarkupCallbacks cb;
	char *temp, *ip, *op, *end;
	UInt32 len;

	for (temp = in, len = inSize; 0 != len; len--, temp++) {
		char c = *temp;
		if (('<' == c) || ('&' == c))
			goto doStrip;
	}


	err = FskMemPtrNewFromData(inSize + 1, in, (FskMemPtr *)out);
	if (err) return err;

	(*out)[inSize] = 0;

	if (outSize) *outSize = inSize;

	goto doClean;

doStrip:
	err = FskMemPtrNew(inSize + 10, (FskMemPtr *)out);
	if (err) return err;
	
	temp = *out;
	temp[0] = 0;
	FskStrCopy(temp + 1, "<a>");
	FskMemMove(temp + 4, in, inSize);
	FskMemMove(temp + 4 + inSize, "</a>", 5);

	FskMemSet(&cb, 0, sizeof(cb));
	cb.processStartTag = stripStartTag;
	cb.processStopTag = NULL;
	cb.processText = justText;
	kxParseMarkupBuffer(temp + 1, inSize + 7, NULL, 0, &cb, &temp);

	inSize = FskStrLen(*out);

doClean:
	ip = *out;
	op = ip;
	end = ip + inSize + 1;

	// strip leading white space
	while (ip < end) {
		if ((*ip == '\t') || (*ip == ' ') || (10 == *ip) || (13 == *ip))
			ip += 1;
		else
			break;
	}

	// clean up line endings, tabs, etc
	while (ip < end) {
		char oc = *ip++;

		if (oc == '\t') {
			oc = ' ';		// tabs to space
		}
		else
		if (((10 == oc) && (13 == ip[0])) || ((13 == oc) && (10 == ip[0]))) {
			oc = 10;
			ip += 1;
		}
		else
		if (13 == oc)
			oc = 10;

		if (10 == oc) {
			// supress multiple lines - including those full of spaces
			char *t = op - 1;
			while (t >= *out) {
				oc = *t;
				if (10 == oc)
					op = t;
				else if (' ' != oc)
					break;
				t -= 1;
			}
			oc = 10;
		}
		*op++ = oc;
	}
	
	// strip trailing white space
	ip = *out + FskStrLen(*out) - 1;
	while (ip >= *out) {
		char c = *ip;
		if ((' ' == c) || (10 == c)) {
			*ip = 0;
			ip--;
		}
		else
			break;
	}

	len = FskStrLen(*out);
	if (0 == len)
		FskMemPtrDisposeAt((void **)out);

	if (NULL != outSize)
		*outSize = len;

	return kFskErrNone;
}

void justText(void *state, char *text)
{
	char **out = state;

	FskStrCat(*out, text);
	*out += FskStrLen(text);
}

void stripStartTag(void *state, char *tagName, txAttribute *attributes)
{
	if ((0 == FskStrCompareCaseInsensitive(tagName, "br")) || (0 == FskStrCompareCaseInsensitive(tagName, "p"))) {
		char **out = state;
		char *p = *out;
		p[0] = 10;
		p[1] = 0;
		*out += 1;
	}
}

void xs_asx_strip(xsMachine *the)
{
	FskErr err = kFskErrNone;
	char *str = xsToString(xsArg(0));
	char *out = NULL;

	err = stripTags(str, FskStrLen(str), &out, NULL);
	if (kFskErrNone == err) {
		if (out)
			xsResult = xsString(out);
		else
			xsResult = xsString("");
		FskMemPtrDispose(out);
	}
	else
		xsResult = xsArg(0);
}
