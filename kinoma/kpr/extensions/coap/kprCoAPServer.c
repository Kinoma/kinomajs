/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "kprCoAPServer.h"
#include "kprCoAPServerSession.h"
#include "kprCoAPReceiver.h"
#include "kprCoAPEndpoint.h"
#include "FskNetInterface.h"

static FskErr KprCoAPServerInterfaceNew(KprCoAPServerInterface *it, UInt16 port, const char *interfaceName);
static FskErr KprCoAPServerInterfaceDispose(KprCoAPServerInterface self);

static void KprCoAPServer_receiveCallback(KprCoAPMessage message, UInt32 remoteAddr, UInt16 remotePort, void *refcon);
static void KprCoAPServer_errorCallback(FskErr err, const char *reason, void *refcon);

typedef FskErr (*KprCoAPServerSessionCallback)(KprCoAPServerSession *sessions, KprCoAPServerSession session, void *refcon);

static FskErr KprCoAPServerForEachSession(KprCoAPServer self, KprCoAPServerSessionCallback callback, void *refcon);

FskErr KprCoAPServerNew(KprCoAPServerCallbacks *callbacks, void *refcon, KprCoAPServer *it)
{
	FskErr err = kFskErrNone;
	KprCoAPServer self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPServerRecord), &self));
	bailIfError(KprRetainableNew(&self->retainable));

	FskTimeCallbackNew(&self->periodicalCallback);
	bailIfNULL(self->periodicalCallback);

	self->sessionId = 1;
	self->messageId = FskRandom();
	self->callbacks = *callbacks;
	self->refcon = refcon;

	*it = self;

bail:
	if (err) {
		KprCoAPServerDispose(self);
	}
	return err;
}

FskErr KprCoAPServerDispose(KprCoAPServer self)
{
	FskErr err = kFskErrNone;
	if (self && KprRetainableRelease(self->retainable)) {
		KprCoAPServerStop(self);

		FskTimeCallbackDispose(self->periodicalCallback);

		KprRetainableDispose(self->retainable);
		KprMemPtrDispose(self);
	}
	return err;
}

KprCoAPServer KprCoAPServerRetain(KprCoAPServer self)
{
	KprRetainableRetain(self->retainable);
	return self;
}

FskErr KprCoAPServerStart(KprCoAPServer self, UInt16 port, const char *interfaceName)
{
	FskErr err = kFskErrNone;
	FskNetInterfaceRecord *ifc = NULL;
	KprCoAPServerInterface si = NULL;

	self->port = port;

	if (interfaceName) {
		bailIfError(KprCoAPServerInterfaceNew(&si, port, interfaceName));
		si->server = self;
		FskListAppend(&self->interfaces, si);
		self->all = false;
		si = NULL;
	} else {
		int i, numI;

		self->all = true;
		numI = FskNetInterfaceEnumerate();
		for (i = 0; i < numI; i++) {
			FskErr ignoreErr = FskNetInterfaceDescribe(i, &ifc);
			if (ignoreErr) continue;
			if (ifc->status) {
				bailIfError(KprCoAPServerInterfaceNew(&si, port, ifc->name));
				si->server = self;
				FskListAppend(&self->interfaces, si);
				si = NULL;
			}
			FskNetInterfaceDescriptionDispose(ifc);
			ifc = NULL;
		}
	}

bail:
	KprCoAPServerInterfaceDispose(si);
	FskNetInterfaceDescriptionDispose(ifc);
	return err;
}

static FskErr KprCoAPServerDisposeSession(KprCoAPServerSession *sessions, KprCoAPServerSession session, void *refcon)
{
	FskListRemove(sessions, session);
	session->server = NULL;
	KprCoAPServerSessionDispose(session);
	return kFskErrNone;
}

