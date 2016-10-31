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
#define __FSKNETUTILS_PRIV__
#define __FSKECMASCRIPT_PRIV__

#include "Fsk.h"
#include "FskEnvironment.h"
#include "FskUtilities.h"
#include "FskSSL.h"
#include "FskFiles.h"
#include "FskTextConvert.h"
#include "FskTime.h"
#if TARGET_OS_ANDROID
#define __FSKTHREAD_PRIV__
#include "FskThread.h"
#include "FskHardware.h"
#endif

#include "kprShell.h"
#include "kprMessage.h"


#include "FskExtensions.h"
#include "FskECMAScript.h"
#include "FskEnvironment.h"
#include "FskUtilities.h"
#if TARGET_OS_ANDROID
#include "FskThread.h"
#include "FskHardware.h"
#endif

#if !FSK_EXTENSION_EMBED
#include "FskSSL.xs.h"

FskExport(xsGrammar *) fskGrammar = &FskSSLGrammar;
#endif /* !FSK_EXTENSION_EMBED */

FskExport(FskErr) FskSSLAll_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}

FskExport(FskErr) FskSSLAll_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

/* why this has to be a xs function, not just called from fskLoad()? Because fskLoad can be called after <program> */
void xs_FskSSL_getCertPath(xsMachine *the)
{
	char *capath;

#if TARGET_OS_ANDROID
	extern void unpackAndroid();

	unpackAndroid();
	capath = gAndroidCallbacks->getStaticDataDirCB();
#else
	capath = FskEnvironmentGet("applicationPath");
#endif
	xsResult = xsString(capath);
}

#if TARGET_OS_ANDROID
void unpackAndroid() {
	char * src, *dst, buf[4096];
	int doit=0;
	UInt32 amt, amtWrt;
	FskFileInfo info, infoSrc;
	FskFile srcFref, dstFref;
	FskErr err;

	dst = FskStrDoCat(gAndroidCallbacks->getStaticDataDirCB(), "ca-bundle.crt");
	src = FskStrDoCat(gAndroidCallbacks->getStaticAppDirCB(), "/res/raw/kinoma.jet/ca-bundle.crt");
	if (kFskErrFileNotFound == FskFileGetFileInfo(dst, &info)) {
		fprintf(stderr, "dst: %s not found\n", dst);
		doit = 1;
	}
	else if (kFskErrNone == FskFileGetFileInfo(src, &infoSrc)) {
		if (infoSrc.filesize != info.filesize) {
			fprintf(stderr, "src size: %lld, dstSize: %lld\n", infoSrc.filesize, info.filesize);
			doit = 1;
		}
	}
	if (doit) {
		fprintf(stderr, "Need to copy it over.\n");
		err = FskFileOpen(src, kFskFilePermissionReadOnly, &srcFref);
		if (err) fprintf(stderr, "open %s failed %d\n", src, err);
		err = FskFileCreate(dst);
		if (err) fprintf(stderr, "create %s failed %d\n", dst, err);
		err = FskFileOpen(dst, kFskFilePermissionReadWrite, &dstFref);
		if (err) fprintf(stderr, "open %s failed %d\n", dst, err);
		while (kFskErrNone == err) {
			err = FskFileRead(srcFref, 4096, buf, &amt);
			if (err) fprintf(stderr, "read %s (%u bytes) failed %d\n", src, amt, err);
			if (0 >= amt)
				break;
			while (amt) {
				err = FskFileWrite(dstFref, amt, buf, &amtWrt);
				if (err) fprintf(stderr, "write %s (%u bytes) failed %d\n", dst, amt, err);
				amt -= amtWrt;
			}
		}
		err = FskFileClose(dstFref);
		if (err) fprintf(stderr, "close %s failed %d\n", src, err);
		err = FskFileClose(srcFref);
		if (err) fprintf(stderr, "close %s failed %d\n", src, err);
	}
	FskMemPtrDispose(src);
	FskMemPtrDispose(dst);
}
#endif

