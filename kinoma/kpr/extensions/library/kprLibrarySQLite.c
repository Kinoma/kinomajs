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
#include "kprSQLite.h"
#include "kprURL.h"
#include "kprUtilities.h"
#include "kprLibraryServer.h"

#if !TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
typedef struct KprExtensionStruct KprExtensionRecord, *KprExtension;

struct KprLibraryStruct {
	KprDB db;
	KprExtension extensions;
	SInt32 fileID;
	xsMachine* the;
	KprMessage message;
	KprLibraryServer server;
};

struct KprExtensionStruct {
	KprExtension next;
	KprExtension first;
	char c;
};

typedef struct KprLibraryFileStruct {
	char* path;
	double date;
	FskInt64 size;
	char* title;
	SInt32 id;
} KprLibraryFileRecord, *KprLibraryFile;

typedef struct KprLibraryFileInfoStruct {
	SInt32 id;
	double date;
	FskInt64 size;
} KprLibraryFileInfoRecord, *KprLibraryFileInfo;

typedef struct KprLibraryProgressStruct {
	SInt32 count;
	SInt32 index;
} KprLibraryProgressRecord, *KprLibraryProgress;

typedef struct KprLibraryImageStruct {
	char* mime;
	double taken;
	SInt32 rotation;
	SInt32 width;
	SInt32 height;
	SInt32 area;
	SInt32 id;
} KprLibraryImageRecord;

typedef struct KprLibraryMediaAudioStruct {
	char* mime;
	double duration;
	SInt32 artist_id;
	SInt32 album_id;
	double taken;
	SInt32 track;
	SInt32 genre_id;
	SInt32 id;
} KprLibraryAudioRecord;

typedef struct KprLibraryMediaVideoStruct {
	char* mime;
	double duration;
	SInt32 width;
	SInt32 height;
	SInt32 area;
	SInt32 id;
} KprLibraryVideoRecord;

typedef struct KprLibraryThumbnailStruct {
	void* jpeg;
	SInt32 size;
	SInt32 width;
	SInt32 height;
	SInt32 id;
} KprLibraryThumbnailRecord;

typedef struct KprLibraryAlbumViewStruct {
	SInt32 id;
	char* album_name;
	char* artist_name;
	double taken;
	SInt32 song_count;
} KprLibraryAlbumViewRecord;

typedef struct KprLibraryArtistViewStruct {
	SInt32 id;
	char* artist_name;
	char* album_name;
	SInt32 album_count;
	SInt32 song_count;
} KprLibraryArtistViewRecord;

typedef struct KprLibraryGenreViewStruct {
	SInt32 id;
	char* genre_name;
	SInt32 song_count;
} KprLibraryGenreViewRecord;

typedef struct KprLibraryAudioViewStruct {
	SInt32 id;
	double date;
	char* mime;
	FskInt64 size;
	char* url;
	char* title;
	double duration;
	double time;
	char* artist_name;
	char* album_name;
	SInt32 track;
	char* genre_name;
} KprLibraryAudioViewRecord;

typedef struct KprLibraryImageViewStruct {
	SInt32 id;
	double date;
	char* mime;
	FskInt64 size;
	char* url;
	char* title;
	SInt32 width;
	SInt32 height;
	SInt32 rotation;
	double taken;
} KprLibraryImageViewRecord;

typedef struct KprLibraryVideoViewStruct {
	SInt32 id;
	double date;
	char* mime;
	FskInt64 size;
	char* url;
	char* title;
	SInt32 width;
	SInt32 height;
	double duration;
	double time;
} KprLibraryVideoViewRecord;

typedef struct KprLibraryContentStruct {
	char* mime;
	char* path;
} KprLibraryContentRecord;

static FskErr KprLibraryNew();
static void KprLibraryDispose();
static void KprLibraryCancel(KprService service, KprMessage message);
extern void KprLibraryInvoke(KprService service, KprMessage message);
extern void KprLibraryStart(KprService service, FskThread thread, xsMachine* the);
extern void KprLibraryStop(KprService service);

static Boolean KprLibraryIDProc(void* target, const char** keys UNUSED, const void* values);
static Boolean KprLibraryAlbumProc(void* target, const char** keys UNUSED, const void* values);
static Boolean KprLibraryArtistProc(void* target, const char** keys UNUSED, const void* values);
static Boolean KprLibraryAudioProc(void* target, const char** keys UNUSED, const void* values);
static Boolean KprLibraryGenreProc(void* target, const char** keys UNUSED, const void* values);
static Boolean KprLibraryImageProc(void* target, const char** keys UNUSED, const void* values);
static Boolean KprLibraryThumbnailProc(void* it, const char** keys UNUSED, const void* values);
static Boolean KprLibraryVideoProc(void* target, const char** keys UNUSED, const void* values);

static void KprScan(Boolean clear);
static FskErr KprScanDirectory(char* path, UInt32 pathLength);
static FskErr KprScanFile(char* path, char* title);
static Boolean KprScanFileGetInfo(void* target, const char** keys UNUSED, const void* values);
static Boolean KprScanFileProc(void* target, const char** keys UNUSED, const void* values);
static FskErr KprScanError(KprLibraryProgress progress, KprLibraryFile file);
static FskErr KprScanImage(KprLibraryProgress progress, KprLibraryFile file, char* mime);
static FskErr KprScanImageSpoolerClose(FskMediaSpooler spooler);
static FskErr KprScanImageSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions);
static FskErr KprScanImageSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static double KprScanImageTaken(char* string);
static FskErr KprScanMedia(KprLibraryProgress progress, KprLibraryFile file, char* mime);
static FskErr KprScanMediaAudio(KprLibraryProgress progress, KprLibraryFile file, char* mime, FskMediaPlayer mp);
static char* KprScanMediaAudioGenre(char* genre);
static double KprScanMediaAudioTaken(char* string);
static FskErr KprScanMediaVideo(KprLibraryProgress progress, KprLibraryFile file, char* mime, FskMediaPlayer mp);
static FskErr KprScanThumbnail(KprLibraryProgress progress, KprLibraryFile file, FskBitmap bitmap);
static FskErr KprScanThumbnailScale(FskBitmap* bitmap, SInt32 width, SInt32 height);
//static FskErr KprScanThumbnailCompress(FskBitmap bitmap, double quality, void **data, UInt32 *dataSize);

static FskErr KprLibrarySaveImage(KprLibrary library, UInt8 *data, UInt32 size);

static Boolean KprLibraryServerThumbnailProc(void* it, const char** keys UNUSED, const void* values);

static FskErr KprExtensionNew(KprExtension *it, char c, KprExtension next);
static void KprExtensionDispose(KprExtension self);
static Boolean KprExtensionExists(KprExtension self, char* s);
static void KprExtensionInsert(KprExtension self, char* s);
static Boolean KprExtensionProc(void* target, const char** keys UNUSED, const void* values);

enum {
	kprLibraryBegin,
	kprLibraryCommit,
	kprLibraryRollback,
	kprLibraryFileID,
	kprLibraryExtensionGet,
	
	kprLibraryFileClear,
	kprLibraryFileCollect,
	kprLibraryCreationClear,
	kprLibraryDeletionClear,
	kprLibraryModificationClear,
	kprLibraryDeletionCopy,
	
	kprLibraryFileGetInfo,
	kprLibraryDeletionRemove,
	kprLibraryFileUpdate,
	kprLibraryModificationAdd,
	kprLibraryFileInsert,
	kprLibraryCreationAdd,
	
	kprLibraryFileDelete,
	kprLibraryImageDelete,
	kprLibraryAudioDelete,
	kprLibraryVideoDelete,
	kprLibraryThumbnailDelete,
	
	kprLibraryFileCount,
	kprLibraryModificationCount,
	kprLibraryCreationCount,
	kprLibraryFileModify,
	kprLibraryFileCreate,
	
	kprLibraryImageInsert,
	
	kprLibraryFileSetTitle,
	kprLibraryArtistInsert,
	kprLibraryArtistGetID,
	kprLibraryAlbumInsert,
	kprLibraryAlbumGetID,
	kprLibraryGenreInsert,
	kprLibraryGenreGetID,
	kprLibraryAudioInsert,
	