FskErr KprCoAPServerStop(KprCoAPServer self)
{
	FskErr err = kFskErrNone;

	KprCoAPServerForEachSession(self, KprCoAPServerDisposeSession, NULL);

	FskTimeCallbackRemove(self->periodicalCallback);

	while (self->interfaces) {
		KprCoAPServerInterface si = self->interfaces;

		FskListRemove(&self->interfaces, si);
		KprCoAPServerInterfaceDispose(si);
	}
	return err;
}

static FskErr KprCoAPServerInvalidateSessionEndpoint(KprCoAPServerSession *sessions, KprCoAPServerSession session, void *refcon)
{
	KprCoAPEndpoint endpoint = refcon;

	if (session->endpoint == endpoint) {
		KprCoAPServerDisposeSession(sessions, session, NULL);
	}
	return kFskErrNone;
}

static void KprCoAPServerInvalidateEndpoint(KprCoAPServer self, KprCoAPEndpoint endpoint)
{
	KprCoAPServerForEachSession(self, KprCoAPServerInvalidateSessionEndpoint, endpoint);
}

UInt16 KprCoAPServerGenerateMessageId(KprCoAPServer self)
{
	UInt16 messageId = self->messageId++;
	if (messageId == 0) messageId = 1;
	return messageId;
}

static int KprCoAPServerCompareSessionsByTimestamp(const void *a, const void *b);
static void KprCoAPServerResetPeriodicalCheck(KprCoAPServer self);
static void KprCoAPServerPeriodicalCheck(FskTimeCallBack callback UNUSED, const FskTime time, void *it);

static FskErr KprCoAPServerForEachSession(KprCoAPServer self, KprCoAPServerSessionCallback callback, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPServerSession *storage[] = { &self->runningSessions, &self->finishedSessions, NULL };
	int i;

	for (i = 0; i < 2; ++i) {
		KprCoAPServerSession *sessions = storage[i], session, next;

		session = *sessions;
		while (session) {
			next = session->next;

			err = callback(sessions, session, refcon);
			if (err) return err;

			session = next;
		}
	}
	return kFskErrNone;
}

FskErr KprCoAPServerGetRunningSession(KprCoAPServer self, UInt32 sessionId, KprCoAPServerSession *it)
{
	KprCoAPServerSession session;

	session = self->runningSessions;
	while (session) {
		if (session->sessionId == sessionId) {
			*it = session;
			return kFskErrNone;
		}
		session = session->next;
	}

	return kFskErrNotFound;
}


FskErr KprCoAPServerRememberSession(KprCoAPServer self, KprCoAPServerSession session)
{
	if (!FskListContains(self->runningSessions, session)) {
		KprRetain(session);

		if (FskListContains(self->finishedSessions, session)) {
			FskListRemove(self->finishedSessions, session);
			KprCoAPServerSessionDispose(session);
		}

		FskListAppend(&self->runningSessions, session);

		KprCoAPServerResetPeriodicalCheck(self);
	}

	return kFskErrNone;
}

FskErr KprCoAPServerFinishSession(KprCoAPServer self, KprCoAPServerSession session)
{
	KprRetain(session);

	if (FskListContains(self->runningSessions, session)) {
		FskListRemove(&self->runningSessions, session);
		KprCoAPServerSessionDispose(session);
	} else if (FskListContains(self->finishedSessions, session)) {
		FskListRemove(&self->finishedSessions, session);
		KprCoAPServerSessionDispose(session);
	}

	if (session->confiramableRequest) {
		KprCoAPServerSessionEndObserve(session);

		KprRetain(session);
		FskListInsertSorted(&self->finishedSessions, session, KprCoAPServerCompareSessionsByTimestamp);
	}

	KprCoAPServerSessionDispose(session);
	KprCoAPServerResetPeriodicalCheck(self);

	return kFskErrNone;
}

static int KprCoAPServerCompareSessionsByTimestamp(const void *a, const void *b)
{
	FskTime t1 = &((KprCoAPServerSession) a)->expireAt;
	FskTime t2 = &((KprCoAPServerSession) b)->expireAt;
	return FskTimeCompare(t1, t2) * -1;
}

