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
#define _XOPEN_SOURCE
#define __FSKHTTPSERVER_PRIV__
#include "kprLibrary.h"

#include "FskAAScaleBitmap.h"
#include "FskEnvironment.h"
#include "FskExtensions.h"
#include "FskECMAScript.h"
#include "FskHTTPServer.h"
#include "FskImage.h"
#include "FskMediaPlayer.h"
#include "FskThread.h"

#include "kprHTTPClient.h"
#include "kprImage.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"
#include "kprUtilities.h"
#include "kprLibraryServer.h"

#include "FskHardware.h"
#include <android/log.h>

#define HELLO_MSG "kprLibrary build " __DATE__ " " __TIME__

static void LOGDV(const char *fmt, va_list ap) {
	gAndroidCallbacks->logVPrintCB(ANDROID_LOG_DEBUG, "basuke", fmt, ap);
}

static void LOGD(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	LOGDV(fmt, ap);
	va_end(ap);
}



struct KprLibraryStruct {
	xsMachine* the;
	KprMessage message;
	KprLibraryServer server;
};

typedef struct KprLibraryAlbumViewStruct {
	FskInt64 id;
	char* album_name;
	char* artist_name;
	SInt32 song_count;
	char* initial;
} KprLibraryAlbumViewRecord;

typedef struct KprLibraryArtistViewStruct {
	FskInt64 id;
	char* artist_name;
	char* album_name;
	SInt32 album_count;
	SInt32 song_count;
	char* initial;
	FskInt64 album_id;
} KprLibraryArtistViewRecord;

typedef struct KprLibraryGenreViewStruct {
	FskInt64 id;
	char* genre_name;
	SInt32 song_count;
	char* initial;
	FskInt64 album_id;
} KprLibraryGenreViewRecord;

typedef struct KprLibraryAudioViewStruct {
	FskInt64 id;
	double date;
	FskInt64 size;
	char* mime;
	char* title;
	double duration;
	double time;
	char* artist_name;
	char* album_name;
	SInt32 track;
	char* path;
	char* initial;
	FskInt64 album_id;
} KprLibraryAudioViewRecord;

typedef struct KprLibraryImageViewStruct {
	FskInt64 id;
	double date;
	char* mime;
	FskInt64 size;
	char* title;
	SInt32 width;
	SInt32 height;
	SInt32 rotation;
	double taken;
	char* path;
} KprLibraryImageViewRecord;

typedef struct KprLibraryVideoViewStruct {
	FskInt64 id;
	double date;
	char* mime;
	FskInt64 size;
	char* title;
	SInt32 width;
	SInt32 height;
	double duration;
	double time;
	char* path;
} KprLibraryVideoViewRecord;

static FskErr KprLibraryNew();
static void KprLibraryDispose();
static void KprLibraryCancel(KprService service, KprMessage message);
extern void KprLibraryInvoke(KprService service, KprMessage message);
extern void KprLibraryStart(KprService service, FskThread thread, xsMachine* the);
extern void KprLibraryStop(KprService service);

static void KprLibraryPushAlbum(xsMachine* the, const KprLibraryAlbumViewRecord *albumView);
static void KprLibraryPushArtist(xsMachine* the, const KprLibraryArtistViewRecord *genreView);
static void KprLibraryPushGenre(xsMachine* the, const KprLibraryGenreViewRecord *genreView);
static void KprLibraryPushAudio(xsMachine* the, const KprLibraryAudioViewRecord *audioView);
static void KprLibraryPushImage(xsMachine* the, const KprLibraryImageViewRecord *imageView);
static void KprLibraryPushVideo(xsMachine* the, const KprLibraryVideoViewRecord *videoView);

static char* KprLibraryParseAlbum(char *str, KprLibraryAlbumViewRecord *albumView);
static char* KprLibraryParseArtist(char *str, KprLibraryArtistViewRecord *artistView);
static char* KprLibraryParseGenre(char *str, KprLibraryGenreViewRecord *genreView);
static char* KprLibraryParseAudio(char *str, KprLibraryAudioViewRecord *audioView);
static char* KprLibraryParseImage(char *str, KprLibraryImageViewRecord *imageView);
static char* KprLibraryParseVideo(char *str, KprLibraryVideoViewRecord *videoView);

static void KprLibraryFetchArtistAlbums(xsMachine* the, const char *artist);
static void KprLibraryFetchAlbums(xsMachine* the);

static void KprLibraryFetchArtists(xsMachine* the);

static void KprLibraryFetchGenres(xsMachine* the);

static void KprLibraryFetchSongsOfAlbum(xsMachine* the, const char *album);
static void KprLibraryFetchSongsOfArtist(xsMachine* the, const char *artist);
static void KprLibraryFetchSongsOfGenre(xsMachine* the, const char *genre);
static void KprLibraryFetchSongs(xsMachine* the);

static void KprLibraryFetchImagesByDate(xsMachine* the);
static void KprLibraryFetchImagesByDimensions(xsMachine* the);
static void KprLibraryFetchImagesBySize(xsMachine* the);
static void KprLibraryFetchImagesByTitle(xsMachine* the);
static FskErr KprLibrarySaveImage(KprLibrary library, UInt8 *data, UInt32 size);