	kprLibraryVideoInsert,
	
	kprLibraryThumbnailInsert,
	
	kprLibraryArtistDelete,
	kprLibraryAlbumDelete,
	kprLibraryGenreDelete,
	
	kprLibraryThumbnailGet,

	kprLibraryImageViewGetByDate,
	kprLibraryImageViewGetByDimensions,
	kprLibraryImageViewGetBySize,
	kprLibraryImageViewGetByTitle,

	kprLibraryVideoViewGetByDate,
	kprLibraryVideoViewGetByDimensions,
	kprLibraryVideoViewGetByDuration,
	kprLibraryVideoViewGetBySize,
	kprLibraryVideoViewGetByTitle,
	
	kprLibraryAudioViewGetAlbums,
	kprLibraryAudioViewGetArtistAlbums,
	kprLibraryAudioViewGetArtists,
	kprLibraryAudioViewGetGenres,
	kprLibraryAudioViewGetSongs,
	kprLibraryAudioViewGetAlbumSongs,
	kprLibraryAudioViewGetArtistSongs,
	kprLibraryAudioViewGetGenreSongs,
	
	kprLibraryFileGetByID
};

static KprDBQueryRecord gKprLibraryDBQuery[] = {
	{ "BEGIN IMMEDIATE", NULL, NULL, NULL},
	{ "COMMIT", NULL, NULL, NULL},
	{ "ROLLBACK", NULL, NULL, NULL},
	{ "SELECT max(id) FROM file", NULL, "i", NULL},
	{ "SELECT extension_name FROM extension", NULL, "t", NULL },
	
	{ "DELETE FROM file ", NULL, NULL, NULL },
	{ "DELETE FROM file WHERE NOT EXISTS (SELECT id FROM image WHERE id = file.id UNION SELECT id FROM audio WHERE id = file.id UNION SELECT id FROM video WHERE id = file.id)", NULL, NULL, NULL },
	{ "DELETE FROM creation", NULL, NULL, NULL },
	{ "DELETE FROM deletion", NULL, NULL, NULL },
	{ "DELETE FROM modification", NULL, NULL, NULL },
	{ "INSERT INTO deletion ( id ) SELECT id FROM file", NULL, NULL, NULL },
	
	{ "SELECT id, date, size FROM file WHERE path == ?", "t", "rdI", NULL },
	{ "DELETE FROM deletion WHERE id = ?", "r", NULL, NULL },
	{ "UPDATE file SET path = ?, date = ?, size = ?, title = ? WHERE id = ?", "tdItr", NULL, NULL },
	{ "INSERT INTO modification ( id ) VALUES ( ? )", "r", NULL, NULL },
	{ "INSERT INTO file ( path, date, size, title, id ) VALUES ( ?, ?, ?, ?, ? )", "tdItr", NULL, NULL },
	{ "INSERT INTO creation ( id ) VALUES ( ? )", "r", NULL, NULL },

	{ "DELETE FROM file WHERE EXISTS (SELECT id FROM deletion WHERE id = file.id)", NULL, NULL, NULL },
	{ "DELETE FROM image WHERE EXISTS (SELECT id FROM deletion WHERE id = image.id UNION SELECT id FROM modification WHERE id = image.id)", NULL, NULL, NULL },
	{ "DELETE FROM audio WHERE EXISTS (SELECT id FROM deletion WHERE id = audio.id UNION SELECT id FROM modification WHERE id = audio.id)", NULL, NULL, NULL },
	{ "DELETE FROM video WHERE EXISTS (SELECT id FROM deletion WHERE id = video.id UNION SELECT id FROM modification WHERE id = video.id)", NULL, NULL, NULL },
	{ "DELETE FROM thumbnail WHERE EXISTS (SELECT id FROM deletion WHERE id = thumbnail.id UNION SELECT id FROM modification WHERE id = thumbnail.id)", NULL, NULL, NULL },

	{ "SELECT count(*) FROM file", NULL, "i", NULL },
	{ "SELECT count(*) FROM modification", NULL, "i", NULL },
	{ "SELECT count(*) FROM creation", NULL, "i", NULL },
	{ "SELECT path, date, size, title, id FROM modification NATURAL JOIN file", NULL, "tdItr", NULL },
	{ "SELECT path, date, size, title, id FROM creation NATURAL JOIN file", NULL, "tdItr", NULL },
	
	{ "INSERT INTO image ( mime, taken, rotation, width, height, area, id ) VALUES ( ?, ?, ?, ?, ?, ?, ? )", "tdiiiir", NULL, NULL},
	
	{ "UPDATE file SET title = ? WHERE id = ?", "tr", NULL, NULL},
	{ "INSERT OR IGNORE INTO artist ( artist_name ) VALUES ( ? )", "t", NULL, NULL},
	{ "SELECT artist_id FROM artist WHERE ( artist_name = ? )", "t", "r", NULL},
	{ "INSERT OR IGNORE INTO album ( album_name, artist_id ) VALUES ( ?, ? )", "tr", NULL, NULL},
	{ "SELECT album_id FROM album WHERE ( album_name = ? ) AND ( artist_id = ?)", "tr", "r", NULL},
	{ "INSERT OR IGNORE INTO genre ( genre_name ) VALUES ( ? )", "t", NULL, NULL},
	{ "SELECT genre_id FROM genre WHERE ( genre_name = ? )", "t", "r", NULL},
	{ "INSERT INTO audio ( mime, duration, artist_id, album_id, taken, track, genre_id, id ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ? )", "tdrrdirr", NULL, NULL},
	
	{ "INSERT INTO video ( mime, duration, width, height, area, id ) VALUES ( ?, ?, ?, ?, ?, ? )", "tdiiir", NULL, NULL},
	
	{ "INSERT INTO thumbnail ( thumbnail_jpeg, thumbnail_width, thumbnail_height, id ) VALUES ( ?, ?, ?, ? )", "biir", NULL, NULL},
	
	{ "DELETE FROM artist WHERE NOT EXISTS (SELECT artist_id FROM audio WHERE artist_id = artist.artist_id)", NULL, NULL, NULL },
	{ "DELETE FROM album WHERE NOT EXISTS (SELECT album_id FROM audio WHERE album_id = album.album_id)", NULL, NULL, NULL },
	{ "DELETE FROM genre WHERE NOT EXISTS (SELECT genre_id FROM audio WHERE genre_id = genre.genre_id)", NULL, NULL, NULL },
	
	{ "SELECT thumbnail_jpeg, thumbnail_width, thumbnail_height FROM thumbnail WHERE id = ?", "r", "bii", NULL},
	
	{ "SELECT id, date, mime, size, path, title, width, height, rotation, taken FROM image_view ORDER by taken DESC", NULL, "rdtIttiiid", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, rotation, taken FROM image_view ORDER by area, title", NULL, "rdtIttiiid", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, rotation, taken FROM image_view ORDER by size, title", NULL, "rdtIttiiid", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, rotation, taken FROM image_view ORDER by title", NULL, "rdtIttiiid", NULL},
			
	{ "SELECT id, date, mime, size, path, title, width, height, duration, time FROM video_view ORDER by date DESC", NULL, "rdtIttiidd", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, duration, time FROM video_view ORDER by area, title", NULL, "rdtIttiidd", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, duration, time FROM video_view ORDER by duration, title", NULL, "rdtIttiidd", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, duration, time FROM video_view ORDER by size, title", NULL, "rdtIttiidd", NULL},
	{ "SELECT id, date, mime, size, path, title, width, height, duration, time FROM video_view ORDER by title", NULL, "rdtIttiidd", NULL},

    { "SELECT id, album_name, artist_name, taken, COUNT(*) FROM audio_view GROUP BY album_name ORDER BY album_name", NULL, "rttdi", NULL},
    { "SELECT id, album_name, artist_name, taken, COUNT(*) FROM audio_view WHERE artist_name = ? GROUP BY album_name ORDER BY album_name", "t", "rttdi", NULL},
    { "SELECT id, artist_name, album_name, COUNT(DISTINCT album_name), COUNT(*) FROM audio_view GROUP BY artist_name ORDER BY artist_name", NULL, "rttii", NULL},
    { "SELECT id, genre_name, COUNT(*) FROM audio_view GROUP BY genre_name ORDER BY genre_name ", NULL, "rti", NULL},
	{ "SELECT id, date, mime, size, path, title, duration, time, artist_name, album_name, track, genre_name FROM audio_view ORDER by title", NULL, "rdtIttddttit", NULL},
	{ "SELECT id, date, mime, size, path, title, duration, time, artist_name, album_name, track, genre_name FROM audio_view WHERE album_name = ? ORDER by track", "t", "rdtIttddttit", NULL},
	{ "SELECT id, date, mime, size, path, title, duration, time, artist_name, album_name, track, genre_name FROM audio_view WHERE artist_name = ? ORDER by album_name,track", "t", "rdtIttddttit", NULL},
	{ "SELECT id, date, mime, size, path, title, duration, time, artist_name, album_name, track, genre_name FROM audio_view WHERE genre_name = ? ORDER by title", "t", "rdtIttddttit", NULL},
    
	{ "SELECT mime, path FROM file NATURAL JOIN audio WHERE id=? UNION SELECT mime, path FROM file NATURAL JOIN image WHERE id=? UNION SELECT mime, path FROM file NATURAL JOIN video WHERE id=?", "rrr", "tt", NULL },

	{NULL, NULL, NULL, NULL}
};

static SInt32 gScanThumbnailSize = 320; // @@ scale

#include "kprLibrary.sql.c"

//--------------------------------------------------
// Scanner
//--------------------------------------------------

static KprLibraryRecord gLibrary = { NULL, NULL, 0, NULL, NULL, NULL };
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
	char* url = NULL;
	char* path = NULL;
	Boolean exists;
	FskFileInfo info;
	KprDBQuery query;
	KprLibrary self = &gLibrary;
	
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &url));
	bailIfError(KprURLMerge(url, "kprLibrary2.sqlite", &path));
	exists = kFskErrNone == FskFileGetFileInfo(path, &info);
	bailIfError(KprDBNew(&self->db, path, true));
	if (!exists)
		bailIfError(KprDBExecuteBuffer(self->db, kprLibrarySQLInstructions));
	
	for (query = gKprLibraryDBQuery; query->sql; query++) {
		bailIfError(KprDBStatementNew(&query->statement, self->db, query->sql, query->bind, query->row));
	}
	
	bailIfError(KprExtensionNew(&self->extensions, 0, NULL));
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryExtensionGet].statement, NULL, KprExtensionProc, self->extensions));
	
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileID].statement, NULL, KprLibraryIDProc, &self->fileID));

	self->the = xsAliasMachine(&allocation, gShell->root, "library", self);
	bailIfNULL(self->the);
	
	gLibraryService.machine = self->the;
	gLibraryService.thread = gLibraryThread;

	bailIfError(KprLibraryServerNew(&self->server));
	