static KprCoAPServerSession KprCoAPServerNextExpiringSession(KprCoAPServer self)
{
	KprCoAPServerSession session, found = NULL;

	session = self->runningSessions;
	while (session) {
		if (!session->observeAccepted) {
			if (!found || FskTimeCompare(&found->expireAt, &session->expireAt) < 0) {
				found = session;
			}
		}
		session = session->next;
	}

	if (self->finishedSessions) {
		session = self->finishedSessions;
		if (!found || FskTimeCompare(&found->expireAt, &session->expireAt) < 0) {
			found = session;
		}
	}

	return found;
}

static void KprCoAPServerResetPeriodicalCheck(KprCoAPServer self)
{
	KprCoAPServerSession session = KprCoAPServerNextExpiringSession(self);

	if (session) {
		FskTimeCallbackSet(self->periodicalCallback, &session->expireAt, KprCoAPServerPeriodicalCheck, self);
	} else {
		FskTimeCallbackRemove(self->periodicalCallback);
	}
}

static FskErr KprCoAPServerDisposeExpiringSession(KprCoAPServerSession *sessions, KprCoAPServerSession session, void *refcon)
{
	FskTime time = (FskTime) refcon;

	if (!session->observeAccepted) {
		if (FskTimeCompare(time, &session->expireAt) <= 0) {
			KprCoAPServerDisposeSession(sessions, session, NULL);
		}
	}

	return kFskErrNone;
}

struct KprCoAPServerSessionCounter {
	KprCoAPEndpoint endpoint;
	int count;
};

static FskErr KprCoAPServerIncrementCountForEndpoint(KprCoAPServerSession *sessions, KprCoAPServerSession session, void *refcon)
{
	struct KprCoAPServerSessionCounter *counter = refcon;
	if (session->endpoint == counter->endpoint) {
		counter->count += 1;
	}
	return kFskErrNone;
}

static int KprCoAPServerCountSessionsForEndpoint(KprCoAPServer self, KprCoAPEndpoint endpoint)
{
	struct KprCoAPServerSessionCounter counter = { endpoint, 0 };

	KprCoAPServerForEachSession(self,  KprCoAPServerIncrementCountForEndpoint, &counter);

	return counter.count;
}

static void KprCoAPServerPeriodicalCheck(FskTimeCallBack callback UNUSED, const FskTime time, void *it)
{
	FskErr err = kFskErrNone;
	KprCoAPServer self = it;
	KprCoAPServerInterface si;

	bailIfError(KprCoAPServerForEachSession(self, KprCoAPServerDisposeExpiringSession, time));

	si = self->interfaces;
	while (si) {
		KprCoAPEndpoint endpoint = si->endpoints;
		while (endpoint) {
			KprCoAPEndpoint next = endpoint->next;

			if (KprCoAPServerCountSessionsForEndpoint(self, endpoint) == 0) {
				FskListRemove(&si->endpoints, endpoint);
				KprCoAPEndpointDispose(endpoint);
			}

			endpoint = next;
		}
		si = si->next;
	}
bail:
	KprCoAPServerResetPeriodicalCheck(self);

	if (err) {
		self->callbacks.errorCallback(err, "periodical cleaning error", self->refcon);
	}
}

static void KprCoAPServerReportError(KprCoAPServer self, FskErr err, const char *reason)
{
	self->callbacks.errorCallback(err, reason, self->refcon);
}

int KprCoAPServerNextSessionId(KprCoAPServer self)
{
	int sessionId = self->sessionId++;
	if (self->sessionId == 0) self->sessionId = 1;
	return sessionId;
}

static void KprCoAPServerInterface_retryCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, UInt32 retryCount, void *refcon);
static void KprCoAPServerInterface_errorCallback(FskErr err, const char *reason, void *refcon);
static void KprCoAPServerInterface_deliveryErrorCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, KprCoAPEndpointDeliveryFailure failure, void *refcon);