static void KprLibraryFetchVideosByDate(xsMachine* the);
static void KprLibraryFetchVideosByDimensions(xsMachine* the);
static void KprLibraryFetchVideosByDuration(xsMachine* the);
static void KprLibraryFetchVideosBySize(xsMachine* the);
static void KprLibraryFetchVideosByTitle(xsMachine* the);

static FskErr KprLibraryLoadThumbnail(char *path, FskBitmap *bitmap);
static FskErr KprLibraryGetThumbnail(char *kind, long id, Boolean micro, char* rotation, FskBitmap *bitmap);
static FskErr KprLibraryFetchAlbumArtMetadata(char *path, Boolean micro, FskBitmap *bitmap);

static char *KprURLCopyPath(KprURLParts parts);
static FskErr KprLibrarySniffForMIME(FskHeaders *headers, char *path, char **outMime);

static char *KprLibraryGetAlbumSongPath(long album_id, int index);

enum {
	kItemArrayVarIndex = 0,
	kItemVarIndex,
	kInitialItemIndex,

	kInitialCount = 26 + 1,
};

enum {
	kVarCount = kItemVarIndex + 1,
	kVarCountForSort = kInitialItemIndex + kInitialCount,
};

#define xsVarItems xsVar(kItemArrayVarIndex)
#define xsVarItem xsVar(kItemVarIndex)
#define xsVarInitial(x) xsVar(kInitialItemIndex + (x))

//--------------------------------------------------
// Scanner
//--------------------------------------------------

static KprLibraryRecord gLibrary = { NULL, NULL, NULL };
KprServiceRecord gLibraryService = {
	NULL,
	0,
	"xkpr://library/",
	NULL,
	NULL,
	KprServiceAccept,
	KprLibraryCancel,
	KprLibraryInvoke,
	KprLibraryStart,
	KprLibraryStop,
	NULL,
	NULL,
	NULL
};
static FskThread gLibraryThread = NULL;
static Boolean gLibraryThreadQuitting = false;

static void KprLibraryLoop(void* theParameter UNUSED)
{
	FskErr err = kFskErrNone;
	FskThreadInitializationComplete(FskThreadGetCurrent());
	bailIfError(KprLibraryNew());
	while (!gLibraryThreadQuitting)
		FskThreadRunloopCycle(-1);
	KprLibraryDispose();
bail:
	return;
}

void KprLibraryStart(KprService service, FskThread thread, xsMachine* the)
{
	FskErr err = kFskErrNone;
	UInt32 flags = kFskThreadFlagsWaitForInit | kFskThreadFlagsJoinable;
	gLibraryThreadQuitting = false;
	bailIfError(FskThreadCreate(&gLibraryThread, KprLibraryLoop, flags, NULL, "Library"));
bail:
	return;
}

void KprLibraryStop(KprService service)
{
	gLibraryThreadQuitting = true;
	FskThreadJoin(gLibraryThread);
}

FskExport(FskErr) kprLibrary_fskLoad(FskLibrary it)
{
	KprServiceRegister(&gLibraryService);
	LOGD(HELLO_MSG);
	return kFskErrNone;
}

FskExport(FskErr) kprLibrary_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

FskErr KprLibraryNew()
{
	xsAllocation allocation = {
		1100000,		// 700 * 1024,
		32 * 1024,
		70000,			// 62000,
		4096,
		2048,
		16000,
		1993
	};
	FskErr err = kFskErrNone;
	KprLibrary self = &gLibrary;
	
	bailIfError(KprLibraryServerNew(&self->server));

	self->the = xsAliasMachine(&allocation, gShell->root, "library", self);
	bailIfNULL(self->the);
	
	gLibraryService.machine = self->the;
	gLibraryService.thread = gLibraryThread;

bail:
	if (err != kFskErrNone) {
		if (self->server) KprLibraryServerDispose(self->server);
		if (self->the) xsDeleteMachine(self->the);
		
		self->server = NULL;
		self->the = NULL;
	}
	return err;
}

void KprLibraryDispose()
{
	KprLibrary self = &gLibrary;
	
	if (self->server)
		KprLibraryServerDispose(self->server);

	if (self->the)
		xsDeleteMachine(self->the);

	self->server = NULL;
	self->the = NULL;
}

void KprLibraryCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprLibraryInvoke(KprService service, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (KprMessageContinue(message)) {
		KprLibrary self = &gLibrary;
		FskAssociativeArray query = NULL;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryBegin, message)
		gLibrary.message = message;
		if (message->parts.query)
			bailIfError(KprQueryParse(message->parts.query, &query));
		if (FskStrCompareWithLength("/server/", message->parts.path, 8) == 0) {
			if (FskStrCompareWithLength("start", message->parts.name, message->parts.nameLength) == 0) {
				char* value = FskAssociativeArrayElementGetString(query, "port");
				KprLibraryServerStart(self->server, value);
			}
			else if (FskStrCompareWithLength("stop", message->parts.name, message->parts.nameLength) == 0) {
				KprLibraryServerStop(self->server);
			}
		}
		else if (FskStrCompareWithLength("movies", message->parts.name, message->parts.nameLength) == 0) {
			xsBeginHostSandboxCode(self->the, NULL);
			{
				xsVars(kVarCount);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						char* value = FskAssociativeArrayElementGetString(query, "sort");
						xsVarItems = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompare("date", value) == 0)
							KprLibraryFetchVideosByDate(the);
						else if (FskStrCompare("dimensions", value) == 0)
							KprLibraryFetchVideosByDimensions(the);
						else if (FskStrCompare("duration", value) == 0)
							KprLibraryFetchVideosByDuration(the);
						else if (FskStrCompare("size", value) == 0)
							KprLibraryFetchVideosBySize(the);
						else
							KprLibraryFetchVideosByTitle(the);
						target->result = xsMarshall(xsVarItems);
					}
					xsCatch {
					}
				}
			}
			xsEndHostSandboxCode();
		}
		else if (FskStrCompareWithLength("/music/", message->parts.path, 7) == 0) {
			xsBeginHostSandboxCode(self->the, NULL);
			{
				xsVars(kVarCountForSort);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						xsVarItems = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompareWithLength("albums", message->parts.name, message->parts.nameLength) == 0) {
							char* value = FskAssociativeArrayElementGetString(query, "artist");
							if (value)
								KprLibraryFetchArtistAlbums(the, value);
							else
								KprLibraryFetchAlbums(the);
						}
						else if (FskStrCompareWithLength("artists", message->parts.name, message->parts.nameLength) == 0)
							KprLibraryFetchArtists(the);
						else if (FskStrCompareWithLength("genres", message->parts.name, message->parts.nameLength) == 0)
							KprLibraryFetchGenres(the);
						else {
							char* value;
							if ((value = FskAssociativeArrayElementGetString(query, "album")))
								KprLibraryFetchSongsOfAlbum(the, value);
							else if ((value = FskAssociativeArrayElementGetString(query, "artist")))
								KprLibraryFetchSongsOfArtist(the, value);
							else if ((value = FskAssociativeArrayElementGetString(query, "genre")))
								KprLibraryFetchSongsOfGenre(the, value);
							else
								KprLibraryFetchSongs(the);
						}
						target->result = xsMarshall(xsVarItems);
					}
					xsCatch {
					}
				}
			}
			xsEndHostSandboxCode();
		}
		else if (FskStrCompareWithLength("pictures", message->parts.name, message->parts.nameLength) == 0) {
			xsBeginHostSandboxCode(self->the, NULL);
			{
				xsVars(kVarCount);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						char* value = FskAssociativeArrayElementGetString(query, "sort");
						xsVarItems = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompare("date", value) == 0)
							KprLibraryFetchImagesByDate(the);
						else if (FskStrCompare("dimensions", value) == 0)
							KprLibraryFetchImagesByDimensions(the);
						else if (FskStrCompare("size", value) == 0)
							KprLibraryFetchImagesBySize(the);
						else
							KprLibraryFetchImagesByTitle(the);
						target->result = xsMarshall(xsVarItems);
					}
					xsCatch {
					}
				}
			}
			xsEndHostSandboxCode();
		}
		else if (FskStrCompareWithLength("/pictures/", message->parts.path, 10) == 0) {
			if (FskStrCompareWithLength("save", message->parts.name, message->parts.nameLength) == 0) {
				bailIfError(KprLibrarySaveImage(self, message->request.body, message->request.size));
			}
		}
		else if (FskStrCompareWithLength("thumbnail", message->parts.name, message->parts.nameLength) == 0) {
			if (query) {
				char* kind = FskAssociativeArrayElementGetString(query, "kind");
				char* id = FskAssociativeArrayElementGetString(query, "id");
				char* rotation = FskAssociativeArrayElementGetString(query, "rotation");
				KprImageTarget target = (KprImageTarget)message->stream;
				if (kind && *kind && id && *id) {
					Boolean micro = (target->width <= 96 && target->height < 96);

					long mediaId = FskStrToFskInt64(id);
					bailIfError(KprLibraryGetThumbnail(kind, mediaId, micro, rotation, (FskBitmap*)&target->bitmap));
				}
			}
		}
	bail:
		gLibrary.message = NULL;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		FskAssociativeArrayDispose(query);
		message->error = err;
	}
	KprMessageComplete(message);
}

xsMachine* KprLibraryMachine()
{
	return gLibrary.the;
}

static void KprLibrary_setStringSlot(xsMachine* the, char *slot, xsStringValue value) {
	xsNewHostProperty (xsVarItem, xsID(slot), xsString(value), xsDefault, xsDontScript);
}

static void KprLibrary_setIntegerSlot(xsMachine* the, char *slot, xsIntegerValue value) {
	xsNewHostProperty (xsVarItem, xsID(slot), xsInteger(value), xsDefault, xsDontScript);
}

static void KprLibrary_setNumberSlot(xsMachine* the, char *slot, xsNumberValue value) {
	xsNewHostProperty (xsVarItem, xsID(slot), xsNumber(value), xsDefault, xsDontScript);
}

static void KprLibrary_setUrlSlotV(xsMachine* the, char *slot, char *fmt, va_list ap) {
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, ap);

	KprLibrary_setStringSlot(the, slot, buf);
}

static void KprLibrary_setUrlSlot(xsMachine* the, char *slot, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	KprLibrary_setUrlSlotV(the, slot, fmt, ap);
	va_end(ap);
}

static char *KprLibrary_encode(xsMachine* the, char *str) {
	xsResult = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString(str));
	return xsToString(xsResult);
}

