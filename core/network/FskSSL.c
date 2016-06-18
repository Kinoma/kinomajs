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
#if CLOSED_SSL
#define __FSKNETUTILS_PRIV__
#define __FSKECMASCRIPT_PRIV__
#endif
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
#ifdef KPR_CONFIG
#include "kprShell.h"
#endif

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

#if OPEN_SSL

static SSL_CTX *gSSLContext = NULL;
static char *gSSLPassword = NULL;
static SSL_CTX *FskOpenSSLInitialize(const char *calistpath);

void doSSLError(char *info) {
	unsigned long err;

	while (0 != (err = ERR_get_error())) {
		fprintf(stderr, "%s: %d - %s\n", info, err, ERR_error_string(err, NULL));
	}
}

SSL_CTX *FskSSLGetContext() {
	return gSSLContext;
}

static int passwordCallback(char *buf, int size, int rwflag, void *password) {
	FskStrNCopy(buf, gSSLPassword, size);
	buf[size -1] = '\0';
	return (FskStrLen(buf));
}

// ---------------------------------------------------------------------
SInt32 FskStrEndsWith(const char *s1, const char *s2)
{	
	int l1, l2;
	char *p;
	if (NULL == s1 && NULL == s2)
		return 1;

	l1 = FskStrLen(s1);
	l2 = FskStrLen(s2);

	if (l2 > l1)
		return 0;

	p = s1 + (l1 - l2);
	fprintf(stderr, "checking %s [%s] ends with %s - return %d\n", s1, p, s2, FskStrCompare(p, s2));
	return FskStrCompare(p, s2);
}