static FskErr KprCoAPServerInterfaceNew(KprCoAPServerInterface *it, UInt16 port, const char *interfaceName)
{
	FskErr err = kFskErrNone;
	KprCoAPServerInterface self = NULL;
	FskNetInterfaceRecord *ifc = NULL;
	KprCoAPReceiverCallbacks receiverCallbacks = {
		KprCoAPServer_receiveCallback,
		KprCoAPServer_errorCallback,
	};

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPServerInterfaceRecord), &self));

	self->interfaceName = FskStrDoCopy(interfaceName);
	bailIfNULL(self->interfaceName);

	bailIfError(FskNetSocketNewUDP(&self->socket, "CoAP Server Interface"));
	FskNetSocketReuseAddress(self->socket);

	ifc = FskNetInterfaceFindByName((char *) self->interfaceName);
	bailIfNULL(ifc);

	self->ipaddr = ifc->ip;
	self->port = port;
	bailIfError(FskNetSocketBind(self->socket, self->ipaddr, self->port));

	FskNetSocketMakeNonblocking(self->socket);

	bailIfError(KprCoAPReceiverNew(&self->receiver, self->socket, &receiverCallbacks, self));

	*it = self;

bail:
	FskNetInterfaceDescriptionDispose(ifc);
	if (err) {
		KprCoAPServerInterfaceDispose(self);
	}
	return err;
}

static FskErr KprCoAPServerInterfaceDispose(KprCoAPServerInterface self)
{
	FskErr err = kFskErrNone;

	if (self) {
		while (self->endpoints) {
			KprCoAPEndpoint endpoint = self->endpoints;
			FskListRemove(&self->endpoints, endpoint);
			KprCoAPEndpointDispose(endpoint);
		}

		KprCoAPReceiverDispose(self->receiver);
		FskNetSocketClose(self->socket);
		KprMemPtrDispose((char *) self->interfaceName);

		KprMemPtrDispose(self);
	}
	return err;
}

static KprCoAPEndpoint KprCoAPServerInterfaceFindEndpoint(KprCoAPServerInterface self, UInt32 remoteAddr, UInt16 remotePort)
{
	KprCoAPEndpoint endpoint = self->endpoints;

	while (endpoint) {
		if (endpoint->ipaddr == remoteAddr && endpoint->port == remotePort) {
			return endpoint;
		}
		endpoint = endpoint->next;
	}

	return NULL;
}

static FskErr KprCoAPServerInterfaceCreateEndpoint(KprCoAPServerInterface self, UInt32 remoteAddr, UInt16 remotePort, KprCoAPEndpoint *it)
{
	FskErr err = kFskErrNone;
	KprCoAPEndpoint endpoint = NULL;
	KprCoAPEndpointCallbacks callbacks = {
		KprCoAPServerInterface_retryCallback,
		KprCoAPServerInterface_errorCallback,
		KprCoAPServerInterface_deliveryErrorCallback,
	};

	bailIfError(KprCoAPEndpointNew(&endpoint, self->socket, remoteAddr, remotePort, &callbacks, self));

	FskListAppend(&self->endpoints, endpoint);

	*it = endpoint;

bail:
	return err;
}

static void KprCoAPServerInterfaceInvalidateEndpoint(KprCoAPServerInterface self, KprCoAPEndpoint endpoint)
{
	KprCoAPServerInvalidateEndpoint(self->server, endpoint);

	if (FskListContains(self->endpoints, endpoint)) {
		FskListRemove(&self->endpoints, endpoint);
		KprCoAPEndpointDispose(endpoint);
	}
}

static void KprCoAPServerInterface_retryCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, UInt32 retryCount, void *refcon)
{
	KprCoAPServerInterface self = (KprCoAPServerInterface) refcon;
	self->server->callbacks.retryCallback(message, retryCount, self->server->refcon);

}