static void KprLibraryPushAlbum(xsMachine* the, const KprLibraryAlbumViewRecord *view)
{
	xsVarItem = xsNewInstanceOf(xsObjectPrototype);

	KprLibrary_setStringSlot(the, "album", view->album_name);
	KprLibrary_setStringSlot(the, "artist", view->artist_name);
	KprLibrary_setIntegerSlot(the, "songs", view->song_count);
	
	KprLibrary_setUrlSlot(the, "thumbnail", "xkpr://library/thumbnail?kind=album&id=%lld", view->id);
	KprLibrary_setUrlSlot(the, "contents", "xkpr://library/music/songs?album=%s", KprLibrary_encode(the, view->album_name));
	
	KprLibrary_setStringSlot(the, "initial", view->initial);

	xsCall1(xsVar(0), xsID("push"), xsVarItem);
}

static void KprLibraryPushArtist(xsMachine* the, const KprLibraryArtistViewRecord *view)
{
	xsVarItem = xsNewInstanceOf(xsObjectPrototype);

	KprLibrary_setIntegerSlot(the, "albums", view->album_count);
	KprLibrary_setStringSlot(the, "artist", view->artist_name);
	KprLibrary_setIntegerSlot(the, "songs", view->song_count);
	
	KprLibrary_setUrlSlot(the, "thumbnail", "xkpr://library/thumbnail?kind=album&id=%lld", view->album_id);
	
	if (view->album_count == 1) {
		KprLibrary_setStringSlot(the, "album", view->album_name);
		KprLibrary_setUrlSlot(the, "contents", "xkpr://library/music/songs?album=%s", KprLibrary_encode(the, view->album_name));
	} else {
		KprLibrary_setUrlSlot(the, "contents", "xkpr://library/music/albums?artist=%lld", view->id);
	}

	KprLibrary_setStringSlot(the, "initial", view->initial);

	xsCall1(xsVar(0), xsID("push"), xsVarItem);
}

static void KprLibraryPushGenre(xsMachine* the, const KprLibraryGenreViewRecord *view)
{
	xsVarItem = xsNewInstanceOf(xsObjectPrototype);

	KprLibrary_setStringSlot(the, "genre", view->genre_name);
	KprLibrary_setIntegerSlot(the, "songs", view->song_count);
	
	KprLibrary_setUrlSlot(the, "thumbnail", "xkpr://library/thumbnail?kind=album&id=%lld", view->album_id);
	KprLibrary_setUrlSlot(the, "contents", "xkpr://library/music/songs?genre=%s", KprLibrary_encode(the, view->genre_name));
	
	KprLibrary_setStringSlot(the, "initial", view->initial);

	xsCall1(xsVar(0), xsID("push"), xsVarItem);
}

static void KprLibraryPushAudio(xsMachine* the, const KprLibraryAudioViewRecord *view)
{
	xsVarItem = xsNewInstanceOf(xsObjectPrototype);

	KprLibrary_setStringSlot(the, "album", view->album_name);
	KprLibrary_setStringSlot(the, "artist", view->artist_name);
	KprLibrary_setNumberSlot(the, "date", view->date);
	KprLibrary_setNumberSlot(the, "duration", view->duration);
	KprLibrary_setNumberSlot(the, "size", view->size);
	KprLibrary_setStringSlot(the, "title", view->title);
	KprLibrary_setIntegerSlot(the, "track", view->track);
	
	KprLibrary_setIntegerSlot(the, "kind", kprLibraryFileKind);
	KprLibrary_setUrlSlot(the, "thumbnail", "xkpr://library/thumbnail?kind=album&id=%lld", view->album_id);
	
	KprLibrary_setStringSlot(the, "mime", view->mime);
	KprLibrary_setUrlSlot(the, "url", "file://%s", view->path);
	
	KprLibrary_setStringSlot(the, "initial", view->initial);

	xsCall1(xsVar(0), xsID("push"), xsVarItem);
}

static void KprLibraryPushImage(xsMachine* the, const KprLibraryImageViewRecord *view)
{
	xsVarItem = xsNewInstanceOf(xsObjectPrototype);

	KprLibrary_setNumberSlot(the, "date", view->taken);
	KprLibrary_setNumberSlot(the, "size", view->size);
	KprLibrary_setStringSlot(the, "title", view->title);
	KprLibrary_setIntegerSlot(the, "width", view->width);
	KprLibrary_setIntegerSlot(the, "height", view->height);
	KprLibrary_setIntegerSlot(the, "rotation", view->rotation);
	
	KprLibrary_setIntegerSlot(the, "kind", kprLibraryFileKind);
	KprLibrary_setUrlSlot(the, "thumbnail", "xkpr://library/thumbnail?kind=image&id=%lld&rotation=%ld", view->id, view->rotation);
	
	KprLibrary_setStringSlot(the, "mime", view->mime);
	KprLibrary_setUrlSlot(the, "url", "file://%s", view->path);
	
	xsCall1(xsVar(0), xsID("push"), xsVarItem);
}

