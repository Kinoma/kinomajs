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
#ifndef __KPRDB__
#define __KPRDB__

#define KPR_NO_GRAMMAR 1
#include "kpr.h"
#include <sqlite3.h>

typedef struct KprDBStruct KprDBRecord, *KprDB;
typedef struct KprDBStatementStruct KprDBStatementRecord, *KprDBStatement;
typedef struct KprDBQueryStruct KprDBQueryRecord, *KprDBQuery;

//--------------------------------------------------
//--------------------------------------------------
// DB
//--------------------------------------------------
//--------------------------------------------------

struct KprDBStruct {
	sqlite3* data;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprDBNew(KprDB* it, char* path, Boolean rw);
FskAPI(void) KprDBDispose(KprDB self);
FskAPI(FskErr) KprDBExecuteFile(KprDB self, char* path);
FskAPI(FskErr) KprDBExecuteBuffer(KprDB self, char* buffer);

//--------------------------------------------------
//--------------------------------------------------
// DB STATEMENT
//--------------------------------------------------
//--------------------------------------------------

typedef Boolean (*KprDBStatementRowProc)(void* target, const char** keys, const void* values);

struct KprDBStatementStruct {
	sqlite3_stmt* data;
	char* bindFormat;
	char* rowFormat;
	const char** keys;
	void* values;
	FskInstrumentedItemDeclaration
};

struct KprDBQueryStruct {
	char* sql;
	char* bind;
	char* row;
	KprDBStatement statement;
};

FskAPI(FskErr) KprDBStatementNew(KprDBStatement* it, KprDB db, const char* text, char* bindFormat, char* rowFormat);
FskAPI(void) KprDBStatementDispose(KprDBStatement self);
FskAPI(FskErr) KprDBStatementExecute(KprDBStatement self, void* binding, KprDBStatementRowProc rowProc, void* target);

#endif