static void KprCoAPServerInterface_errorCallback(FskErr err, const char *reason, void *refcon)
{
	KprCoAPServerInterface self = (KprCoAPServerInterface) refcon;
	KprCoAPServerReportError(self->server, err, reason);
}

static void KprCoAPServerInterface_deliveryErrorCallback(KprCoAPEndpoint endpoint, KprCoAPMessage message, KprCoAPEndpointDeliveryFailure failure, void *refcon)
{
	KprCoAPServerInterface self = (KprCoAPServerInterface) refcon;
	FskErr err = kFskErrRequestAborted;
	char *reason;

	KprCoAPServerInterfaceInvalidateEndpoint(self, endpoint);

	switch (failure) {
		case kKprCoAPEndpointDeliveryFailureMaxRetry:
			reason = "max retry";
			break;

		case kKprCoAPEndpointDeliveryFailureReset:
			reason = "reset by peer";
			err = -1;
			break;

		default:
			reason = "unknown";
			break;
	}

	KprCoAPServerReportError(self->server, err, reason);
}

static KprCoAPServerSession KprCoAPServerFindEndpointSession(KprCoAPServerSession session, KprCoAPMessage request, KprCoAPEndpoint endpoint)
{
	while (session) {
		if (session->server && KprCoAPServerSessionCompareWith(session, request, endpoint)) return session;
		session = session->next;
	}

	return NULL;
}

static KprCoAPServerSession KprCoAPServerFindSession(KprCoAPServer self, KprCoAPMessage request, KprCoAPEndpoint endpoint)
{
	KprCoAPServerSession session;

	session = KprCoAPServerFindEndpointSession(self->runningSessions, request, endpoint);
	if (session) return session;

	session = KprCoAPServerFindEndpointSession(self->finishedSessions, request, endpoint);
	if (session) return session;

	return NULL;
}

static FskErr KprCoAPServerHandleRequest(KprCoAPServer self, KprCoAPMessage request, KprCoAPEndpoint endpoint)
{
	FskErr err = kFskErrNone;
	KprCoAPServerSession session = NULL;

	session = KprCoAPServerFindSession(self, request, endpoint);

	if (session) {
		KprRetain(session);
		bailIfError(KprCoAPServerSessionRunAgain(session));
	} else {
		bailIfError(KprCoAPServerSessionNew(self, request, endpoint, &session));

		bailIfError(KprCoAPServerSessionRun(session));

		if (!session->responded || session->observeAccepted) {
			bailIfError(KprCoAPServerRememberSession(self, session));
		} else {
			bailIfError(KprCoAPServerFinishSession(self, session));
		}
	}

bail:
	KprCoAPServerSessionDispose(session);

	return err;
}

static void KprCoAPServer_receiveCallback(KprCoAPMessage request, UInt32 remoteAddr, UInt16 remotePort, void *refcon)
{
	FskErr err = kFskErrNone;
	KprCoAPServerInterface si = (KprCoAPServerInterface) refcon;
	KprCoAPServer server = si->server;
	KprCoAPEndpoint endpoint;

	endpoint = KprCoAPServerInterfaceFindEndpoint(si, remoteAddr, remotePort);
	if (endpoint == NULL) {
		bailIfError(KprCoAPServerInterfaceCreateEndpoint(si, remoteAddr, remotePort, &endpoint));
	}

	KprCoAPEndpointHandleMessage(endpoint, request);

	if (request->type == kKprCoAPMessageTypeConfirmable ||
		request->type == kKprCoAPMessageTypeNonConfirmable) {

		bailIfError(KprCoAPServerHandleRequest(server, request, endpoint));
	}

bail:
	if (err) {
		KprCoAPServer_errorCallback(err, "handle request", refcon);
	}
}

static void KprCoAPServer_errorCallback(FskErr err, const char *reason, void *refcon)
{
	KprCoAPServerInterface si = (KprCoAPServerInterface) refcon;
	KprCoAPServer server = si->server;

	KprCoAPServerReportError(server, err, reason);
}