bail:
	FskMemPtrDispose(path);
	FskMemPtrDispose(url);
	return err;
}

void KprLibraryDispose()
{
	KprLibrary self = &gLibrary;
	KprDBQuery query;
	
	KprLibraryServerDispose(self->server);

	for (query = gKprLibraryDBQuery; query->sql; query++) {
		KprDBStatementDispose(query->statement);
		query->statement = NULL;
	}
	KprDBDispose(self->db);
	self->db = NULL;
	
	KprExtensionDispose(self->extensions);

	if (self->the)
		xsDeleteMachine(self->the);
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
		if (FskStrCompareWithLength("scan", message->parts.name, message->parts.nameLength) == 0) {
			KprScan(FskAssociativeArrayElementGetString(query, "clear") != NULL);
		}
		else if (FskStrCompareWithLength("/server/", message->parts.path, 8) == 0) {
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
				xsVars(2);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						char* value = FskAssociativeArrayElementGetString(query, "sort");
						xsVar(0) = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompare("date", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoViewGetByDate].statement, NULL, KprLibraryVideoProc, the));
						else if (FskStrCompare("dimensions", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoViewGetByDimensions].statement, NULL, KprLibraryVideoProc, the));
						else if (FskStrCompare("duration", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoViewGetByDuration].statement, NULL, KprLibraryVideoProc, the));
						else if (FskStrCompare("size", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoViewGetBySize].statement, NULL, KprLibraryVideoProc, the));
						else
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoViewGetByTitle].statement, NULL, KprLibraryVideoProc, the));
						target->result = xsMarshall(xsVar(0));
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
				xsVars(2);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						xsVar(0) = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompareWithLength("albums", message->parts.name, message->parts.nameLength) == 0) {
							char* value;
							if ((value = FskAssociativeArrayElementGetString(query, "artist")))
								xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetArtistAlbums].statement, &value, KprLibraryAlbumProc, the));
							else
								xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetAlbums].statement, NULL, KprLibraryAlbumProc, the));
						}
						else if (FskStrCompareWithLength("artists", message->parts.name, message->parts.nameLength) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetArtists].statement, NULL, KprLibraryArtistProc, the));
						else if (FskStrCompareWithLength("genres", message->parts.name, message->parts.nameLength) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetGenres].statement, NULL, KprLibraryGenreProc, the));
						else {
							char* value;
							if ((value = FskAssociativeArrayElementGetString(query, "album")))
								xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetAlbumSongs].statement, &value, KprLibraryAudioProc, the));
							else if ((value = FskAssociativeArrayElementGetString(query, "artist")))
								xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetArtistSongs].statement, &value, KprLibraryAudioProc, the));
							else if ((value = FskAssociativeArrayElementGetString(query, "genre")))
								xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetGenreSongs].statement, &value, KprLibraryAudioProc, the));
							else
								xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioViewGetSongs].statement, NULL, KprLibraryAudioProc, the));
						}
						target->result = xsMarshall(xsVar(0));
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
				xsVars(2);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						char* value = FskAssociativeArrayElementGetString(query, "sort");
						xsVar(0) = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompare("date", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryImageViewGetByDate].statement, NULL, KprLibraryImageProc, the));
						else if (FskStrCompare("dimensions", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryImageViewGetByDimensions].statement, NULL, KprLibraryImageProc, the));
						else if (FskStrCompare("size", value) == 0)
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryImageViewGetBySize].statement, NULL, KprLibraryImageProc, the));
						else
							xsThrowIfFskErr(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryImageViewGetByTitle].statement, NULL, KprLibraryImageProc, the));
						target->result = xsMarshall(xsVar(0));
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
				char* value = FskAssociativeArrayElementGetString(query, "id");
				SInt32 id = FskStrToNum(value);
				bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryThumbnailGet].statement, &id, KprLibraryThumbnailProc, message));
			}
		}
	bail:
		gLibrary.message = NULL;
		FskAssociativeArrayDispose(query);
	}
	if (kFskErrNeedMoreTime != err) {
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		message->error = err;
		KprMessageComplete(message);
	}
}

xsMachine* KprLibraryMachine()
{
	return gLibrary.the;
}

Boolean KprLibraryIDProc(void* target, const char** keys UNUSED, const void* values)
{
	*((SInt32*)target) = *((SInt32*)values);
	return true;
}

void KprLibraryInitial(xsMachine* the, char* string)
{
	if ((65 <= *string) && (*string <= 90)) {
		char *p = string + 1;
		char c = *p;
		*p = 0;
		xsNewHostProperty(xsVar(1), xsID("initial"), xsString(string), xsDefault, xsDontScript);
		*p = c;
	}
	else {
		xsNewHostProperty(xsVar(1), xsID("initial"), xsString("#"), xsDefault, xsDontScript);
	}
}

Boolean KprLibraryAlbumProc(void* target, const char** keys UNUSED, const void* values)
{
	xsMachine* the = target;
	KprLibraryAlbumViewRecord *albumView = (KprLibraryAlbumViewRecord *)values;
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(1), xsID("album"), xsString(albumView->album_name), xsDefault, xsDontScript);
	KprLibraryInitial(the, albumView->album_name);
	xsNewHostProperty(xsVar(1), xsID("artist"), xsString(albumView->artist_name), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("taken"), xsNumber(albumView->taken), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger(albumView->song_count), xsDefault, xsDontScript);
	
	xsResult = xsCall1(xsString("xkpr://library/thumbnail?id="), xsID("concat"), xsInteger(albumView->id));
	xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
	xsResult = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString(albumView->album_name));
	xsResult = xsCall1(xsString("xkpr://library/music/songs?album="), xsID("concat"), xsResult);
	xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
	
	xsCall1(xsVar(0), xsID("push"), xsVar(1));
	return true;
}