typedef struct {
	xsMachine *vm;
	xsSlot ssl, socket;
	FskNetSocketCreatedCallback socketCallback;
	void *callbackData;
	FskSocket skt;
	FskTimeCallBack timer;
} FskSSL;

#if !FSK_EMBED
#include "FskCore.xs.h"
#endif

#if FSK_EMBED
extern void FskExtensionsEmbedLoad(char *vmName);
extern void FskExtensionsEmbedUnload(char *vmName);
extern void FskExtensionsEmbedGrammar(char *vmName, char **xsbName, xsGrammar **grammar);
#endif

#define xsHostData(ptr)	_xsHostData(the, ptr)
static xsSlot _xsHostData(xsMachine *the, void *ptr)
{
	the->scratch = xsNewHostObject(NULL);
	xsSetHostData(the->scratch, ptr);
	return the->scratch;
}

static void
disposeSSLRootVM()
{
}

static FskErr
makeSSLRootVM(const char *calistpath)
{
	FskErr err = kFskErrNone;

	xsBeginHost(gShell->root);
	xsTry {
		xsCall1_noResult(xsGet(xsGlobal, xsID("FskSSL")), xsID("loadRootCerts"), xsString((xsStringValue)calistpath));
	} xsCatch {
		err = kFskErrOperationFailed;
	}
	xsEndHost();
	return err;
}

static void
time_callback(FskTimeCallBack callback, const FskTime time, void *param)
{
	FskSSL *fssl = param;
	FskSocket skt;
	void *refCon;

	FskTimeCallbackDispose(callback);
	fssl->timer = NULL;
	skt = fssl->skt;
	refCon = fssl->callbackData;
	if (fssl->skt == NULL) {	/* check if some error has occurred and dispose everything here in that case as there's no chance to do after this point */
		FskNetSocketCreatedCallback socketCallback = fssl->socketCallback;
		FskSSLDispose(fssl);
		(*socketCallback)(NULL, refCon);
	}
	else
		(*fssl->socketCallback)(skt, refCon);
	/* nothing should be here! */
}

static void
xs_handshake_finished_callback(xsMachine *the)
{
	FskSSL *fssl = xsGetHostData(xsGet(xsThis, xsID("_hostData")));
	int ac = xsToInteger(xsArgc);
	FskSocket skt = ac > 0 && xsTest(xsArg(0)) ? (FskSocket)xsGetHostData(xsArg(0)): NULL;
	FskErr err = ac > 1 && xsTypeOf(xsArg(1)) == xsIntegerType ? (FskErr)xsToInteger(xsArg(1)): kFskErrNone;

	if (fssl->socketCallback == NULL)
		return;	/* callback is the only way to get a socket so no callback no socket */

	fssl->skt = skt;
	if (skt != NULL) {
		skt->lastErr = (err == kFskErrNone) ? kFskErrNone : kFskErrSSLHandshakeFailed;	// ignore the detailed error
		skt->fssl = fssl;
	}
	/* it is possible that the callback calls FskSSLDispose which destroys not only all objects but the VM, so we need to be free from VM when the callback is called */
	FskTimeCallbackNew(&fssl->timer);
	FskTimeCallbackScheduleNextRun(fssl->timer, time_callback, fssl);
}


FskErr
FskSSLNew(void **fsslp, const char *host, int port, Boolean blocking, long flags, int priority)
{
	FskSSLOption option;

	FskMemSet(&option, 0, sizeof(FskSSLOption));
	option.host = host;
	option.port = port;
	option.blocking = blocking;
	option.synchronous = !!(flags & kConnectFlagsSynchronous);

	return FskSSLNewWithOption(fsslp, &option);
}