static void KprLibraryPushVideo(xsMachine* the, const KprLibraryVideoViewRecord *view)
{
	xsVarItem = xsNewInstanceOf(xsObjectPrototype);

	KprLibrary_setNumberSlot(the, "date", view->date);
	KprLibrary_setNumberSlot(the, "duration", view->duration);
	KprLibrary_setNumberSlot(the, "size", view->size);
	KprLibrary_setStringSlot(the, "title", view->title);
	KprLibrary_setIntegerSlot(the, "width", view->width);
	KprLibrary_setIntegerSlot(the, "height", view->height);
	
	KprLibrary_setIntegerSlot(the, "kind", kprLibraryFileKind);
	KprLibrary_setUrlSlot(the, "thumbnail", "xkpr://library/thumbnail?kind=video&id=%lld", view->id);
	
	KprLibrary_setStringSlot(the, "mime", view->mime);
	KprLibrary_setUrlSlot(the, "url", "file://%s", view->path);
	
	xsCall1(xsVar(0), xsID("push"), xsVarItem);
}

/* ALBUM */

static void KprLibraryFetchArtistAlbums(xsMachine* the, const char *artist)
{
	// kprLibraryAudioViewGetArtistAlbums
    // { "FROM audio_view WHERE artist_name = ? GROUP BY album_name ORDER BY album_name", "t", "rtti", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("album", "artist", artist);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryAlbumViewRecord record;
			str = KprLibraryParseAlbum(str, &record);
			KprLibraryPushAlbum(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchAlbums(xsMachine* the)
{
	// kprLibraryAudioViewGetAlbums
    // { "FROM audio_view GROUP BY album_name ORDER BY album_name", NULL, "rtti", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("album", NULL, NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryAlbumViewRecord record;
			str = KprLibraryParseAlbum(str, &record);
			KprLibraryPushAlbum(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

/* ARTIST */

static void KprLibraryFetchArtists(xsMachine* the)
{
	// kprLibraryAudioViewGetArtists
	// { "FROM audio_view GROUP BY artist_name ORDER BY artist_name", NULL, "rttii", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("artist", NULL, NULL);
	if (result) {
		LOGD("artist views: %s", result);
		char *str = result;
		while (*str) {
			KprLibraryArtistViewRecord record;
			str = KprLibraryParseArtist(str, &record);
			KprLibraryPushArtist(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

/* GENRE */

static void KprLibraryFetchGenres(xsMachine* the)
{
	// kprLibraryAudioViewGetGenres
    // { "FROM audio_view GROUP BY genre_name ORDER BY genre_name ", NULL, "rti", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("genre", NULL, NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryGenreViewRecord record;
			str = KprLibraryParseGenre(str, &record);
			KprLibraryPushGenre(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

/* SONG */

static void KprLibraryFetchSongsOfAlbum(xsMachine* the, const char *album)
{
	// kprLibraryAudioViewGetAlbumSongs
	// { "FROM audio_view WHERE album_name = ? ORDER by track", "t", "rdtIttddttit", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("song", "album", album);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryAudioViewRecord record;
			str = KprLibraryParseAudio(str, &record);
			KprLibraryPushAudio(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchSongsOfArtist(xsMachine* the, const char *artist)
{
	// kprLibraryAudioViewGetArtistSongs
	// { "FROM audio_view WHERE artist_name = ? ORDER by album_name,track", "t", "rdtIttddttit", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("song", "artist", artist);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryAudioViewRecord record;
			str = KprLibraryParseAudio(str, &record);
			KprLibraryPushAudio(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchSongsOfGenre(xsMachine* the, const char *genre)
{
	// kprLibraryAudioViewGetGenreSongs
	// { "FROM audio_view WHERE genre_name = ? ORDER by title", "t", "rdtIttddttit", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("song", "genre", genre);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryAudioViewRecord record;
			str = KprLibraryParseAudio(str, &record);
			KprLibraryPushAudio(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchSongs(xsMachine* the)
{
	// kprLibraryAudioViewGetSongs
	// { "FROM audio_view ORDER by title", NULL, "rdtIttddttit", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("song", NULL, NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryAudioViewRecord record;
			str = KprLibraryParseAudio(str, &record);
			KprLibraryPushAudio(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

/* IMAGE */

static void KprLibraryFetchImagesByDate(xsMachine* the)
{
	// kprLibraryImageViewGetByDate
	// { "FROM image_view ORDER by taken DESC", NULL, "rdtIttiiid", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("image", "date", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryImageViewRecord record;
			str = KprLibraryParseImage(str, &record);
			KprLibraryPushImage(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchImagesByDimensions(xsMachine* the)
{
	// kprLibraryImageViewGetByDimensions
	// { "FROM image_view ORDER by area, title", NULL, "rdtIttiiid", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("image", "dimensions", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryImageViewRecord record;
			str = KprLibraryParseImage(str, &record);
			KprLibraryPushImage(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchImagesBySize(xsMachine* the)
{
	// kprLibraryImageViewGetBySize
	// { "FROM image_view ORDER by size, title", NULL, "rdtIttiiid", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("image", "size", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryImageViewRecord record;
			str = KprLibraryParseImage(str, &record);
			KprLibraryPushImage(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchImagesByTitle(xsMachine* the)
{
	// kprLibraryImageViewGetByTitle
	// { "FROM image_view ORDER by title", NULL, "rdtIttiiid", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("image", "title", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryImageViewRecord record;
			str = KprLibraryParseImage(str, &record);
			KprLibraryPushImage(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static FskErr KprLibrarySaveImage(KprLibrary library, UInt8 *data, UInt32 size)
{
	FskErr err = kFskErrNone;

	//TODO: Add more metadata, such as orientation
	err = gAndroidCallbacks->librarySaveImageCB(data, size);

	return err;
}
/* VIDEO */

static void KprLibraryFetchVideosByDate(xsMachine* the)
{
	//	kprLibraryVideoViewGetByDate
	//	{ "FROM video_view ORDER by date DESC", NULL, "rdtIttiidd", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("video", "date", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryVideoViewRecord record;
			str = KprLibraryParseVideo(str, &record);
			KprLibraryPushVideo(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchVideosByDimensions(xsMachine* the)
{
	// kprLibraryVideoViewGetByDimensions
	// { "FROM video_view ORDER by area, title", NULL, "rdtIttiidd", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("video", "dimensions", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryVideoViewRecord record;
			str = KprLibraryParseVideo(str, &record);
			KprLibraryPushVideo(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchVideosByDuration(xsMachine* the)
{
	// kprLibraryVideoViewGetByDuration
	// { "FROM video_view ORDER by duration, title", NULL, "rdtIttiidd", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("video", "duration", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryVideoViewRecord record;
			str = KprLibraryParseVideo(str, &record);
			KprLibraryPushVideo(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchVideosBySize(xsMachine* the)
{
	// kprLibraryVideoViewGetBySize
	// { "FROM video_view ORDER by size, title", NULL, "rdtIttiidd", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("video", "size", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryVideoViewRecord record;
			str = KprLibraryParseVideo(str, &record);
			KprLibraryPushVideo(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static void KprLibraryFetchVideosByTitle(xsMachine* the)
{
	// kprLibraryVideoViewGetByTitle
	// { "FROM video_view ORDER by title", NULL, "rdtIttiidd", NULL},
	char *result = gAndroidCallbacks->libraryFetchCB("video", "title", NULL);
	if (result) {
		char *str = result;
		while (*str) {
			KprLibraryVideoViewRecord record;
			str = KprLibraryParseVideo(str, &record);
			KprLibraryPushVideo(the, &record);
		}
		FskMemPtrDispose(result);
	}
}

static char* KprLibraryNextRecord(char *str) {
	while (*str && *str != '\n') str++;
	if (*str == '\n') *str++ = 0;
	return str;
}

static char* KprLibraryNextField(char **str, char *end)
{
	char *field, *src, *dest;
	int escaping = 0;

	field = src = dest = *str;

	if (end > src && *(end - 1) == 0) end -= 1;

	while (src < end) {
		char c = *src++;

		if (escaping) {
			switch (c) {
				case 't':
					c = '\t';
					break;
					
				case 'n':
					c = '\n';
					break;
					
				case '\\':
					c = '\\';
					break;
					
				default:
					break;
			}

			*dest++ = c;
			escaping = 0;
		} else {
			if (c == '\\') {
				escaping = 1;
			} else if (c == '\t') {
				break;
			} else {
				*dest++ = c;
			}
		}
	}

	*dest = 0;
	*str = src;
	return field;
}

static char* KprLibraryParseAlbum(char *str, KprLibraryAlbumViewRecord *view)
{
	char *next = KprLibraryNextRecord(str);

	view->id = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->album_name = KprLibraryNextField(&str, next);
	view->artist_name = KprLibraryNextField(&str, next);
	view->song_count = FskStrToNum(KprLibraryNextField(&str, next));
	view->initial = KprLibraryNextField(&str, next);

	return next;
}

static char* KprLibraryParseArtist(char *str, KprLibraryArtistViewRecord *view)
{
	char *next = KprLibraryNextRecord(str);

	view->id = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->artist_name = KprLibraryNextField(&str, next);
	view->album_name = KprLibraryNextField(&str, next);
	view->album_count = FskStrToNum(KprLibraryNextField(&str, next));
	view->song_count = FskStrToNum(KprLibraryNextField(&str, next));
	view->initial = KprLibraryNextField(&str, next);
	view->album_id = FskStrToFskInt64(KprLibraryNextField(&str, next));

	return next;
}

static char* KprLibraryParseGenre(char *str, KprLibraryGenreViewRecord *view)
{
	char *next = KprLibraryNextRecord(str);

	view->id = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->genre_name = KprLibraryNextField(&str, next);
	view->song_count = FskStrToNum(KprLibraryNextField(&str, next));
	view->initial = KprLibraryNextField(&str, next);
	view->album_id = FskStrToFskInt64(KprLibraryNextField(&str, next));

	return next;
}

static char* KprLibraryParseAudio(char *str, KprLibraryAudioViewRecord *view)
{
	char *next = KprLibraryNextRecord(str);

	char *dummy;
	
	view->id = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->date = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->size = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->mime = KprLibraryNextField(&str, next);
	view->title = KprLibraryNextField(&str, next);
	view->duration = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->time = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->artist_name = KprLibraryNextField(&str, next);
	view->album_name = KprLibraryNextField(&str, next);
	view->track = FskStrToNum(KprLibraryNextField(&str, next));
	view->path = KprLibraryNextField(&str, next);
	view->initial = KprLibraryNextField(&str, next);
	view->album_id = FskStrToFskInt64(KprLibraryNextField(&str, next));

	return next;
}

static char* KprLibraryParseImage(char *str, KprLibraryImageViewRecord *view)
{
	char *next = KprLibraryNextRecord(str);

	char *dummy;
	
	view->id = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->date = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->mime = KprLibraryNextField(&str, next);
	view->size = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->title = KprLibraryNextField(&str, next);
	view->width = FskStrToNum(KprLibraryNextField(&str, next));
	view->height = FskStrToNum(KprLibraryNextField(&str, next));
	view->rotation = FskStrToNum(KprLibraryNextField(&str, next));
	view->taken = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->path = KprLibraryNextField(&str, next);

	return next;
}

static char* KprLibraryParseVideo(char *str, KprLibraryVideoViewRecord *view)
{
	char *next = KprLibraryNextRecord(str);

	char *dummy;
	
	view->id = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->date = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->mime = KprLibraryNextField(&str, next);
	view->size = FskStrToFskInt64(KprLibraryNextField(&str, next));
	view->title = KprLibraryNextField(&str, next);
	view->width = FskStrToNum(KprLibraryNextField(&str, next));
	view->height = FskStrToNum(KprLibraryNextField(&str, next));
	view->duration = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->time = FskStrToD(KprLibraryNextField(&str, next), &dummy);
	view->path = KprLibraryNextField(&str, next);

	return next;
}

/* SERVER */

static FskErr KprScanThumbnailScale(FskBitmap* bitmap, SInt32 width, SInt32 height)
{
	FskErr err = kFskErrNone;
	FskBitmap src, dst;
	FskBitmapFormatEnum pixelFormat = kFskBitmapFormatUnknown;
	FskRectangleRecord srcRect, dstRect;
	src = *bitmap;
	FskBitmapGetPixelFormat(src, &pixelFormat);
	FskBitmapGetBounds(src, &srcRect);
	if ((srcRect.width > width) || (srcRect.height > height)) {
		FskRectangleSet(&dstRect, 0, 0, width, height);
		FskRectangleScaleToFit(&dstRect, &srcRect, &dstRect);
		FskRectangleOffset(&dstRect, -dstRect.x, -dstRect.y);
		bailIfError(FskBitmapNew(dstRect.width, dstRect.height, pixelFormat, &dst));
		bailIfError(FskAAScaleBitmap(kAAScaleTentKernelType, src, NULL, dst, &dstRect));
		FskBitmapDispose(src);
		*bitmap = dst;
	}
bail:
	return err;
}

FskErr KprDataServerOpen(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	FskAssociativeArray query = NULL;
	char* kind;
	long id;
	FskBitmap bitmap = NULL;
	FskRectangleRecord bounds;
	FskImageCompress comp = NULL;
	FskMediaPropertyValueRecord property;
	if (FskStrCompareWithLength("xkpr", session->parts.scheme, 4) == 0) {
		if (FskStrCompareWithLength("/thumbnail", session->parts.path, session->parts.pathLength) == 0) {
			KprQueryParse(session->parts.query, &query);
			kind = FskAssociativeArrayElementGetString(query, "kind");
			id = FskStrToNum(FskAssociativeArrayElementGetString(query, "id"));
			bailIfError(KprLibraryGetThumbnail(kind, id, false, NULL, &bitmap));
			KprScanThumbnailScale(&bitmap, 320, 320);
			bailIfError(FskImageCompressNew(&comp, 0, "image/jpeg", 320, 320));
			property.type = kFskMediaPropertyTypeFloat;
			property.value.number = 0.6;
			FskImageCompressSetProperty(comp, kFskMediaPropertyQuality, &property);
			bailIfError(FskImageCompressFrame(comp, bitmap, &session->data.buffer, &session->data.size, NULL, NULL, NULL, NULL, NULL));
		}
		else {
			err = KprLibrarySessionRedirect(session);
		}
	}
	else
		err = kFskErrNotFound;
bail:
	FskImageCompressDispose(comp);
	FskBitmapDispose(bitmap);
	FskAssociativeArrayDispose(query);
	FskInstrumentedItemPrintfNormal(session, "open %s (%d)", session->url, err);
	return err;
}

static FskErr KprLibraryLoadThumbnail(char *path, FskBitmap *bitmap) {
	FskErr err = kFskErrNone;
	char *mime = NULL;
	unsigned char *data = NULL;
	FskInt64 size;

	bailIfError(FskFileLoad(path, &data, &size));

	bailIfError(FskImageDecompressSniffForMIME(data, (UInt32)size, NULL, path, &mime));
	bailIfError(FskImageDecompressDataWithOrientation(data, (UInt32)size, mime, NULL, 0, 0, NULL, NULL, bitmap));

bail:
	if (mime) FskMemPtrDispose(mime);
	if (data) FskMemPtrDispose(data);
	return err;
}

static FskErr KprLibraryGetThumbnail(char *kind, long id, Boolean micro, char* rotation, FskBitmap *bitmap)
{
	FskErr err;

	err = gAndroidCallbacks->libraryThumbnailCB(kind, id, micro, bitmap);
	if (err != kFskErrNone && FskStrCompare(kind, "album") == 0) {
		int index = 0;
		while (true) {
			char *path = KprLibraryGetAlbumSongPath(id, index++);
			if (path == NULL) {
				err = kFskErrNotFound;
				break;
			}
			
			err = KprLibraryFetchAlbumArtMetadata(path, micro, bitmap);
			if (err == kFskErrNone) break;
		}
	}
	if (*bitmap && rotation && FskStrCompare(rotation, "0") && (err == kFskErrNone)) {
		FskBitmap srcBits, dstBits = NULL;
		FskPort port = NULL;
		FskRectangleRecord bounds;
		float transform[3][3];
		UInt32 x, y, width, height;
		srcBits = *bitmap;
		FskBitmapGetBounds(srcBits, &bounds);
		x = bounds.width;
		y = bounds.height;
		switch (FskStrToNum(rotation)) {
		case 180: // down
			transform[0][0] = -1.f; 	transform[0][1] =  0.f;
			transform[1][0] =  0.f; 	transform[1][1] = -1.f;
			transform[2][0] = (float)x;	transform[2][1] = (float)y;
			width  = x;
			height = y;
			break;
		case 90: // left
			transform[0][0] =  0.f; 	transform[0][1] =  1.f;
			transform[1][0] = -1.f; 	transform[1][1] =  0.f;
			transform[2][0] = (float)y;	transform[2][1] =  0.f;
			width  = y;
			height = x;
			break;
		case 270: // right
			transform[0][0] =  0.f; 	transform[0][1] = -1.f;
			transform[1][0] =  1.f; 	transform[1][1] =  0.f;
			transform[2][0] =  0.f; 	transform[2][1] = (float)x;
			width  = y;
			height = x;
			break;
		default:
			goto bail;
		}
		transform[0][2] = 0.f;
		transform[1][2] = 0.f;
		transform[2][2] = 1.f;
		bailIfError(FskPortNew(&port, NULL, NULL));
		bailIfError(FskBitmapNew(width, height, kFskBitmapFormatDefaultNoAlpha, &dstBits));
		FskPortSetBitmap(port, dstBits);
		FskPortBeginDrawing(port, NULL);
		FskPortBitmapProject(port, srcBits, &bounds, transform);
		FskPortEndDrawing(port);
		FskPortSetBitmap(port, NULL);
		*bitmap = dstBits;
		FskBitmapDispose(srcBits);
		FskPortDispose(port);
	}
bail:
	return err;
}

static FskErr KprLibraryFetchAlbumArtMetadata(char *path, Boolean micro, FskBitmap *bitmap) {
	FskErr err;
	FskMediaPlayer mp = NULL;
	FskMediaPropertyValueRecord meta;
	char *sniff = NULL;

	err = KprLibrarySniffForMIME(NULL, path, &sniff);
	if (err != kFskErrNone) return err;

	err = FskMediaPlayerNew(&mp, path, 1, sniff, NULL);
	FskMemPtrDispose(sniff);
	
	if (err == kFskErrNone) {
		err = FskMediaPlayerGetMetadata(mp, "AlbumArt", 1, &meta, NULL);
		if (err == kFskErrUnknownElement) 
			err = FskMediaPlayerGetMetadata(mp, "WM/WMCollectionID", 1, &meta, NULL);
		if (err == kFskErrNone) {
			if (meta.type == kFskMediaPropertyTypeImage) {
				char* mime = (char*)meta.value.data.data;
				UInt32 length = FskStrLen(mime) + 1;
				char *data = mime + length;
				UInt32 size = meta.value.data.dataSize - length;
				SInt32 width, height;
				width = height = (micro ? 96 : 320);

				err = FskImageDecompressData(data, size, mime, NULL, width, height, NULL, NULL, bitmap);
			}
			FskMediaPropertyEmpty(&meta);
		}
		FskMediaPlayerDispose(mp);
	}
	return err;
}


#define fail(E) { err = E; goto bail; }

static char *KprURLCopyPath(KprURLParts parts)
{
	FskErr err;
	char *path = NULL;

	err = FskMemPtrNew(parts->pathLength + 1, &path);
	if (err != kFskErrNone) return NULL;

	FskMemCopy(path, parts->path, parts->pathLength);
	path[parts->pathLength] = 0;
	return path;
}

static FskErr KprLibrarySniffForMIME(FskHeaders *headers, char *path, char **outMime) {
	FskErr err;
	FskFileInfo info;
	FskFile fref = NULL;
	char *data = NULL;
	UInt32 dataSize;

	// if (path[0] == 0) return kFskErrFileNotFound;
	// if (path[1] == 0) return kFskErrIsDirectory;

	bailIfError(FskFileGetFileInfo(path, &info));
	if (info.filetype == kFskDirectoryItemIsDirectory) fail(kFskErrIsDirectory);

	bailIfError(FskFileOpen(path, kFskFilePermissionReadOnly, &fref));

	dataSize = 4096L;
	bailIfError(FskMemPtrNew(dataSize, &data));

	FskFileRead(fref, dataSize, data, &dataSize);

	if (FskMediaPlayerSniffForMIME(data, dataSize, headers, path, outMime) != kFskErrNone) {
		bailIfError(FskImageDecompressSniffForMIME(data, dataSize, headers, path, outMime));
	}

bail:
	if (fref) FskFileClose(fref);
	if (data) FskMemPtrDispose(data);
	return err;
}

static char *KprLibraryGetAlbumSongPath(long album_id, int index) {
	return gAndroidCallbacks->libraryGetSongPathCB(album_id, index);
}

void Library_sniffMIME(xsMachine *the)
{
	xsIntegerValue kind = xsToInteger(xsArg(0));
	if (kprLibraryPodcastKind == kind) {
		KPRLibrarySniffPodcast(the);
	}
	else {
		(void)xsCallFunction1(xsArg(2), xsGlobal, xsUndefined);
	}
}