Boolean KprLibraryArtistProc(void* target, const char** keys UNUSED, const void* values)
{
	xsMachine* the = target;
	KprLibraryArtistViewRecord *artistView = (KprLibraryArtistViewRecord *)values;
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(1), xsID("albums"), xsInteger(artistView->album_count), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("artist"), xsString(artistView->artist_name), xsDefault, xsDontScript);
	KprLibraryInitial(the, artistView->artist_name);
	xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger(artistView->song_count), xsDefault, xsDontScript);
	
	xsResult = xsCall1(xsString("xkpr://library/thumbnail?id="), xsID("concat"), xsInteger(artistView->id));
	xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
	if (artistView->album_count == 1) {
		xsNewHostProperty(xsVar(1), xsID("album"), xsString(artistView->album_name), xsDefault, xsDontScript);
		xsResult = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString(artistView->album_name));
		xsResult = xsCall1(xsString("xkpr://library/music/songs?album="), xsID("concat"), xsResult);
	}
	else {
		xsResult = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString(artistView->artist_name));
		xsResult = xsCall1(xsString("xkpr://library/music/albums?artist="), xsID("concat"), xsResult);
	}
	xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
	xsCall1(xsVar(0), xsID("push"), xsVar(1));
	return true;
}

Boolean KprLibraryGenreProc(void* target, const char** keys UNUSED, const void* values)
{
	xsMachine* the = target;
	KprLibraryGenreViewRecord *genreView = (KprLibraryGenreViewRecord *)values;
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(1), xsID("genre"), xsString(genreView->genre_name), xsDefault, xsDontScript);
	KprLibraryInitial(the, genreView->genre_name);
	xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger(genreView->song_count), xsDefault, xsDontScript);
	
	xsResult = xsCall1(xsString("xkpr://library/thumbnail?id="), xsID("concat"), xsInteger(genreView->id));
	xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);

	xsResult = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString(genreView->genre_name));
	xsResult = xsCall1(xsString("xkpr://library/music/songs?genre="), xsID("concat"), xsResult);
	xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
	
	xsCall1(xsVar(0), xsID("push"), xsVar(1));
	return true;
}

Boolean KprLibraryAudioProc(void* target, const char** keys UNUSED, const void* values)
{
	xsMachine* the = target;
	KprLibraryAudioViewRecord *audioView = (KprLibraryAudioViewRecord *)values;
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(1), xsID("album"), xsString(audioView->album_name), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("artist"), xsString(audioView->artist_name), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("date"), xsNumber(audioView->date), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("duration"), xsNumber(audioView->duration), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("genre"), xsString(audioView->genre_name), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("size"), xsNumber(audioView->size), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("title"), xsString(audioView->title), xsDefault, xsDontScript);
	KprLibraryInitial(the, audioView->title);
	xsNewHostProperty(xsVar(1), xsID("track"), xsInteger(audioView->track), xsDefault, xsDontScript);
	
	xsNewHostProperty(xsVar(1), xsID("kind"), xsInteger(kprLibraryFileKind), xsDefault, xsDontScript);
	xsResult = xsCall1(xsString("xkpr://library/thumbnail?id="), xsID("concat"), xsInteger(audioView->id));
	xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
	xsNewHostProperty(xsVar(1), xsID("mime"), xsString(audioView->mime), xsDefault, xsDontScript);
	xsResult = xsCall1(xsString("file://"), xsID("concat"), xsString(audioView->url));
	xsNewHostProperty(xsVar(1), xsID("url"), xsResult, xsDefault, xsDontScript);

	xsCall1(xsVar(0), xsID("push"), xsVar(1));
	return true;
}

Boolean KprLibraryImageProc(void* target, const char** keys UNUSED, const void* values)
{
	xsMachine* the = target;
	KprLibraryImageViewRecord *imageView = (KprLibraryImageViewRecord *)values;
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(1), xsID("date"), xsNumber(imageView->taken), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("size"), xsNumber(imageView->size), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("title"), xsString(imageView->title), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("width"), xsInteger(imageView->width), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("height"), xsInteger(imageView->height), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("rotation"), xsInteger(imageView->rotation), xsDefault, xsDontScript);
	
	xsNewHostProperty(xsVar(1), xsID("kind"), xsInteger(kprLibraryFileKind), xsDefault, xsDontScript);
	xsResult = xsCall1(xsString("xkpr://library/thumbnail?id="), xsID("concat"), xsInteger(imageView->id));
	xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
	xsNewHostProperty(xsVar(1), xsID("mime"), xsString(imageView->mime), xsDefault, xsDontScript);
	xsResult = xsCall1(xsString("file://"), xsID("concat"), xsString(imageView->url));
	xsNewHostProperty(xsVar(1), xsID("url"), xsResult, xsDefault, xsDontScript);
	
	xsCall1(xsVar(0), xsID("push"), xsVar(1));
	return true;
}

Boolean KprLibraryThumbnailProc(void* it, const char** keys UNUSED, const void* values)
{
	KprMessage message = it;
	KprLibraryThumbnailRecord *thumbnail = (KprLibraryThumbnailRecord *)values;
	KprImageTarget self = (KprImageTarget)message->stream;
    if (!self) return true;
	FskImageDecompressData(thumbnail->jpeg, thumbnail->size, "image/jpeg", NULL, 0, 0, NULL, NULL, (FskBitmap*)&self->bitmap);
	//fprintf(stderr, "%dx%d -> %dx%d %s\n", self->bitmap->bounds.width, self->bitmap->bounds.height, self->width, self->height, message->url);
	if (self->width && self->height)
		KprImageTargetScale(&self->bitmap, self->aspect, self->width, self->height);
	return true;
}

Boolean KprLibraryVideoProc(void* target, const char** keys UNUSED, const void* values)
{
	xsMachine* the = target;
	KprLibraryVideoViewRecord *videoView = (KprLibraryVideoViewRecord *)values;
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(1), xsID("date"), xsNumber(videoView->date), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("duration"), xsNumber(videoView->duration), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("size"), xsNumber(videoView->size), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("title"), xsString(videoView->title), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("width"), xsInteger(videoView->width), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(1), xsID("height"), xsInteger(videoView->height), xsDefault, xsDontScript);
	
	xsNewHostProperty(xsVar(1), xsID("kind"), xsInteger(kprLibraryFileKind), xsDefault, xsDontScript);
	xsResult = xsCall1(xsString("xkpr://library/thumbnail?id="), xsID("concat"), xsInteger(videoView->id));
	xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
	xsNewHostProperty(xsVar(1), xsID("mime"), xsString(videoView->mime), xsDefault, xsDontScript);
	xsResult = xsCall1(xsString("file://"), xsID("concat"), xsString(videoView->url));
	xsNewHostProperty(xsVar(1), xsID("url"), xsResult, xsDefault, xsDontScript);
	
	xsCall1(xsVar(0), xsID("push"), xsVar(1));
	return true;
}

