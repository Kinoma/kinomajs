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
#include "kprSQLite.h"
#include "kprURL.h"
#include "kprUtilities.h"

//--------------------------------------------------
//--------------------------------------------------
// DB
//--------------------------------------------------
//--------------------------------------------------

static int gDBCount = 0;

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprDBInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprDB", FskInstrumentationOffset(KprDBRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprDBNew(KprDB* it, char* path, Boolean rw)
{
	FskErr err = kFskErrNone;
	KprDB self = NULL;
	if (!gDBCount)
		bailIfError(sqlite3_initialize());
	bailIfError(KprMemPtrNewClear(sizeof(KprDBRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprDBInstrumentation);
	bailIfError(sqlite3_open_v2(path, &self->data, rw ? SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE : SQLITE_OPEN_READONLY, NULL));
	gDBCount++;
bail:
	return err;
}

void KprDBDispose(KprDB self)
{
	if (self) {
		if (self->data) {
			sqlite3_close(self->data);
			gDBCount--;
			if (!gDBCount)
				sqlite3_shutdown();
		}
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

FskErr KprDBExecuteFile(KprDB self, char* path)
{
	FskErr err = kFskErrNone;
	FskFileInfo info;
	FskFile fref = NULL;
	char *buffer = NULL;;
	
	bailIfError(FskFileGetFileInfo(path, &info));
	bailIfError(KprMemPtrNew(info.filesize + 1, &buffer));
	bailIfError(FskFileOpen(path, kFskFilePermissionReadOnly, &fref));
	bailIfError(FskFileRead(fref, info.filesize, buffer, NULL));
	buffer[info.filesize] = 0;
	sqlite3_exec(self->data, buffer, NULL, NULL, NULL);
bail:
	KprMemPtrDispose(buffer);
	FskFileClose(fref);
	return err;
}

FskErr KprDBExecuteBuffer(KprDB self, char* buffer)
{
	sqlite3_exec(self->data, buffer, NULL, NULL, NULL);
	return kFskErrNone;
}

//--------------------------------------------------
//--------------------------------------------------
// DB STATEMENT
//--------------------------------------------------
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprDBStatementInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprDBStatement", FskInstrumentationOffset(KprDBStatementRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprDBStatementNew(KprDBStatement* it, KprDB db, const char* text, char* bindFormat, char* rowFormat)
{
	FskErr err = kFskErrNone;
	KprDBStatement self = NULL;
	SInt32 count, size, i;
	bailIfError(KprMemPtrNewClear(sizeof(KprDBStatementRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprDBStatementInstrumentation);
	bailIfError(sqlite3_prepare_v2(db->data, text, FskStrLen(text), &self->data, NULL));
	count = sqlite3_bind_parameter_count(self->data);
	if (count && !bindFormat) BAIL(kFskErrInvalidParameter);
	if (!count && bindFormat) BAIL(kFskErrInvalidParameter);
	if (count) {
		if ((SInt32)FskStrLen(bindFormat) != count) BAIL(kFskErrInvalidParameter);
		self->bindFormat = FskStrDoCopy(bindFormat);
		bailIfNULL(self->bindFormat);
	}
	count = sqlite3_column_count(self->data);
	if (count && !rowFormat) BAIL(kFskErrInvalidParameter);
	if (!count && rowFormat) BAIL(kFskErrInvalidParameter);
	if (count) {
		if ((SInt32)FskStrLen(rowFormat) != count) BAIL(kFskErrInvalidParameter);
		self->rowFormat = FskStrDoCopy(rowFormat);
		bailIfNULL(self->rowFormat);
		bailIfError(KprMemPtrNewClear(count * sizeof(char*), &self->keys));
		for (size = 0, i = 0; rowFormat[i]; i++) {
			self->keys[i] = sqlite3_column_name(self->data, i);
			switch (rowFormat[i]) {
				case 'b':
					size += sizeof(void*) + sizeof(UInt32);
					break;
				case 'd':
					size += sizeof(double);
					break;
				case 'i':
					size += sizeof(SInt32);
					break;
				case 'I':
					size += sizeof(FskInt64);
					break;
				case 'r':
					size += sizeof(SInt32);
					break;
				case 't':
					size += sizeof(char*);
					break;
				default:
					BAIL(kFskErrInvalidParameter);
					break;
			}
		}
		bailIfError(KprMemPtrNewClear(size, &self->values));
	}
bail:
	if (err)
		KprDBStatementDispose(self);
	return err;
}

void KprDBStatementDispose(KprDBStatement self)
{
	if (self) {
		if (self->data) {
			sqlite3_finalize(self->data);
			self->data = NULL;
		}
		KprMemPtrDispose(self->bindFormat);
		KprMemPtrDispose(self->rowFormat);
		KprMemPtrDispose(self->keys);
		KprMemPtrDispose(self->values);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

FskErr KprDBStatementExecute(KprDBStatement self, void* binding, KprDBStatementRowProc rowProc, void* target)
{
	FskErr err = kFskErrNone;
	Boolean more = true;
	int result;
	char* ptr = NULL;
	UInt32 i;
	char* format;
	void* blob;
	char* text;
	SInt32 size;
	SInt32 reference;
	
	format = self->bindFormat;
	if (format && !binding) BAIL(kFskErrInvalidParameter);
	if (!format && binding) BAIL(kFskErrInvalidParameter);
	if (format) {
		ptr = binding;
		for (i = 0; format[i]; i++) {
			switch (format[i]) {
				case 'b':
					blob = *(void**)ptr;
					ptr += sizeof(void*);
					size = *((UInt32*)ptr);
					ptr +=sizeof(UInt32);
					if (blob && size) {
						bailIfError(sqlite3_bind_blob(self->data, i + 1, blob, size, SQLITE_STATIC));
					}
					else {
						bailIfError(sqlite3_bind_null(self->data, i + 1));
					}
					break;
				case 'd':
					bailIfError(sqlite3_bind_double(self->data, i + 1, *((double*)ptr)));
					ptr += sizeof(double);
					break;
				case 'i':
					bailIfError(sqlite3_bind_int(self->data, i + 1, *((SInt32*)ptr)));
					ptr += sizeof(SInt32);
					break;
				case 'I':
					bailIfError(sqlite3_bind_int64(self->data, i + 1, *((FskInt64*)ptr)));
					ptr += sizeof(FskInt64);
					break;
				case 'r':
					reference = *(SInt32*)ptr;
					if (reference) {
						bailIfError(sqlite3_bind_int64(self->data, i + 1, reference));
					}
					else {
						bailIfError(sqlite3_bind_null(self->data, i + 1));
					}
					ptr += sizeof(SInt32);
					break;
				case 't':
					text = *(char**)ptr;
					if (text) {
						bailIfError(sqlite3_bind_text(self->data, i + 1, text, FskStrLen(text), SQLITE_STATIC));
					}
					else {
						bailIfError(sqlite3_bind_null(self->data, i + 1));
					}
					ptr += sizeof(char*);
					break;
				default:
					BAIL(kFskErrInvalidParameter);
					break;
			}
		}
	}
	while (more) {
		switch ((result = sqlite3_step(self->data))) {
			case SQLITE_ROW:
				ptr = self->values;
				format = self->rowFormat;
				for (i = 0; format[i]; i++) {
					switch (format[i]) {
						case 'b':
							*((const void**)ptr) = sqlite3_column_blob(self->data, i);
							ptr += sizeof(void*);
							*((UInt32*)ptr) = sqlite3_column_bytes(self->data, i);
							ptr +=sizeof(UInt32);
							break;
						case 'd':
							*((double*)ptr) = sqlite3_column_double(self->data, i);
							ptr += sizeof(double);
							break;
						case 'i':
							*((SInt32*)ptr) = sqlite3_column_int(self->data, i);
							ptr += sizeof(SInt32);
							break;
						case 'I':
							*((FskInt64*)ptr) = sqlite3_column_int64(self->data, i);
							ptr += sizeof(FskInt64);
							break;
						case 'r':
							*((SInt32*)ptr) = sqlite3_column_int(self->data, i);
							ptr += sizeof(SInt32);
							break;
						case 't':
							*((const UInt8**)ptr) = sqlite3_column_text(self->data, i);
							ptr += sizeof(UInt8*);
							break;
						default:
							BAIL(kFskErrInvalidParameter);
							break;
					}
				}
				more = (*rowProc)(target, self->keys, self->values);
				break;
			case SQLITE_DONE:
				more = false;
				break;
			default:
				more = false;
				break;
		}
	}
	bailIfError(sqlite3_reset(self->data));
	bailIfError(sqlite3_clear_bindings(self->data));
bail:
	return err;
}