FskErr FskSSLNewWithOption(void **fsslp, FskSSLOption *option)
{
	FskSSL *fssl;
	FskErr err;

	if ((err = FskMemPtrNewClear(sizeof(FskSSL), (FskMemPtr*)(void*)&fssl)) != kFskErrNone)
		return err;
	fssl->vm = KprServicesGetMachine();

	xsBeginHost(fssl->vm);
	xsTry {
		const char *prStr;
		xsVars(3);
		/* construct the options */
		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		if (option->blocking)
			xsSet(xsVar(0), xsID("blocking"), xsTrue);
		if (option->synchronous)
			xsSet(xsVar(0), xsID("synchronous"), xsTrue);
		switch (option->priority) {
			default:
			case kFskNetSocketLowestPriority: prStr = "lowest"; break;
			case kFskNetSocketLowPriority: prStr = "low"; break;
			case kFskNetSocketMediumPriority: prStr = "medium"; break;
			case kFskNetSocketHighPriority: prStr = "high"; break;
			case kFskNetSocketHighestPriority: prStr = "highest"; break;
		}
		(void)xsSet(xsVar(0), xsID("priority"), xsString((xsStringValue)prStr));
		(void)xsSet(xsVar(0), xsID("raw"), xsTrue);
		xsVar(1) = xsNew3(xsGet(xsGlobal, xsID("Stream")), xsID("Socket"), xsString((xsStringValue)option->host), xsInteger(option->port), xsVar(0));
		fssl->socket = xsVar(1); xsRemember(fssl->socket);

		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		xsSet(xsVar(0), xsID("server_name"), xsString((char *) option->host));
		if (option->protocolVersion) {
			xsVar(2) = xsNewInstanceOf(xsObjectPrototype);

			xsSet(xsVar(2), xsID("major"), xsInteger(option->protocolVersion & 0xff));
			xsSet(xsVar(2), xsID("minor"), xsInteger(option->protocolVersion >> 8));

			xsSet(xsVar(0), xsID("protocolVersion"), xsVar(2));
		}
		if (option->applicationProtocols) {
			xsSet(xsVar(0), xsID("application_layer_protocol_negotiation"), xsString((char *) option->applicationProtocols));
		}
		xsVar(1) = xsNew1(xsGet(xsGlobal, xsID("FskSSL")), xsID("Session"), xsVar(0));
		fssl->ssl = xsVar(1); xsRemember(fssl->ssl);
		/* translation
		 var opt;
		 opt = {};
		 if (option->blocking) opt.blocking = true;
		 if (option->synchronous) opt.synchronous = true;
		 switch (option->priority) {
			default:
			case kFskNetSocketLowestPriority:
		 opt.priority = "lowest"; break;
			case kFskNetSocketLowPriority:
		 opt.priority = "low"; break;
			case kFskNetSocketMediumPriority:
		 opt.priority = "medium"; break;
			case kFskNetSocketHighPriority:
		 opt.priority = "high"; break;
			case kFskNetSocketHighestPriority:
		 opt.priority = "highest"; break;
		 }
		 opt.raw = xsTrue;
		 fssl->socket = new Stream.Socket(option->host, option->port, opt);
		 xsRemember(fssl->socket);

		 opt = {server_name: option->host};
		 if (option->protocolVersion) {
			opt.protocolVersion = {
		 major: option->protocolVersion & 0xff,
		 minor: option->protocolVersion >> 8,
			}
		 }
		 if (option->applicationProtocols) {
			opt.application_layer_protocol_negotiation = option->applicationProtocols;
		 }
		 fssl->ssl = new FskSSL.Session(opt);
		 xsRemember(fssl->ssl);
		 */
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost();

	if (err == kFskErrNone) {
		if (fsslp != NULL)
			*fsslp = fssl;
	}
	else {
		FskMemPtrDispose(fssl);
	}
	return err;
}

FskErr
FskSSLAttach(void **fsslp, FskSocket skt)
{
	FskSSL *fssl;
	FskErr err;

	if ((err = FskMemPtrNewClear(sizeof(FskSSL), (FskMemPtr*)(void*)&fssl)) != kFskErrNone)
		return err;
	fssl->vm = KprServicesGetMachine();

	xsBeginHost(fssl->vm);
	xsTry {
		xsVars(2);
		xsVar(0) = xsNew1(xsGet(xsGlobal, xsID("Stream")), xsID("Socket"), xsHostData(skt));
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		(void)xsSet(xsVar(1), xsID("raw"), xsTrue);
		xsCall1_noResult(xsVar(0), xsID("setProperties"), xsVar(1));
		fssl->socket = xsVar(0); xsRemember(fssl->socket);
		xsVar(0) = xsNew0(xsGet(xsGlobal, xsID("FskSSL")), xsID("Session"));
		fssl->ssl = xsVar(0); xsRemember(fssl->ssl);
		/*
		 var sock;
		 sock = new Stream.Socket(skt);
		 sock.setProperties({raw:true});
		 fssl->socket = sock;
		 xsRemember(fssl->socket);

		 fssl->ssl = new FskSSL.Session();
		 xsRemember(fssl->ssl);
		 */
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost();

	if (err == kFskErrNone) {
		if (fsslp != NULL)
			*fsslp = fssl;
	}
	else {
		FskMemPtrDispose(fssl);
	}
	return err;
}

void
FskSSLDispose(void *a)
{
	FskSSL *fssl = a;

	if (fssl->timer != NULL)
		FskTimeCallbackDispose(fssl->timer);

	/* when a VM is disposed, all objects associated to the vm will be discarded? */
	xsBeginHost(fssl->vm);
	xsTry {
		if (fssl->skt != NULL) {
			/* someone else owns the skt */
			xsCall0_noResult(fssl->socket, xsID("detachData"));
		}
		xsCall0_noResult(fssl->socket, xsID("close"));
	} xsCatch {
	}
	xsForget(fssl->socket);
	xsForget(fssl->ssl);
	xsEndHost();

	FskMemPtrDispose(a);
}

FskErr
FskSSLLoadCerts(void *a, FskSocketCertificateRecord *cert)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	xsBeginHost(fssl->vm);

	xsTry {
		xsVars(2);

		xsVar(0) = xsUndefined;
		xsVar(1) = xsUndefined;
		if (cert != NULL) {
			if (cert->certificates != NULL && cert->certificatesSize > 0) {
				xsVar(0) = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(cert->certificatesSize));
				FskMemCopy(xsGetHostData(xsVar(0)), cert->certificates, cert->certificatesSize);
			}
			if (cert->policies != NULL) {
				if (cert->certificates == NULL)
					xsVar(0) = xsNew0(xsGlobal, xsID("Chunk"));	// create a null chunk just for setting the polices
				xsSet(xsVar(0), xsID("policies"), xsString(cert->policies));
			}
			if (cert->hostname != NULL) {
				xsSet(fssl->socket, xsID("hostname"), xsString(cert->hostname));	// too easy but should work...
			}
			if (cert->key != NULL && cert->keySize > 0) {
				xsVar(1) = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(cert->keySize));
				FskMemCopy(xsGetHostData(xsVar(1)), cert->key, cert->keySize);
			}
		}
		xsCall2_noResult(fssl->ssl, xsID("loadCerts"), xsVar(0), xsVar(1));
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}

	xsEndHost();
	return err;
}