FskErr KprLibrarySaveImage(KprLibrary library, UInt8 *data, UInt32 size)
{
	FskErr err = kFskErrNone;
	FskFile file = NULL;
	char *path = NULL;
	char *photoDirectory = NULL;
	char dateString[20];

	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypePhoto, true, NULL, &photoDirectory));

	FskTimeRecord tr;
	FskTimeElementsRecord ter;
	FskTimeGetOSTime(&tr);
	bailIfError(FskTimeLocaltime(&tr, &ter));
	FskTimeStrftime(dateString, 19, "IMG_%Y%m%d_%H%M%S", &ter, NULL);	
	bailIfError(FskMemPtrNewClear(FskStrLen(photoDirectory) + 1 + 19 + 5, &path));
	FskStrCopy(path, photoDirectory);
	FskStrCat(path, "/");
	FskStrCat(path, dateString);
	FskStrCat(path, ".jpg"); //Assume it's all jpg now

	err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	if (kFskErrFileNotFound == err) {
		bailIfError(FskFileCreate(path));
		err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	}
	else {
		FskInt64 zero = 0;
		err = FskFileSetSize(file, &zero);
	}
	bailIfError(err);
	bailIfError(FskFileWrite(file, size, data, NULL));
	FskFileClose(file);
	file = NULL;

	KprLibraryProgressRecord progress;
	char* title = dateString;
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryBegin].statement, NULL, NULL, NULL));
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCreationClear].statement, NULL, NULL, NULL));
	KprScanFile(path, title);
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileCreate].statement, NULL, KprScanFileProc, &progress));
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCommit].statement, NULL, NULL, NULL));

bail:
	if (file)
		FskFileClose(file);

	if (photoDirectory)
		FskMemPtrDispose(photoDirectory);
	
	if (path)
		FskMemPtrDispose(path);

	return err;
}

void KprScan(Boolean clear)
{
	char* path;
	char buffer[4096];
	SInt32 modificationCount, creationCount;
	KprLibraryProgressRecord progress;
	double start, stop;
	KprMessage message;
	
	path = FskEnvironmentGet("libraryPath");
	if (path)
		FskStrCopy(buffer, path);
	else {
		FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeDocument, true, NULL, &path);
		FskStrCopy(buffer, path);
		FskStrCat(buffer, "Kinoma Media/");
	}
	
	start = KprShellTicks(gShell);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryBegin].statement, NULL, NULL, NULL);
	if (clear)
		KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileClear].statement, NULL, NULL, NULL);
	else
		KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileCollect].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCreationClear].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryDeletionClear].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryModificationClear].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryDeletionCopy].statement, NULL, NULL, NULL);
	stop = KprShellTicks(gShell);
	fprintf(stderr, "scan before %f\n", stop - start);

	//KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryBegin].statement, NULL, NULL, NULL);
	start = KprShellTicks(gShell);
	KprScanDirectory(buffer, FskStrLen(buffer));
	stop = KprShellTicks(gShell);
	fprintf(stderr, "scan file %f\n", stop - start);
	//KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCommit].statement, NULL, NULL, NULL);

	start = KprShellTicks(gShell);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileDelete].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryImageDelete].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioDelete].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoDelete].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryThumbnailDelete].statement, NULL, NULL, NULL);
	stop = KprShellTicks(gShell);
	fprintf(stderr, "scan after %f\n", stop - start);
	
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileCount].statement, NULL, KprLibraryIDProc, &progress.count);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryModificationCount].statement, NULL, KprLibraryIDProc, &modificationCount);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCreationCount].statement, NULL, KprLibraryIDProc, &creationCount);
	progress.index = progress.count - modificationCount - creationCount;
	
	sprintf(buffer, "xkpr://library/scanning?c=%ld&i=%ld", progress.count, progress.index);
	KprMessageNew(&message, buffer);
	if (message) KprMessageNotify(message);

	//KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryBegin].statement, NULL, NULL, NULL);
	start = KprShellTicks(gShell);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileModify].statement, NULL, KprScanFileProc, &progress);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileCreate].statement, NULL, KprScanFileProc, &progress);
	stop = KprShellTicks(gShell);
	//KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCommit].statement, NULL, NULL, NULL);
	
	sprintf(buffer, "xkpr://library/scanned?c=%ld&i=%ld", progress.count, progress.index);
	KprMessageNew(&message, buffer);
	if (message) KprMessageNotify(message);
	
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryArtistDelete].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAlbumDelete].statement, NULL, NULL, NULL);
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryGenreDelete].statement, NULL, NULL, NULL);
	
	KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCommit].statement, NULL, NULL, NULL);
	fprintf(stderr, "scan metadata %f\n", stop - start);
	
}

FskErr KprScanDirectory(char* path, UInt32 pathLength)
{
	FskErr err = kFskErrNone;
	FskDirectoryIterator iterator = NULL;
	char* name = NULL;
	UInt32 itemType, nameLength, length;
	char* dot;
	
	bailIfError(FskDirectoryIteratorNew(path, &iterator, 0));
	while (kFskErrNone == FskDirectoryIteratorGetNext(iterator, &name, &itemType)) {
		if (name[0] == '.')
			continue;
		nameLength = FskStrLen(name);
		length = pathLength + nameLength + 1;
		FskMemCopy(path + pathLength, name, nameLength);
		switch (itemType) {
		case kFskDirectoryItemIsDirectory:
			path[length - 1] = '/';
			path[length] = 0;
			KprScanDirectory(path, length);
			break;
		case kFskDirectoryItemIsFile:
			path[length - 1] = 0;
			dot = FskStrRChr(name, '.');
			if (dot) {
				if (KprExtensionExists(gLibrary.extensions, dot + 1)) {
					*dot = 0;
					KprScanFile(path, name);
				}
			}
			break;
		default:
			// @@ link
			break;
		}
		FskMemPtrDispose(name);
		name = NULL;
	}
bail:
	FskMemPtrDispose(name);
	FskDirectoryIteratorDispose(iterator);
	return err;
}

FskErr KprScanFile(char* path, char* title)
{
	FskErr err = kFskErrNone;
	KprLibraryFileRecord file;
	KprLibraryFileInfoRecord oldInfo;
	FskFileInfo newInfo;
	
	file.path = path;
	oldInfo.id = 0;
	oldInfo.date = 0;
	oldInfo.size = 0;
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileGetInfo].statement, &file, KprScanFileGetInfo, &oldInfo));
	bailIfError(FskFileGetFileInfo(path, &newInfo));
	if (newInfo.fileModificationDate > newInfo.fileCreationDate)
		file.date = newInfo.fileModificationDate * 1000.0;	
	else
		file.date = newInfo.fileCreationDate * 1000.0;
	file.size = newInfo.filesize;
	file.title = title;
	if (oldInfo.id) {
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryDeletionRemove].statement, &oldInfo, NULL, NULL));
		if ((oldInfo.date != file.date) || (oldInfo.size != file.size)) {
			file.id = oldInfo.id;
			bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileUpdate].statement, &file, NULL, NULL));
			bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryModificationAdd].statement, &oldInfo, NULL, NULL));
		}
	}
	else {
		file.id = oldInfo.id = ++gLibrary.fileID;
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileInsert].statement, &file, NULL, NULL));
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryCreationAdd].statement, &oldInfo, NULL, NULL));
	}
bail:
	return err;
}

Boolean KprScanFileGetInfo(void* target, const char** keys UNUSED, const void* values)
{
	*((KprLibraryFileInfo)target) = *((KprLibraryFileInfo)values);
	return true;
}

Boolean KprScanFileProc(void* target, const char** keys UNUSED, const void* values)
{
	FskErr err = kFskErrNone;
	KprLibraryProgress progress = target;
	KprLibraryFile file = (KprLibraryFile)values;
	FskFile fref = NULL;
	unsigned char buffer[4096];
	UInt32 size;
	char *sniff = NULL;
	
	progress->index++;
	bailIfError(FskFileOpen(file->path, kFskFilePermissionReadOnly, &fref));
	bailIfError(FskFileRead(fref, sizeof(buffer), buffer, &size));
	FskFileClose(fref);
	fref = NULL;
	if (kFskErrNone == FskMediaPlayerSniffForMIME(buffer, size, NULL, file->path, &sniff)) {
		KprScanMedia(progress, file, sniff);
	}
	else if (kFskErrNone == FskImageDecompressSniffForMIME(buffer, size, NULL, file->path, &sniff)) {
		KprScanImage(progress, file, sniff);
	}
	else {
		KprScanError(progress, file);
	}
bail:
	FskMemPtrDispose(sniff);
	FskFileClose(fref);
	if (gLibrary.message)
		return KprMessageContinue(gLibrary.message);
	return true;
}

FskErr KprScanError(KprLibraryProgress progress, KprLibraryFile file)
{
	return kFskErrNone;
}