Boolean FskSSLCheckServerCert(SSL *ssl, char *hostname) {
	long err;
	X509 *peer;
	char peer_CN[256];
	err = SSL_get_verify_result(ssl);
#if 0
	if ((err != X509_V_OK)
		&& (err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
		&& (err != X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)) {
		ERR_print_errors_fp(stderr);
fprintf(stderr, "[%s] cert for %s didn't verify %d %s\n", threadTag(FskThreadGetCurrent()), hostname, err, X509_verify_cert_error_string(err));
#else
	if (err != X509_V_OK) {
		if ((NULL != FskStrStr(hostname, "google.com")) 
			|| (NULL != FskStrStr(hostname, "googleapis.com")) 
			|| (NULL != FskStrStr(hostname, "twitter.com"))
			|| (NULL != FskStrStr(hostname, "yammer.com"))
			|| (NULL != FskStrStr(hostname, "facebook.com"))
			|| (NULL != FskStrStr(hostname, "foursquare.com"))
			|| (NULL != FskStrStr(hostname, "akamaihd.net"))
			|| (NULL != FskStrStr(hostname, "fbcdn.net"))
			|| (NULL != FskStrStr(hostname, "radiotime.com"))
			|| (NULL != FskStrStr(hostname, "s3.amazonaws.com"))
			|| (NULL != FskStrStr(hostname, "orb.com"))) {
			if ((err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
				&& (err != X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)) {
				return false;
			}
			myprintf((stderr, "b) cert didn't verify because %d %s\n", err, X509_verify_cert_error_string(err)));
			myprintf((stderr, " but since it's %s we'll let it through\n", hostname));
		}
		else {
#endif
			return false;
		}
	}

	peer = SSL_get_peer_certificate(ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName,
						peer_CN, 256);
//fprintf(stderr, "comparing peer_CN %s with hostname %s\n", peer_CN, hostname);
	if (FskStrCompareCaseInsensitive(peer_CN, hostname)) {
		int pos, L, subL;
		char *match = peer_CN + 1;

		subL = FskStrLen(match);
		L = FskStrLen(hostname);
		if (peer_CN[0] == '*') {
			pos = L - subL;
			if (0 == FskStrCompareCaseInsensitive(match, hostname + pos)) {
//fprintf(stderr, "Matched wildcard %s and %s\n", match, hostname + pos);
				return true;
			}
		}

		if (	(FskStrEndsWith(match, "akamaihd.net")
				&& FskStrEndsWith(hostname, "akamai.net")) 
			||	(FskStrEndsWith(match, "akamai.net")
				&& FskStrEndsWith(hostname, "akamaihd.net")) ) {
			return true;
		}
			
	

		myprintf((stderr, "cert common name %s and host %s don't match\n", peer_CN, hostname));
		return false;
	}

	return true;
}

//SSL_CTX *FskSSLInitialize(char *keyfile, char *password)
static SSL_CTX *FskOpenSSLInitialize(const char *calistpath)
{
	SSL_METHOD	*method;
	SSL_CTX		*context;

	if (gSSLContext) {
		return gSSLContext;
	}
	else {
		SSL_library_init();
		SSL_load_error_strings();			// not necessary, but useful
	}

	method = SSLv23_method();
	context = SSL_CTX_new(method);

#if 0
	if (FskStrLen(keyfile) > 0) {
		keyfile = FskEnvironmentDoApply(FskStrDoCopy(keyfile));
		if (!SSL_CTX_use_certificate_chain_file(context, keyfile))
			doSSLError("Can't read certificate file");
		fprintf(stderr, "keyfile is %s\n", keyfile);

		if (FskStrLen(password) > 0) {
			gSSLPassword = FskStrDoCopy(password);
			SSL_CTX_set_default_passwd_cb(context, passwordCallback);
			if (!SSL_CTX_use_PrivateKey_file(context, keyfile, SSL_FILETYPE_PEM))
				doSSLError( "Can't read private keyfile");
		}

		FskMemPtrDispose(keyfile);
	}
#endif

#if TARGET_OS_WIN32
	{
	// try to make the path 8.3 safe to avoid nightmares with multibyte character paths, etc.
	UInt16 *nativePath;

	if (kFskErrNone == FskFilePathToNative(calistpath, (char **)&nativePath)) {
		DWORD shortLen;

		shortLen = GetShortPathNameW(nativePath, NULL, 0);
		if (0 != shortLen) {
			UInt16 *eightDotThree;

			if (kFskErrNone == FskMemPtrNewClear(shortLen * 2, (FskMemPtr *)&eightDotThree)) {
				if (0 != GetShortPathNameW(nativePath, eightDotThree, shortLen)) {
					char *utf8;
					UInt32 utf8Len;

					if (kFskErrNone == FskTextUnicode16LEToUTF8(eightDotThree, shortLen * 2, &utf8, &utf8Len)) {
						char *enc;

						if (kFskErrNone == FskTextToPlatform(utf8, utf8Len, &enc, NULL)) {
							FskMemPtrDispose(calistpath);
							calistpath = enc;
						}
						FskMemPtrDispose(utf8);
					}
				}
				FskMemPtrDispose(eightDotThree);
			}
		}
		FskMemPtrDispose(nativePath);
	}
	}
#endif

	if (!(SSL_CTX_load_verify_locations(context, calistpath, 0))) {
		doSSLError("Can't read default CA list");
	}

	SSL_CTX_set_verify_depth(context, 0);

//	SSL_CTX_set_verify(context, SSL_VERIFY_PEER, 0);

	gSSLContext = context;

	return context;
}

static void FskOpenSSLTerminate() {
	// destroy gSSLContext
	SSL_CTX_free(gSSLContext);
	FskMemPtrDispose(gSSLPassword);
	gSSLContext = NULL;
	gSSLPassword = NULL;
}

#endif /* OPEN_SSL */

#if CLOSED_SSL
#include "FskECMAScript.h"

#ifdef KPR_CONFIG
typedef struct {
	xsMachine* the;
} FskECMAScriptRecord, *FskECMAScript;
#else
static FskECMAScript gSSLVM = NULL;
extern FskErr loadGrammar(const char *xsbPath, xsGrammar *grammar);
#endif

typedef struct {
	FskECMAScript vm;
	xsSlot ssl, socket;
	FskNetSocketCreatedCallback socketCallback;
	void *callbackData;
	FskSocket skt;
	FskTimeCallBack timer;
} FskSSL;

static xsAllocation anAllocation = {
	200*1024,	// was 1100000
	32 * 1024,
	8192,		// was 70000
	4096,
	2048,
	4000,		// was 16000
	373		// was 1993
};

#if !FSK_EMBED
#include "FskCore.xs.h"
#endif

#if FSK_EMBED
extern void FskExtensionsEmbedLoad(char *vmName);
extern void FskExtensionsEmbedUnload(char *vmName);
extern void FskExtensionsEmbedGrammar(char *vmName, char **xsbName, xsGrammar **grammar);
#endif

#define SSL_VMNAME	"sslroot"

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
#ifdef KPR_CONFIG
#else
	if (gSSLVM != NULL) {
		if (gSSLVM->the != NULL)
			xsDeleteMachine(gSSLVM->the);
		FskMemPtrDisposeAt(&gSSLVM);
	}
#endif
}

static FskErr
makeSSLRootVM(const char *calistpath)
{
#ifdef KPR_CONFIG
	FskErr err = kFskErrNone;

	xsBeginHost(gShell->root);
	xsTry {
		xsCall1_noResult(xsGet(xsGlobal, xsID("FskSSL")), xsID("loadRootCerts"), xsString((xsStringValue)calistpath));
	} xsCatch {
		err = kFskErrOperationFailed;
	}
	xsEndHost(gShell->root);
	return err;
#else
	char *rootPath, *xsbPath, *xsbName;
	xsGrammar *grammar;
	FskErr err;

	if ((err = FskMemPtrNewClear(sizeof(FskECMAScriptRecord), &gSSLVM)) != kFskErrNone)
		return err;

#if FSK_EMBED
	FskExtensionsEmbedLoad(SSL_VMNAME);
	FskExtensionsEmbedGrammar(SSL_VMNAME, &xsbName, &grammar);
#else
	xsbName = "FskCore.xsb";
	grammar = &FskCoreGrammar;
#endif
	rootPath = FskEnvironmentDoApply(FskStrDoCopy("[applicationPath]"));
	xsbPath = FskStrDoCat(rootPath, xsbName);
	FskMemPtrDispose(rootPath);

	grammar->name = (xsStringValue)SSL_VMNAME;
	if ((err = loadGrammar(xsbPath, grammar)) == kFskErrNone) {
		if ((gSSLVM->the = xsNewMachine(&anAllocation, grammar, gSSLVM)) == NULL)
			err = kFskErrMemFull;
		FskMemPtrDispose(grammar->symbols);
		FskMemPtrDispose(grammar->code);
	}
	FskMemPtrDispose(xsbPath);
	BAIL_IF_ERR(err);

	xsBeginHost(gSSLVM->the);
	xsTry {
#if !FSK_EMBED
		/* load Crypt and FskSSL extensions which are only needed for Fsk SSL */
		xsCall3_noResult(xsGet(xsGlobal, xsID("System")), xsID("loadExtension"), xsString("Crypt"), xsString("[applicationPath]"), xsInteger(0));
		xsCall3_noResult(xsGet(xsGlobal, xsID("System")), xsID("loadExtension"), xsString("FskSSL"), xsString("[applicationPath]"), xsInteger(1));
#endif
		xsCall1_noResult(xsGet(xsGlobal, xsID("FskSSL")), xsID("loadRootCerts"), xsString((xsStringValue)calistpath));
	} xsCatch {
		err = kFskErrOperationFailed;
	}
	xsEndHost(gSSLVM->the);

	if (err == kFskErrNone)
		xsShareMachine(gSSLVM->the);

bail:
	if (err != kFskErrNone)
		disposeSSLRootVM();
	return err;
#endif
}

static FskErr
newSSLVM(FskECMAScript *vmp)
{
	FskECMAScript vm;
	FskErr err;

#ifdef KPR_CONFIG
	if ((err = FskMemPtrNewClear(sizeof(FskECMAScriptRecord), &vm)) != kFskErrNone)
		return err;
	if ((vm->the = xsAliasMachine(&anAllocation, gShell->root, "SSL", vm)) == NULL) {
		FskMemPtrDispose(vm);
		return kFskErrMemFull;
	}
#else
	if (gSSLVM == NULL)
		return kFskErrUnimplemented;

	if ((err = FskMemPtrNewFromData(sizeof(FskECMAScriptRecord), gSSLVM, &vm)) != kFskErrNone)
		return err;
	vm->the = NULL;
	vm->context = NULL;
	vm->refcon = NULL;
	vm->libraries = NULL;
	vm->name = FskStrDoCopy("SSL");
	vm->thread = FskThreadGetCurrent();
	vm->id = (UInt32)vm;

	if ((vm->the = xsAliasMachine(&anAllocation, gSSLVM->the, vm->name, vm)) == NULL) {
		FskMemPtrDispose(vm);
		return kFskErrMemFull;
	}
#endif
	*vmp = vm;
	return err;
}

static void
disposeSSLVM(FskECMAScript vm)
{
	if (vm->the != NULL)
		xsDeleteMachine(vm->the);
#ifdef KPR_CONFIG
#else
	if (vm->name != NULL)
		FskMemPtrDispose(vm->name);
#endif
	FskMemPtrDispose(vm);
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
	FskSSL *fssl;
	FskErr err;

	if ((err = FskMemPtrNewClear(sizeof(FskSSL), (FskMemPtr*)(void*)&fssl)) != kFskErrNone)
		return err;
	if ((err = newSSLVM(&fssl->vm)) != kFskErrNone) {
		FskMemPtrDispose(fssl);
		return err;
	}

	xsBeginHost(fssl->vm->the);
	xsTry {
		const char *prStr;
		xsVars(3);
		/* construct the options */
		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		if (blocking)
			xsSet(xsVar(0), xsID("blocking"), xsTrue);
		if (flags & kConnectFlagsSynchronous)
			xsSet(xsVar(0), xsID("synchronous"), xsTrue);
		switch (priority) {
		default:
		case kFskNetSocketLowestPriority: prStr = "lowest"; break;
		case kFskNetSocketLowPriority: prStr = "low"; break;
		case kFskNetSocketMediumPriority: prStr = "medium"; break;
		case kFskNetSocketHighPriority: prStr = "high"; break;
		case kFskNetSocketHighestPriority: prStr = "highest"; break;
		}
		(void)xsSet(xsVar(0), xsID("priority"), xsString((xsStringValue)prStr));
		(void)xsSet(xsVar(0), xsID("raw"), xsTrue);
		xsVar(1) = xsNew3(xsGet(xsGlobal, xsID("Stream")), xsID("Socket"), xsString((xsStringValue)host), xsInteger(port), xsVar(0));
		fssl->socket = xsVar(1); xsRemember(fssl->socket);
		xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
		xsSet(xsVar(2), xsID("server_name"), xsString((char *) host));
		xsVar(1) = xsNew1(xsGet(xsGlobal, xsID("FskSSL")), xsID("Session"), xsVar(2));
		fssl->ssl = xsVar(1); xsRemember(fssl->ssl);
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost(fssl->vm->the);

	if (err == kFskErrNone) {
		if (fsslp != NULL)
			*fsslp = fssl;
	}
	else {
		disposeSSLVM(fssl->vm);
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
	if ((err = newSSLVM(&fssl->vm)) != kFskErrNone) {
		FskMemPtrDispose(fssl);
		return err;
	}

	xsBeginHost(fssl->vm->the);
	xsTry {
		xsVars(2);
		xsVar(0) = xsNew1(xsGet(xsGlobal, xsID("Stream")), xsID("Socket"), xsHostData(skt));
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		(void)xsSet(xsVar(1), xsID("raw"), xsTrue);
		xsCall1_noResult(xsVar(0), xsID("setProperties"), xsVar(1));
		fssl->socket = xsVar(0); xsRemember(fssl->socket);
		xsVar(0) = xsNew0(xsGet(xsGlobal, xsID("FskSSL")), xsID("Session"));
		fssl->ssl = xsVar(0); xsRemember(fssl->ssl);
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost(fssl->vm->the);

	if (err == kFskErrNone) {
		if (fsslp != NULL)
			*fsslp = fssl;
	}
	else {
		disposeSSLVM(fssl->vm);
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
	xsBeginHost(fssl->vm->the);
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
	xsEndHost(fssl->vm->the);

	disposeSSLVM(fssl->vm);
	FskMemPtrDispose(a);
}

FskErr
FskSSLLoadCerts(void *a, FskSocketCertificateRecord *cert)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	xsBeginHost(fssl->vm->the);

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

	xsEndHost(fssl->vm->the);
	return err;
}

FskErr
FskSSLHandshake(void *a, FskNetSocketCreatedCallback callback, void *refCon, Boolean initiate, int timeout)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	fssl->socketCallback = callback;
	fssl->callbackData = refCon;

	xsBeginHost(fssl->vm->the);

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

	xsEndHost(fssl->vm->the);
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

	xsBeginHost(fssl->vm->the);
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
	xsEndHost(fssl->vm->the);
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

	xsBeginHost(fssl->vm->the);
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
	xsEndHost(fssl->vm->the);
	return err;
}

FskErr
FskSSLFlush(void *a)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	if (fssl->skt == NULL)
		return kFskErrOperationFailed;

	xsBeginHost(fssl->vm->the);
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
	xsEndHost(fssl->vm->the);
	return err;
}

int
FskSSLGetBytesAvailable(void *a)
{
	FskSSL *fssl = a;
	int ret = 0;

	if (fssl->skt == NULL)
		return -1;

	xsBeginHost(fssl->vm->the);
	xsTry {
		xsResult = xsGet(fssl->ssl, xsID("bytesAvailable"));
		ret = xsToInteger(xsResult);
	} xsCatch {
		ret = -1;
	}
	xsEndHost(fssl->vm->the);
	return ret;
}

FskErr
FskSSLClose(void *a)
{
	FskSSL *fssl = a;
	FskErr err = kFskErrNone;

	if (fssl->skt == NULL)
		return kFskErrOperationFailed;

	xsBeginHost(fssl->vm->the);
	xsTry {
		xsResult = xsCall1(fssl->ssl, xsID("close"), fssl->socket);
	} xsCatch {
		if (xsHas(xsException, xsID("code")))
			err = xsToInteger(xsGet(xsException, xsID("code")));
		if (err == kFskErrNone)
			err = kFskErrOperationFailed;
	}
	xsEndHost(fssl->vm->the);
	return err;
}
#endif /* CLOSED_SSL */

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

#if OPEN_SSL
	(void)FskOpenSSLInitialize(calistpath);
#endif
#if CLOSED_SSL
	err = makeSSLRootVM(calistpath);
#endif
	FskMemPtrDispose(calistpath);
	return err;
}

void FskSSLTerminate() {
#if OPEN_SSL
	FskOpenSSLTerminate();
#endif
#if CLOSED_SSL
	disposeSSLRootVM();
#endif
}

#if CLOSED_SSL
#include "FskMain.h"

/*
	Timer
*/
static void xs_Timer_callback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);

typedef struct {
	FskTimeCallBack	timer;
	xsSlot			obj;
	FskECMAScript	vm;
	Boolean			repeating;
	UInt32			interval;
	FskTimeRecord	scheduleTime;
} xsNativeTimerRecord, *xsNativeTimer;

void xs_Timer(xsMachine *the)
{
	FskErr err;
	xsNativeTimer nt;

	err = FskMemPtrNew(sizeof(xsNativeTimerRecord), &nt);
	if (err) {
		FskMainDoQuit(err);
		xsResult = xsNew1(xsGet(xsGet(xsGlobal, xsID("Fsk")), xsID("Error")), xsID("Native"), xsInteger(err));
		xsThrow(xsResult);
	}
	xsSetHostData(xsThis, nt);
	nt->obj = xsThis;
	nt->vm = (FskECMAScript)xsGetContext(the);

	if ((0 == xsToInteger(xsArgc)) || !xsTest(xsArg(0)))
		FskTimeCallbackNew(&nt->timer);
	else
		FskTimeCallbackUINew(&nt->timer);
}

void xs_Timer_destuctor(void *hostData)
{
	if (hostData) {
		xsNativeTimer nt = (xsNativeTimer)hostData;
		FskTimeCallbackDispose(nt->timer);
		FskMemPtrDispose(nt);
	}
}

void xs_Timer_close(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	FskTimeCallbackDispose(nt->timer);
	nt->timer = NULL;
}

void xs_Timer_schedule(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	FskTimeRecord when;

	if (NULL == nt->timer) return;

	nt->repeating = false;
	FskTimeGetNow(&when);
	nt->scheduleTime = when;
	FskTimeAddMS(&when, xsToInteger(xsArg(0)));

	FskTimeCallbackSet(nt->timer, &when, xs_Timer_callback, nt);
}

void xs_Timer_scheduleRepeating(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	FskTimeRecord when;

	if (NULL == nt->timer) return;

	nt->repeating = true;
	nt->interval = xsToInteger(xsArg(0));
	FskTimeGetNow(&when);
	nt->scheduleTime = when;
	if (nt->interval) {
		FskTimeAddMS(&when, nt->interval);
		FskTimeCallbackSet(nt->timer, &when, xs_Timer_callback, nt);
	}
	else
		FskTimeCallbackScheduleNextRun(nt->timer, xs_Timer_callback, nt);
}

void xs_Timer_cancel(xsMachine *the)
{
	xsNativeTimer nt = (xsNativeTimer)xsGetHostData(xsThis);
	if (NULL == nt->timer) return;
	FskTimeCallbackRemove(nt->timer);
	nt->repeating = false;
}

void xs_Timer_callback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param)
{
	xsNativeTimer nt = (xsNativeTimer)param;
	xsMachine *the = nt->vm->the;
	FskTimeRecord delta;

	xsBeginHost(the); {
		xsTry {
			FskTimeGetNow(&delta);
			FskTimeSub(&nt->scheduleTime, &delta);
			xsCall1_noResult(nt->obj, xsID("onCallback"), xsInteger(FskTimeInMS(&delta)));
		}
		xsCatch {		// try/catch so that if callback throws an error we will still do the repeat
		}

		if (nt->timer && nt->repeating) {
			if (nt->interval) {
				FskTimeRecord when = *time;		// this will give us a non-drifting clock
				FskTimeAddMS(&when, nt->interval);
				FskTimeCallbackSet(nt->timer, &when, xs_Timer_callback, nt);
			}
			else
				FskTimeCallbackScheduleNextRun(nt->timer, xs_Timer_callback, nt);
		}
	}
	xsEndHost(the);
}
#endif