FskErr
FskSSLHandshake(void *a, FskNetSocketCreatedCallback callback, void *refCon, Boolean initiate, int timeout)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	fssl->socketCallback = callback;
	fssl->callbackData = refCon;

	xsBeginHost(fssl->vm);

	xsTry {
		xsVars(1);

		/* set session._hostData = ssl, just for callback */
		xsVar(0) = xsNewHostObject(NULL);
		xsSetHostData(xsVar(0), fssl);
		xsSet(fssl->ssl, xsID("_hostData"), xsVar(0));
		xsVar(0) = xsNewHostFunction(xs_handshake_finished_callback, 2);
		xsSet(fssl->ssl, xsID("_hostCallback"), xsVar(0));
		xsCall4_noResult(fssl->ssl, xsID("handshake"), fssl->socket, xsVar(0), initiate ? xsTrue : xsFalse, xsInteger(timeout));
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}

	xsEndHost();
	return err;
}

FskErr
FskSSLRead(void *a, void *buf, int *bufLen)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;
	int len = *bufLen, nread;

	if (fssl->skt == NULL)
		return kFskErrOperationFailed;

	xsBeginHost(fssl->vm);
	xsTry {
		xsCall1_noResult(fssl->socket, xsID("attachData"), xsHostData(fssl->skt));
		xsResult = xsCall2(fssl->ssl, xsID("read"), fssl->socket, xsInteger(len));
		if (xsTest(xsResult)) {
			nread = xsToInteger(xsGet(xsResult, xsID("length")));
			if (nread > len)
				nread = len;
			FskMemCopy(buf, xsGetHostData(xsResult), nread);
			xsCall0_noResult(xsResult, xsID("free"));
			*bufLen = nread;
			err = nread == 0 ? kFskErrNoData: kFskErrNone;
		}
		else
			err = kFskErrConnectionClosed;
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost();
	return err;
}