typedef struct {
	char* path;
	FskFile fref;
	unsigned char* buffer;
	UInt32 bufferSize;
} KprScanImageSpoolerRefconRecord, *KprScanImageSpoolerRefcon;

FskErr KprScanImage(KprLibraryProgress progress, KprLibraryFile file, char* mime)
{
	FskErr err = kFskErrNone;
	FskImageDecompress deco = NULL;
	KprScanImageSpoolerRefconRecord refcon;
	FskMediaPropertyValueRecord spooler, meta;
	FskFileMapping map = NULL;
	FskBitmap bitmap = NULL;
	KprLibraryImageRecord image;
	
	image.mime = mime;
	image.taken = 0;
	image.rotation = 0;
	image.width = 0;
	image.height = 0;
	image.area = 0;
	image.id = file->id;

	bailIfError(FskImageDecompressNew(&deco, 0, mime, NULL));
	FskMemSet(&refcon, 0, sizeof(refcon));
	refcon.path = file->path;
	spooler.type = kFskMediaPropertyTypeSpooler;
	FskMemSet(&spooler.value.spooler, 0, sizeof(spooler.value.spooler));
	spooler.value.spooler.refcon = &refcon;
	spooler.value.spooler.flags = kFskMediaSpoolerValid;
	spooler.value.spooler.doOpen = KprScanImageSpoolerOpen;
	spooler.value.spooler.doClose = KprScanImageSpoolerClose;
	spooler.value.spooler.doRead = KprScanImageSpoolerRead;
	bailIfError(FskImageDecompressSetProperty(deco, kFskMediaPropertySpooler, &spooler));

	if (kFskErrNone == FskImageDecompressGetMetaData(deco, 0x40019003, 1, &meta, NULL)) {
		if (meta.type == kFskMediaPropertyTypeString)
			image.taken = KprScanImageTaken(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	if (!image.taken) {
		if (kFskErrNone == FskImageDecompressGetMetaData(deco, 0x40019004, 1, &meta, NULL)) {
			if (meta.type == kFskMediaPropertyTypeString)
				image.taken = KprScanImageTaken(meta.value.str);
			FskMediaPropertyEmpty(&meta);
		}
	}
	if (!image.taken) {
		if (kFskErrNone == FskImageDecompressGetMetaData(deco, 0x40000132, 1, &meta, NULL)) {
			if (meta.type == kFskMediaPropertyTypeString)
				image.taken = KprScanImageTaken(meta.value.str);
			FskMediaPropertyEmpty(&meta);
		}
	}
	
	if (kFskErrNone == FskImageDecompressGetMetaData(deco, kFskImageDecompressMetaDataOrientation, 1, &meta, NULL)) {
		if (meta.type == kFskMediaPropertyTypeInteger) {
			switch (meta.value.integer) {
			case 0: image.rotation = 0; break;
			case 1: image.rotation = 180; break;
			case 2: image.rotation = 90; break;
			case 3: image.rotation = 270; break;
			}
		}
		FskMediaPropertyEmpty(&meta);
	}
	
	if (kFskErrNone == FskImageDecompressGetMetaData(deco, kFskImageDecompressMetaDataDimensions, 1, &meta, NULL)) {
		if (meta.type == kFskMediaPropertyTypeDimension) {
			image.width = meta.value.dimension.width;
			image.height = meta.value.dimension.height;
		}
		FskMediaPropertyEmpty(&meta);
	}
	
	if (kFskErrNone == FskImageDecompressGetMetaData(deco, kFskImageDecompressMetaDataThumbnail, 1, &meta, NULL)) {
		if (meta.type == kFskMediaPropertyTypeImage) {
			char* mime = (char*)meta.value.data.data;
			UInt32 length = FskStrLen(mime) + 1;
			FskImageDecompressDataWithOrientation(mime + length, meta.value.data.dataSize - length, mime, NULL, gScanThumbnailSize, gScanThumbnailSize, NULL, NULL, &bitmap); 
		}
		FskMediaPropertyEmpty(&meta);
	}
	
	FskMemSet(&spooler.value.spooler, 0, sizeof(spooler.value.spooler));
	bailIfError(FskImageDecompressSetProperty(deco, kFskMediaPropertySpooler, &spooler));

	if (!image.width || !image.height || !bitmap) {
		unsigned char* data;
		FskInt64 size;
		bailIfError(FskFileMap(file->path, &data, &size, 0, &map));
		if (!image.width || !image.height) {
			FskImageDecompressSetData(deco, data, (UInt32)size);
			if (kFskErrNone == FskImageDecompressGetMetaData(deco, kFskImageDecompressMetaDataDimensions, 1, &meta, NULL)) {
				if (meta.type == kFskMediaPropertyTypeDimension) {
					image.width = meta.value.dimension.width;
					image.height = meta.value.dimension.height;
				}
				FskMediaPropertyEmpty(&meta);
			}
		}
		if (!bitmap)
			FskImageDecompressDataWithOrientation(data, (UInt32)size, mime, NULL, gScanThumbnailSize, gScanThumbnailSize, NULL, NULL, &bitmap);
	}
	image.area = image.width * image.height;
	
	if (bitmap) {
		KprScanThumbnailScale(&bitmap, gScanThumbnailSize, gScanThumbnailSize);
		bailIfError(KprScanThumbnail(progress, file, bitmap));
	}

	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryImageInsert].statement, &image, NULL, NULL));
bail:
	FskFileDisposeMap(map);
	FskImageDecompressDispose(deco);
	return err;
}

FskErr KprScanImageSpoolerClose(FskMediaSpooler spooler)
{
	KprScanImageSpoolerRefcon refcon = (KprScanImageSpoolerRefcon)spooler->refcon;
	FskMemPtrDispose(refcon->buffer);
	refcon->buffer = NULL;
	FskFileClose(refcon->fref);
	refcon->fref = NULL;
	return kFskErrNone;
}

FskErr KprScanImageSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions UNUSED)
{
	FskErr err = kFskErrNone;
	KprScanImageSpoolerRefcon refcon = (KprScanImageSpoolerRefcon)spooler->refcon;
	bailIfError(FskFileOpen(refcon->path, kFskFilePermissionReadOnly, &refcon->fref));
bail:
	return err;
}

FskErr KprScanImageSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead)
{
	FskErr err = kFskErrNone;
	KprScanImageSpoolerRefcon refcon = (KprScanImageSpoolerRefcon)spooler->refcon;
	if (!refcon->buffer) {
		bailIfError(FskMemPtrNew(bytesToRead, &refcon->buffer));
		refcon->bufferSize = bytesToRead;
	}
	else if (refcon->bufferSize < bytesToRead) {
		bailIfError(FskMemPtrRealloc(bytesToRead, &refcon->buffer));
		refcon->bufferSize = bytesToRead;
	}
	bailIfError(FskFileSetPosition(refcon->fref, &position));
	err = FskFileRead(refcon->fref, bytesToRead, refcon->buffer, bytesRead);
	*buffer = refcon->buffer;
bail:
	return err;
}

double KprScanImageTaken(char* string)
{
	struct tm tm;
	time_t time;
	strptime(string, "%Y:%m:%d%n%H:%M:%S", &tm);
	time = (UInt32)mktime(&tm);
	if (time != -1)
		return time * 1000.0;
	return 0;
}

FskErr KprScanMedia(KprLibraryProgress progress, KprLibraryFile file, char* mime)
{
	FskMediaPlayer mp = NULL;
	FskMediaPropertyValueRecord meta;
	
	if (kFskErrNone == FskMediaPlayerNew(&mp, file->path, 1, mime, NULL)) {
		if (kFskErrNone == FskMediaPlayerGetProperty(mp, kFskMediaPropertyFlags, &meta)) {
			if (meta.type == kFskMediaPropertyTypeInteger) {
				if (meta.value.integer & 1)
					KprScanMediaVideo(progress, file, mime, mp);
				else
					KprScanMediaAudio(progress, file, mime, mp);
			}
			FskMediaPropertyEmpty(&meta);
		}
		FskMediaPlayerDispose(mp);
	}
	return kFskErrNone;
}
	
