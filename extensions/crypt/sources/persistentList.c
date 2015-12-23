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
#include "cryptTypes.h"
#include "FskThread.h"

typedef struct {
	struct nativeList {
		struct nativeList *next;
		int version;
		char *name;
		void *data;
		int size;
	} *list;
	int sync_version;
	FskMutex mutex;
} persistentList;

#define PL_NAME		"__pl_name__"
#define PL_VERSION	"__pl_version__"

static xsSlot
pl_parse(xsMachine *the, struct nativeList *nl, xsSlot *res)
{
	*res = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(nl->size));
	FskMemCopy(xsGetHostData(*res), nl->data, nl->size);
	*res = xsCall1(xsThis, xsID("parse"), *res);
	xsSet(*res, xsID(PL_NAME), xsString(nl->name));
	xsSet(*res, xsID(PL_VERSION), xsInteger(nl->version));
	return(*res);
}

static void
pl_sync(xsMachine *the, persistentList *pl, xsSlot *client)
{
	struct nativeList *master;
	int i, n;
	xsIndex id_name = xsID(PL_NAME);
	xsIndex id_version = xsID(PL_VERSION);

	if (xsToInteger(xsGet(*client, xsID("version"))) == pl->sync_version)
		return;
	xsVar(2) = xsGet(*client, xsID("cache"));
	n = xsToInteger(xsGet(xsVar(2), xsID("length")));
	for (i = 0, master = pl->list; i < n && master != NULL;) {
		xsVar(0) = xsGet(xsVar(2), i);
		if (xsTypeOf(xsVar(0)) == xsUndefinedType ||
		    FskStrCompare(master->name, xsToString(xsGet(xsVar(0), id_name))) != 0) {
			/* i'th item removed */
			(void)xsCall2(xsVar(2), xsID("splice"), xsInteger(i), xsInteger(1));
			--n;
			continue;
		}
		if (master->version != xsToInteger(xsGet(xsVar(0), id_version)))
			xsSet(xsVar(2), i, pl_parse(the, master, &xsVar(1)));
		master = master->next;
		i++;
	}
	if (i < n) {
		/* remove all the remains */
		(void)xsCall2(xsVar(2), xsID("splice"), xsInteger(i), xsInteger(n - i));
	}
	for (; master != NULL; master = master->next)
		(void)xsCall1(xsVar(2), xsID("push"), pl_parse(the, master, &xsVar(1)));
	xsSet(*client, xsID("version"), xsInteger(pl->sync_version));
}

static void
xs_persistentList_throwIf(xsMachine *the, FskErr err)
{
	if (kFskErrNone == err)
		return;

	xsResult = xsNew1(xsGet(xsGlobal, xsID("Crypt")), xsID("Error"), xsInteger(err));
	xsThrow(xsResult);
}

void
xs_persistentList_sync(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);

	xsTry {
		xsVars(3);	/* needs in pl_sync */
		FskMutexAcquire(pl->mutex);
		pl_sync(the, pl, &xsArg(0));
	} xsCatch {
		FskMutexRelease(pl->mutex);
		xsThrow(xsException);
	}
	FskMutexRelease(pl->mutex);
}

void
xs_persistentList_set(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);
	char *name = xsToString(xsArg(0));	/* should be OK not to copy */
	struct nativeList *nl, *last = NULL;
	int i = 0;
	FskErr err;

	xsTry {
		FskMutexAcquire(pl->mutex);
		for (nl = pl->list; nl != NULL; nl = nl->next) {
			if (FskStrCompare(nl->name, name) == 0)
				break;
			last = nl;
			i++;
		}
		if (nl == NULL) {
			err = FskMemPtrNew(sizeof(struct nativeList), (FskMemPtr *)&nl);
			xs_persistentList_throwIf(the, err);
			if (last)
				last->next = nl;
			else
				pl->list = nl;
			nl->next = NULL;
			nl->version = 0;
			nl->name = FskStrDoCopy(name);
			pl->sync_version++;
		}
		else {
			if (nl->data != NULL)
				FskMemPtrDispose(nl->data);
		}
		nl->version++;
		xsResult = xsCall1(xsThis, xsID("serialize"), xsArg(1));
		nl->size = xsToInteger(xsGet(xsResult, xsID("length")));
		err = FskMemPtrNew(nl->size, (FskMemPtr *)&nl->data);
		xs_persistentList_throwIf(the, err);
		FskMemCopy(nl->data, xsGetHostData(xsResult), nl->size);
		xsResult = xsArg(1);
	} xsCatch {
		FskMutexRelease(pl->mutex);
		xsThrow(xsException);
	}
	FskMutexRelease(pl->mutex);
}