FskErr
FskSSLWrite(void *a, const void *buf, int *bufLen)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;
	int len = *bufLen;

	if (fssl->skt == NULL)
		return kFskErrOperationFailed;

	if (buf == NULL || len == 0)
		return kFskErrNone;

	xsBeginHost(fssl->vm);
	xsTry {
		xsVars(1);
		xsCall1_noResult(fssl->socket, xsID("attachData"), xsHostData(fssl->skt));
		xsVar(0) = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(len));
		FskMemCopy(xsGetHostData(xsVar(0)), buf, len);
		xsResult = xsCall2(fssl->ssl, xsID("write"), fssl->socket, xsVar(0));
		*bufLen = xsToInteger(xsResult);
		if (*bufLen == 0)
			err = kFskErrNoData;
		xsCall0_noResult(xsVar(0), xsID("free"));
	}  xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost();
	return err;
}

FskErr
FskSSLFlush(void *a)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	if (fssl->skt == NULL)
		return kFskErrOperationFailed;

	xsBeginHost(fssl->vm);
	xsTry {
		xsVars(1);
		xsCall1_noResult(fssl->socket, xsID("attachData"), xsHostData(fssl->skt));
		xsResult = xsCall1(fssl->ssl, xsID("flush"), fssl->socket);
		err = xsToBoolean(xsResult) ? kFskErrNone: kFskErrNoData;
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost();
	return err;
}

int
FskSSLGetBytesAvailable(void *a)
{
	FskSSL *fssl = a;
	int ret = 0;

	if (fssl->skt == NULL)
		return -1;

	xsBeginHost(fssl->vm);
	xsTry {
		xsResult = xsGet(fssl->ssl, xsID("bytesAvailable"));
		ret = xsToInteger(xsResult);
	} xsCatch {
		ret = -1;
	}
	xsEndHost();
	return ret;
}

FskErr
FskSSLClose(void *a)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	if (fssl->skt == NULL)
		return kFskErrOperationFailed;

	xsBeginHost(fssl->vm);
	xsTry {
		xsResult = xsCall1(fssl->ssl, xsID("close"), fssl->socket);
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost();
	return err;
}

FskErr
FskSSLInitialize(char *CA_List)
{
	char *calistpath;
	FskErr err = kFskErrNone;

#if TARGET_OS_ANDROID
	unpackAndroid();
	calistpath = FskStrDoCat(gAndroidCallbacks->getStaticDataDirCB(), CA_List);
#else
	calistpath = FskStrDoCat(FskEnvironmentGet("applicationPath"), CA_List);
#endif

	err = makeSSLRootVM(calistpath);
	FskMemPtrDispose(calistpath);
	return err;
}

void FskSSLTerminate() {
	disposeSSLRootVM();
}