FskErr KprScanMediaAudio(KprLibraryProgress progress, KprLibraryFile file, char* mime, FskMediaPlayer mp)
{
	FskErr err = kFskErrNone;
	KprLibraryAudioRecord audio;
	FskSampleTime duration;
	FskMediaPropertyValueRecord meta;
	char* title = NULL;
	char* artist = NULL;
	char* album = NULL;
	SInt32 track = 0;
	double taken = 0;
	char* genre = NULL;
	FskBitmap bitmap = NULL;
	
	FskMediaPlayerGetDuration(mp, 1000, &duration);
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "FullName", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			title = FskStrDoCopy(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "Artist", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			artist = FskStrDoCopy(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "Album", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			album = FskStrDoCopy(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "TrackNumber", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			track = FskStrToL(meta.value.str, NULL, 10);
		FskMediaPropertyEmpty(&meta);
	}
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "Genre", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			genre = KprScanMediaAudioGenre(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "Year", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			taken = KprScanMediaAudioTaken(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	err = FskMediaPlayerGetMetadata(mp, "AlbumArt", 1, &meta, NULL);
	if (err == kFskErrUnknownElement)
		err = FskMediaPlayerGetMetadata(mp, "WM/WMCollectionID", 1, &meta, NULL);
	if (err == kFskErrNone) {
		if (meta.type == kFskMediaPropertyTypeImage) {
			char* mime = (char*)meta.value.data.data;
			UInt32 length = FskStrLen(mime) + 1;
			if (kFskErrNone == FskImageDecompressData(mime + length, meta.value.data.dataSize - length, mime, NULL, gScanThumbnailSize, gScanThumbnailSize, NULL, NULL, &bitmap)) {
				KprScanThumbnailScale(&bitmap, gScanThumbnailSize, gScanThumbnailSize); 
			}
		}
		FskMediaPropertyEmpty(&meta);
	}
	
	if (bitmap) {
		bailIfError(KprScanThumbnail(progress, file, bitmap));
	}
	if (title) {
		struct {
			char* title;
			SInt32 id;
		} data;
		data.title = title;
		data.id = file->id;
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryFileSetTitle].statement, &data, NULL, NULL));
	}
	
	if (FskStrCompare(mime, "video/mp4") == 0)
		FskStrCopy(mime, "audio/mp4");
	audio.mime = mime;
	audio.duration = (double)duration;
	if (artist) {
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryArtistInsert].statement, &artist, NULL, NULL));
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryArtistGetID].statement, &artist, KprLibraryIDProc, &audio.artist_id));
	}
	else
		audio.artist_id = 0;
	if (album) {
		struct {
			char* album_name;
			SInt32 artist_id;
		} data;
		data.album_name = album;
		data.artist_id = audio.artist_id;
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAlbumInsert].statement, &data, NULL, NULL));
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAlbumGetID].statement, &data, KprLibraryIDProc, &audio.album_id));
	}
	else
		audio.album_id = 0;
	audio.taken = taken;
	audio.track = track;
	if (genre) {
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryGenreInsert].statement, &genre, NULL, NULL));
		bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryGenreGetID].statement, &genre, KprLibraryIDProc, &audio.genre_id));
	}
	else
		audio.genre_id = 0;
	audio.id = file->id;
	
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryAudioInsert].statement, &audio, NULL, NULL));
bail:
	FskMemPtrDispose(title);
	FskMemPtrDispose(artist);
	FskMemPtrDispose(album);
	FskMemPtrDispose(genre);
	return err;
}

char* KprScanMediaAudioGenre(char* genre)
{
	#define gGenresCount 125
	static char* gGenres[gGenresCount] = {
		/* 0 */ "Blues",
		/* 1 */ "Classic Rock",
		/* 2 */ "Country",
		/* 3 */ "Dance",
		/* 4 */ "Disco",
		/* 5 */ "Funk",
		/* 6 */ "Grunge",
		/* 7 */ "Hip-Hop",
		/* 8 */ "Jazz",
		/* 9 */ "Metal",
		/* 10 */ "New Age",
		/* 11 */ "Oldies",
		/* 12 */ "Other",
		/* 13 */ "Pop",
		/* 14 */ "R&B",
		/* 15 */ "Rap",
		/* 16 */ "Reggae",
		/* 17 */ "Rock",
		/* 18 */ "Techno",
		/* 19 */ "Industrial",
		/* 20 */ "Alternative",
		/* 21 */ "Ska",
		/* 22 */ "Death Metal",
		/* 23 */ "Pranks",
		/* 24 */ "Soundtrack",
		/* 25 */ "Euro-Techno",
		/* 26 */ "Ambient",
		/* 27 */ "Trip-Hop",
		/* 28 */ "Vocal",
		/* 29 */ "Jazz+Funk",
		/* 30 */ "Fusion",
		/* 31 */ "Trance",
		/* 32 */ "Classical",
		/* 33 */ "Instrumental",
		/* 34 */ "Acid",
		/* 35 */ "House",
		/* 36 */ "Game",
		/* 37 */ "Sound Clip",
		/* 38 */ "Gospel",
		/* 39 */ "Noise",
		/* 40 */ "AlternRock",
		/* 41 */ "Bass",
		/* 42 */ "Soul",
		/* 43 */ "Punk",
		/* 44 */ "Space",
		/* 45 */ "Meditative",
		/* 46 */ "Instrumental Pop",
		/* 47 */ "Instrumental Rock",
		/* 48 */ "Ethnic",
		/* 49 */ "Gothic",
		/* 50 */ "Darkwave",
		/* 51 */ "Techno-Industrial",
		/* 52 */ "Electronic",
		/* 53 */ "Pop-Folk",
		/* 54 */ "Eurodance",
		/* 55 */ "Dream",
		/* 56 */ "Southern Rock",
		/* 57 */ "Comedy",
		/* 58 */ "Cult",
		/* 59 */ "Gangsta",
		/* 60 */ "Top 40",
		/* 61 */ "Christian Rap",
		/* 62 */ "Pop/Funk",
		/* 63 */ "Jungle",
		/* 64 */ "Native American",
		/* 65 */ "Cabaret",
		/* 66 */ "New Wave",
		/* 67 */ "Psychadelic",
		/* 68 */ "Rave",
		/* 69 */ "Showtunes",
		/* 70 */ "Trailer",
		/* 71 */ "Lo-Fi",
		/* 72 */ "Tribal",
		/* 73 */ "Acid Punk",
		/* 74 */ "Acid Jazz",
		/* 75 */ "Polka",
		/* 76 */ "Retro",
		/* 77 */ "Musical",
		/* 78 */ "Rock & Roll",
		/* 79 */ "Hard Rock",
		/* 80 */ "Folk",
		/* 81 */ "Folk-Rock",
		/* 82 */ "National Folk",
		/* 83 */ "Swing",
		/* 84 */ "Fast Fusion",
		/* 85 */ "Bebob",
		/* 86 */ "Latin",
		/* 87 */ "Revival",
		/* 88 */ "Celtic",
		/* 89 */ "Bluegrass",
		/* 90 */ "Avantgarde",
		/* 91 */ "Gothic Rock",
		/* 92 */ "Progressive Rock",
		/* 93 */ "Psychedelic Rock",
		/* 94 */ "Symphonic Rock",
		/* 95 */ "Slow Rock",
		/* 96 */ "Big Band",
		/* 97 */ "Chorus",
		/* 98 */ "Easy Listening",
		/* 99 */ "Acoustic",
		/* 100 */ "Humour",
		/* 101 */ "Speech",
		/* 102 */ "Chanson",
		/* 103 */ "Opera",
		/* 104 */ "Chamber Music",
		/* 105 */ "Sonata",
		/* 106 */ "Symphony",
		/* 107 */ "Booty Bass",
		/* 108 */ "Primus",
		/* 109 */ "Porn Groove",
		/* 110 */ "Satire",
		/* 111 */ "Slow Jam",
		/* 112 */ "Club",
		/* 113 */ "Tango",
		/* 114 */ "Samba",
		/* 115 */ "Folklore",
		/* 116 */ "Ballad",
		/* 117 */ "Power Ballad",
		/* 118 */ "Rhythmic Soul",
		/* 119 */ "Freestyle",
		/* 120 */ "Duet",
		/* 121 */ "Punk Rock",
		/* 122 */ "Drum Solo",
		/* 123 */ "A capella",
		/* 124 */ "Euro-House"
	};
	long index;
	char* end;
	if (*genre == '(') {
		index = FskStrToL(genre + 1, &end, 10);
		if (*end != ')')
			index = -1;
	}
	else {
		index = FskStrToL(genre , &end, 10);
		if (*end)
			index = -1;
	}
	if ((0 <= index) && (index < gGenresCount))
		genre = gGenres[index];
	return FskStrDoCopy(genre);	
}