void
xs_persistentList_get(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);
	char *name = xsToString(xsArg(0));	/* be careful not to use with the 'xs' functions */
	xsIndex id_name = xsID(PL_NAME);
	xsIndex id_version = xsID(PL_VERSION);

	xsTry {
		struct nativeList *nl;
		int i = 0;

		xsVars(1);
		FskMutexAcquire(pl->mutex);
		xsVar(0) = xsGet(xsArg(1), xsID("cache"));
		xsResult = xsUndefined;
		for (nl = pl->list; nl != NULL; nl = nl->next, i++) {
			if (FskStrCompare(nl->name, name) == 0) {
				if (xsHas(xsVar(0), i))
					xsResult = xsGet(xsVar(0), i);
				if (xsTypeOf(xsResult) == xsUndefinedType ||
				    FskStrCompare(nl->name, xsToString(xsGet(xsResult, id_name))) != 0 ||
				    xsToInteger(xsGet(xsResult, id_version)) != nl->version)
					xsSet(xsVar(0), i, pl_parse(the, nl, &xsResult));
				break;
			}
		}
	} xsCatch {
		FskMutexRelease(pl->mutex);
		xsThrow(xsException);
	}
	FskMutexRelease(pl->mutex);
}

void
xs_persistentList_nth(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);
	int n = xsToInteger(xsArg(0));
	xsIndex id_name = xsID(PL_NAME);
	xsIndex id_version = xsID(PL_VERSION);

	xsTry {
		struct nativeList *nl;
		int i = n;

		xsVars(1);
		FskMutexAcquire(pl->mutex);
		xsVar(0) = xsGet(xsArg(1), xsID("cache"));
		xsResult = xsUndefined;
		for (nl = pl->list; nl != NULL && --i >= 0; nl = nl->next)
			;
		if (nl != NULL) {
			if (xsHas(xsVar(0), n))
				xsResult = xsGet(xsVar(0), n);
			if (xsTypeOf(xsResult) == xsUndefinedType ||
			    FskStrCompare(nl->name, xsToString(xsGet(xsResult, id_name))) != 0 ||
			    xsToInteger(xsGet(xsResult, id_version)) != nl->version)
				xsSet(xsVar(0), n, pl_parse(the, nl, &xsResult));
		}
	} xsCatch {
		FskMutexRelease(pl->mutex);
		xsThrow(xsException);
	}
	FskMutexRelease(pl->mutex);
}

void
xs_persistentList_remove(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);
	char *name = xsToString(xsArg(0));
	struct nativeList **nlp;
	int i = 0;

	xsTry {
		FskMutexAcquire(pl->mutex);
		for (nlp = &pl->list; *nlp != NULL; nlp = &(*nlp)->next) {
			struct nativeList *nl = *nlp;
			if (FskStrCompare(nl->name, name) == 0) {
				*nlp = nl->next;
				FskMemPtrDispose(nl->name);
				FskMemPtrDispose(nl->data);
				FskMemPtrDispose(nl);
				break;
			}
			i++;
		}
		pl->sync_version++;
	} xsCatch {
		FskMutexRelease(pl->mutex);
		xsThrow(xsException);
	}
	FskMutexRelease(pl->mutex);
}

void
xs_persistentList_push(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);
	int n = xsToInteger(xsArg(0));
	struct nativeList **nlp, *nl;

	xsTry {
		FskMutexAcquire(pl->mutex);
		for (nlp = &pl->list; *nlp != NULL && --n >= 0; nlp = &(*nlp)->next)
			;
		if (*nlp == NULL)
			return;
		nl = *nlp;
		*nlp = nl->next;
		nl->next = pl->list;
		pl->list = nl;
		pl->sync_version++;
	} xsCatch {
		FskMutexRelease(pl->mutex);
		xsThrow(xsException);
	}
	FskMutexRelease(pl->mutex);
}

void
xs_persistentList_getLength(xsMachine *the)
{
	persistentList *pl = xsGetHostData(xsThis);
	struct nativeList *nl;
	int i = 0;

	for (nl = pl->list; nl != NULL; nl = nl->next)
		i++;
	xsResult = xsInteger(i);
}

void
xs_persistentList_constructor(xsMachine *the)
{
	persistentList *pl;
	FskErr err;

	xsVars(1);
	err = FskMemPtrNew(sizeof(persistentList), (FskMemPtr *)&pl);
	xs_persistentList_throwIf(the, err);
	err = FskMutexNew(&pl->mutex, "persistentList");
	if (err) {
		FskMemPtrDispose(pl);
		xs_persistentList_throwIf(the, err);
	}
	pl->list = NULL;
	pl->sync_version = 0;
	xsSetHostData(xsThis, pl);
}

void
xs_persistentList_destructor(void *data)
{
	persistentList *pl = data;
	struct nativeList *nl, *next;

	if (pl == NULL)
		return;
	FskMutexAcquire(pl->mutex);
	for (nl = pl->list; nl != NULL; nl = next) {
		next = nl->next;
		FskMemPtrDispose(nl->name);
		FskMemPtrDispose(nl->data);
		FskMemPtrDispose(nl);
	}
	FskMutexRelease(pl->mutex);
	FskMutexDispose(pl->mutex);
	FskMemPtrDispose(pl);
}