double KprScanMediaAudioTaken(char* string)
{
	struct tm tm;
	time_t time;
    memset(&tm, 0, sizeof(tm));
	strptime(string, "%Y", &tm);
	time = (UInt32)mktime(&tm);
	if (time != -1)
		return time * 1000.0;
	return 0;
}


FskErr KprScanMediaVideo(KprLibraryProgress progress, KprLibraryFile file, char* mime, FskMediaPlayer mp)
{
	FskErr err = kFskErrNone;
	KprLibraryVideoRecord video;
	FskSampleTime duration;
	FskMediaPropertyValueRecord property;
	FskBitmap bitmap;

	video.mime = mime;
	FskMediaPlayerGetDuration(mp, 1000, &duration);
	video.duration = (double)duration;
	FskMediaPlayerGetNaturalDimensions(mp, (UInt32*)&video.width, (UInt32*)&video.height);
	video.area = video.width * video.height;
	video.id = file->id;
	
	property.type = kFskMediaPropertyTypeBoolean;
	property.value.b = true;
	FskMediaPlayerSetProperty(mp, kFskMediaPropertyScrub, &property);
	FskMediaPlayerSetTime(mp, 1000.0, (FskSampleTime)(duration >= 15000) ? 7500 : (duration / 2));
	bailIfError(FskMediaPlayerGetVideoBitmap(mp, gScanThumbnailSize, gScanThumbnailSize, &bitmap));
	bailIfError(KprScanThumbnail(progress, file, bitmap));
	
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryVideoInsert].statement, &video, NULL, NULL));
bail:
	return err;
}

FskErr KprScanThumbnail(KprLibraryProgress progress, KprLibraryFile file, FskBitmap bitmap)
{
	FskErr err = kFskErrNone;
	FskRectangleRecord bounds;
	FskImageCompress comp = NULL;
	FskMediaPropertyValueRecord property;
	KprLibraryThumbnailRecord thumbnail;

	FskBitmapGetBounds(bitmap, &bounds);
	bailIfError(FskImageCompressNew(&comp, 0, "image/jpeg", bounds.width, bounds.height));
	property.type = kFskMediaPropertyTypeFloat;
	property.value.number = 0.6;
	FskImageCompressSetProperty(comp, kFskMediaPropertyQuality, &property);
	thumbnail.id = file->id;
	thumbnail.width = bounds.width;
	thumbnail.height = bounds.height;
	bailIfError(FskImageCompressFrame(comp, bitmap, &thumbnail.jpeg, (UInt32*)&thumbnail.size, NULL, NULL, NULL, NULL, NULL));
	bailIfError(KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryThumbnailInsert].statement, &thumbnail, NULL, NULL));
bail:
	FskImageCompressDispose(comp);
	/*
	if (gLibrary.message) {
		char buffer[256];
		char* url;
		KprMessage message;
		
		sprintf(buffer, "xkpr://library/progress?c=%ld&i=%ld", progress->count, progress->index);
		url = FskStrDoCopy(buffer);
		if (url) {
			FskThreadPostCallback(FskThreadGetMain(), (FskThreadCallback)KprImageEntryProgress, url, bitmap, NULL, NULL);
			KprMessageNew(&message, url);
			KprMessageNotify(message);
		}
	}
	else
		FskBitmapDispose(bitmap);
	*/
	return err;
}

#if 0
FskErr KprScanThumbnailCompress(FskBitmap bitmap, double quality, void **data, UInt32 *dataSize)
{
	FskErr err = kFskErrNone;
	FskRectangleRecord bounds;
	FskImageCompress comp = NULL;
	FskMediaPropertyValueRecord property;
	*data = NULL;
	*dataSize = 0;
	FskBitmapGetBounds(bitmap, &bounds);
	bailIfError(FskImageCompressNew(&comp, 0, "image/jpeg", bounds.width, bounds.height));
	property.type = kFskMediaPropertyTypeBoolean;
	property.value.number = quality;
	FskImageCompressSetProperty(comp, kFskMediaPropertyQuality, &property);
	bailIfError(FskImageCompressFrame(comp, bitmap, data, dataSize, NULL, NULL, NULL, NULL, NULL));
bail:
	FskImageCompressDispose(comp);
	return err;
}
#endif

FskErr KprScanThumbnailScale(FskBitmap* bitmap, SInt32 width, SInt32 height)
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

/* SERVER */

FskErr KprDataServerOpen(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	FskAssociativeArray query = NULL;
	long id;
	if (FskStrCompareWithLength("xkpr", session->parts.scheme, 4) == 0) {
		if (FskStrCompareWithLength("/thumbnail", session->parts.path, session->parts.pathLength) == 0) {
			KprQueryParse(session->parts.query, &query);
			id = FskStrToNum(FskAssociativeArrayElementGetString(query, "id"));
			KprDBStatementExecute(gKprLibraryDBQuery[kprLibraryThumbnailGet].statement, &id, KprLibraryServerThumbnailProc, session);
			if (!session->data.buffer)
				err = kFskErrNotFound;
		}
		else {
			err = KprLibrarySessionRedirect(session);
		}
	} 
	else
		err = kFskErrNotFound;

	FskAssociativeArrayDispose(query);
	FskInstrumentedItemPrintfNormal(session, "open %s (%d)", session->url, err);
	return err;
}

Boolean KprLibraryServerThumbnailProc(void* it, const char** keys UNUSED, const void* values)
{
	KprLibrarySession session = it;
	KprLibraryThumbnailRecord *thumbnail = (KprLibraryThumbnailRecord *)values;
	FskMemPtrNew(thumbnail->size, &session->data.buffer);
	if (session->data.buffer) {
		FskMemCopy(session->data.buffer, thumbnail->jpeg, thumbnail->size);
		session->data.size = thumbnail->size;
	}
	return true;
}

/* EXTENSION */

FskErr KprExtensionNew(KprExtension *it, char c, KprExtension next)
{
	FskErr err = kFskErrNone;
	KprExtension self;
	bailIfError(FskMemPtrNewClear(sizeof(KprExtensionRecord), it));
	self = *it;
	self->c = c;
	self->next = next;
bail:
	return err;
}

void KprExtensionDispose(KprExtension self)
{
	if (self) {
		KprExtension extension = self->first, next;
		while (extension) {
			next = extension->next;
			KprExtensionDispose(extension);
			extension = next;
		}
		FskMemPtrDispose(self);
	}
}

Boolean KprExtensionExists(KprExtension self, char* s)
{
	KprExtension extension = self->first;
	char c = *s;
	if (c) {
		while (extension) {
			if (extension->c == c)
				return KprExtensionExists(extension, s + 1);
			if (extension->c > c)
				break;
			extension = extension->next;
		}
		return false;
	}
	return true;
}

void KprExtensionInsert(KprExtension self, char* s)
{
	KprExtension *address = &self->first;
	KprExtension extension;
	char c = *s;
	if (c) {
		while ((extension = *address)) {
			if (extension->c == c) {
				KprExtensionInsert(extension, s + 1);
				return;
			}
			if (extension->c > c)
				break;
			address = &extension->next;
		}
		KprExtensionNew(&extension, c, extension);
		*address = extension;
		KprExtensionInsert(extension, s + 1);
	}
}

Boolean KprExtensionProc(void* target, const char** keys UNUSED, const void* values)
{
	KprExtension self = target;
	char* s = *((char**)values);
	KprExtensionInsert(self, s);
	return true;
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

#endif
