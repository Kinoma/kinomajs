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
#define __FSKFILES_PRIV__
#define __FSKFSFILES_PRIV__

#include "FskFS.h"
#include "FskTextConvert.h"
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#if TARGET_OS_IPHONE
#include "FskCocoaSupportPhone.h"
#else
#include "FskCocoaSupport.h"
/* for the volume iterator */
#import <IOKit/storage/IOMedia.h>
#import <IOKit/storage/IOCDMedia.h>
#import <IOKit/storage/IODVDMedia.h>
#import <CoreFoundation/CoreFoundation.h>
#endif

static FskErr FskFSFileFilter(const char *fullPath, SInt32 *priority)
{
        *priority = 0;
        return kFskErrNone;
}

FskFileDispatchTableRecord gFSDispatch = {
	0,
	FskFSFileFilter,

	(FskFileOpenProc)FskFSFileOpen,
	(FskFileCloseProc)FskFSFileClose,
	(FskFileGetSizeProc)FskFSFileGetSize,
	(FskFileSetSizeProc)FskFSFileSetSize,
	(FskFileSetPositionProc)FskFSFileSetPosition,
	(FskFileGetPositionProc)FskFSFileGetPosition,
	(FskFileReadProc)FskFSFileRead,
	(FskFileWriteProc)FskFSFileWrite,
	(FskFileGetFileInfoProc)FskFSFileGetFileInfo,
	(FskFileSetFileInfoProc)FskFSFileSetFileInfo,
	(FskFileGetThumbnailProc)NULL,
	(FskFileCreateProc)FskFSFileCreate,
	(FskFileDeleteProc)FskFSFileDelete,
	(FskFileRenameProc)FskFSFileRename,
	(FskFilePathToNativeProc)FskFSFilePathToNative,

	(FskFileCreateDirectoryProc)FskFSFileCreateDirectory,
	(FskFileDeleteDirectoryProc)FskFSFileDeleteDirectory,
	(FskFileRenameDirectoryProc)FskFSFileRenameDirectory,
	(FskDirectoryIteratorNewProc)FskFSDirectoryIteratorNew,
	(FskDirectoryIteratorDisposeProc)FskFSDirectoryIteratorDispose,
	(FskDirectoryIteratorGetNextProc)FskFSDirectoryIteratorGetNext,
	(FskDirectoryGetSpecialPathProc)FskFSDirectoryGetSpecialPath,

	(FskVolumeIteratorNewProc)FskFSVolumeIteratorNew,
	(FskVolumeIteratorDisposeProc)FskFSVolumeIteratorDispose,
	(FskVolumeIteratorGetNextProc)FskFSVolumeIteratorGetNext,

#if TARGET_OS_IPHONE
	(FskVolumeNotifierNewProc)NULL,
	(FskVolumeNotifierDisposeProc)NULL,
	(FskVolumeGetInfoProc)NULL,
	(FskVolumeGetInfoFromPathProc)NULL,
	(FskVolumeGetDeviceInfoProc)NULL,
	(FskVolumeGetIDProc)NULL,
	(FskVolumeEjectProc)NULL,
#else
	(FskVolumeNotifierNewProc)FskFSVolumeNotifierNew,
	(FskVolumeNotifierDisposeProc)FskFSVolumeNotifierDispose,
	(FskVolumeGetInfoProc)FskFSVolumeGetInfo,
	(FskVolumeGetInfoFromPathProc)FskFSVolumeGetInfoFromPath,		// vol get info by path
	(FskVolumeGetDeviceInfoProc)FskFSVolumeGetDeviceInfo,
	(FskVolumeGetIDProc)FskFSVolumeGetID,
	(FskVolumeEjectProc)FskFSVolumeEject,
#endif

	(FskFileMapProc)FskFSFileMap,
	(FskFileDisposeMapProc)FskFSFileDisposeMap,

#if TARGET_OS_IPHONE
	(FskFileChooseProc)NULL,
	(FskFileChooseSaveProc)NULL,
	(FskDirectoryChooseProc)NULL,
#else
	(FskFileChooseProc)FskFSFileChoose,
	(FskFileChooseSaveProc)FskFSFileChooseSave,
	(FskDirectoryChooseProc)FskFSDirectoryChoose,
#endif

	(FskFileTerminateProc)FskFSFileTerminate,

	(FskDirectoryChangeNotifierNewProc)FskFSDirectoryChangeNotifierNew,
	(FskDirectoryChangeNotifierDisposeProc)FskFSDirectoryChangeNotifierDispose,

	(FskFileInitializeProc)NULL,
	(FskFileResolveLinkProc)FskFSFileResolveLink,
};

static struct statfs *gStatFS = NULL;
static int gNumStatFS = 0;

typedef struct FskFSDirectoryChangeNotifierRecord FskFSDirectoryChangeNotifierRecord;
typedef struct FskFSDirectoryChangeNotifierRecord *FskFSDirectoryChangeNotifier;

struct FskFSDirectoryChangeNotifierRecord {
	FskDirectoryChangeNotifierRecord		base;
	FskDirectoryChangeNotifierCallbackProc	callback;
	void									*refCon;
	char									*path;
	int										dirFD;
	int										kq;
	CFFileDescriptorRef dirKQRef;
};

static void KQCallback(CFFileDescriptorRef kqRef, CFOptionFlags callBackTypes, void *info);

extern FskFileDispatchTableRecord gFSDispatch;


/*
 * Volume iterator
 */

#if __unused__ /* remain here just for reference. Should work on 10.7 or later */
static Boolean getVolumeProperty(const char *path, CFStringRef key, void *value)
{
	FskErr err;
	char *url;
	CFURLRef urlRef;

	if ((err = FskMemPtrNew(sizeof("file://") + FskStrLen(path), (FskMemPtr *)&url)) != kFskErrNone)
		return false;
	FskStrCopy(url, "file://");
	FskStrCat(url, path);
	urlRef = CFURLCreateWithBytes(NULL, (UInt8 *)url, FskStrLen(url), kCFStringEncodingUTF8, NULL);
	FskMemPtrDispose(url);
	return CFURLCopyResourcePropertyForKey(urlRef, key, value, NULL);
}

static char *getVolumeName(const struct statfs *fs)
{
	CFStringRef nameRef;
	char *name;

	if (getVolumeProperty(fs->f_mntonname, kCFURLVolumeNameKey, (void *)&nameRef) && nameRef != NULL) {
		int len = CFStringGetLength(nameRef);
		if (FskMemPtrNew(len + 1, (FskMemPtr *)&name) != kFskErrNone)
			return NULL;
		CFStringGetCString(nameRef, name, len + 1, kCFStringEncodingUTF8);
		CFRelease(nameRef);
		return name;
	}
	else
		return NULL;
}

static Boolean volumeIsRemovable(const char *path)
{
	CFBooleanRef removableRef;

	if (getVolumeProperty(fs->f_mntonname, kCFURLVolumeIsRemovableKey, &removableRef) && removableRef != NULL)
		return CFBooleanGetValue(removableRef);
	else
		return false;
}
#endif	/* __unused__ */

static FskErr updateStatFS()
{
	FskErr err;

	if (gStatFS != NULL) {
		FskMemPtrDispose(gStatFS);
		gStatFS = NULL;
	}
	if ((gNumStatFS = getfsstat(NULL, 0, MNT_NOWAIT)) < 0)
		return kFskErrOperationFailed;
	if ((err = FskMemPtrNew(sizeof(struct statfs) * gNumStatFS, (FskMemPtr *)&gStatFS)) != kFskErrNone)
		return err;
	if ((gNumStatFS = getfsstat(gStatFS, sizeof(struct statfs) * gNumStatFS, MNT_NOWAIT)) < 0) {
		FskMemPtrDispose(gStatFS);
		gStatFS = NULL;
		return kFskErrOperationFailed;
	}
	return kFskErrNone;
}


FskErr FskFSVolumeIteratorNew(FskFSVolumeIterator *volIt)
{
	FskErr err = kFskErrNone;
	FskFSVolumeIterator vi;

	err = FskMemPtrNewClear(sizeof(FskFSVolumeIteratorRecord), (FskMemPtr *)&vi);
	BAIL_IF_ERR(err);

	vi->volumeIndex = 0;
	updateStatFS();
	*volIt = vi;

bail:
	return err;
}

FskErr FskFSVolumeIteratorDispose(FskFSVolumeIterator volIt)
{
	return FskMemPtrDispose(volIt);
}

FskErr FskFSVolumeIteratorGetNext(FskFSVolumeIterator volIt, UInt32 *id, char **pathOut, char **nameOut)
{
	FskErr			err = kFskErrNone;
	struct statfs *fs;

	if (volIt->volumeIndex >= (UInt32)gNumStatFS)
		return kFskErrIteratorComplete;
	fs = &gStatFS[volIt->volumeIndex];
	if (pathOut)
		*pathOut = FskStrDoCopy(fs->f_mntonname);
	if (nameOut)
		*nameOut = FskStrDoCopy(FskCocoaDisplayNameAtPath(fs->f_mntonname));
	if (id)
		*id = volIt->volumeIndex;
	volIt->volumeIndex++;
	return( err );
}

FskErr FskFSDirectoryGetSpecialPath(UInt32 type, const Boolean create, const char *volumeName, char **fullPath)
{
	*fullPath = FskCocoaGetSpecialPath(type, create);
	return *fullPath == NULL ? kFskErrInvalidParameter : kFskErrNone;
}

#if !TARGET_OS_IPHONE
FskErr FskFSFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files)
{
	FskCocoaFileChoose(types, prompt, allowMultiple, initialPath, files);
	
	return kFskErrNone;
}

FskErr FskFSFileChooseSave(const char *defaultName, const char *prompt, const char *initialDirectory, char **file)
{
	FskCocoaFileSaveChoose(defaultName, prompt, initialDirectory, file);
	
	return kFskErrNone;
}

FskErr FskFSDirectoryChoose(const char *prompt, const char *initialPath, char **dir)
{
	FskCocoaFileDirectoryChoose(prompt, initialPath, dir);
	
	return kFskErrNone;
}
#endif /* !TARGET_OS_IPHONE */

#if !TARGET_OS_IPHONE
/*
	Volume notifier.
*/

/* notifier hook to update the current statfs */
static void volumeCallbackHook(UInt32 status, const char *path, void *refCon)
{
	FskFSVolumeNotifier ntf = refCon;
	UInt32 volumeID;

	if (status == kFskVolumeHello)
		updateStatFS();
	if (kFskErrNone == FskFSVolumeGetID(path, &volumeID))
        (void)(*ntf->callback)(status, volumeID, ntf->refCon);
	if (status == kFskVolumeBye)
		updateStatFS();		/* update after the callback to keep alive the volumeID */
}

FskErr FskFSVolumeNotifierNew(FskVolumeNotifierCallbackProc callback, void *refCon, FskFSVolumeNotifier *volNtfOut)
{
	FskErr err;
	FskFSVolumeNotifier volNtf;

	if ((err = FskMemPtrNewClear(sizeof(FskFSVolumeNotifierRecord), (FskMemPtr *)&volNtf)) != kFskErrNone)
		return err;
	volNtf->callback = callback;
	volNtf->refCon = refCon;
	volNtf->dispatch.dispatch = &gFSDispatch;	// necessary to call FskFSVolumeNotifierDispose
	volNtf->dispatch.refcon = NULL;
	volNtf->instance = FskCocoaVolumeNotifierNew(volumeCallbackHook, volNtf);
	*volNtfOut = volNtf;
	return kFskErrNone;
}

FskErr FskFSVolumeNotifierDispose(FskFSVolumeNotifier volNtf)
{
	FskCocoaVolumeNotifierDispose(volNtf->instance);
	return kFskErrNone;
}

FskErr FskFSVolumeGetInfo(UInt32 volumeID, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	FskErr err = kFskErrNone;
	struct statfs *fs;

	if (volumeID >= (UInt32)gNumStatFS)
		return kFskErrInvalidParameter;
	fs = &gStatFS[volumeID];
	if (pathOut)
		*pathOut = FskStrDoCopy(fs->f_mntonname);
	if (nameOut)
		*nameOut = FskStrDoCopy(FskCocoaDisplayNameAtPath(fs->f_mntonname));
	if (capacity)
		*capacity = fs->f_blocks * fs->f_bsize;
	if (freeSpace)
		*freeSpace = fs->f_bavail * fs->f_bsize;
	if (isRemovable)
		*isRemovable = FskCocoaIsRemovable(fs->f_mntonname);
	if (volumeType) {
		*volumeType = kFskVolumeTypeUnknown;
		if (!(fs->f_flags & MNT_LOCAL))
			*volumeType = kFskVolumeTypeNetwork;
		else if (FskStrCompare(fs->f_fstypename, "hfs") == 0 || FskStrCompare(fs->f_fstypename, "ufs") == 0)
			*volumeType = kFskVolumeTypeFixed;
		else if (FskStrCompare(fs->f_fstypename, "devfs") == 0)
			*volumeType = kFskVolumeTypeUnknown;
		else {
			char *product;
			if ((err = FskFSVolumeGetDeviceInfo(volumeID, NULL, &product, NULL, NULL)) != kFskErrNone)
				return err;
			if (product == NULL)
				*volumeType = kFskVolumeTypeUnknown;
			else if (FskStrStr(product, "MS") || FskStrStr(product, "WM-MS"))
				*volumeType = kFskVolumeTypeMemoryStick;
			else if (FskStrStr(product, "MMC"))
				*volumeType = kFskVolumeTypeMMC;
			else if (FskStrStr(product, "CF"))
				*volumeType = kFskVolumeTypeCompactFlash;
			else if (FskStrStr(product, "SM"))
				*volumeType = kFskVolumeTypeSmartMedia;
			else if (FskStrStr(product, "SD"))
				*volumeType = kFskVolumeTypeSDMemory;
			else
				*volumeType = kFskVolumeTypeUnknown;
		}
	}
	return err;
}

FskErr FskFSVolumeGetInfoFromPath(const char *pathIn, char **pathOut, char **nameOut, UInt32 *volumeType, Boolean *isRemovable, FskInt64 *capacity, FskInt64 *freeSpace)
{
	int i;
	FskErr	err = kFskErrOperationFailed;

	if (gStatFS == NULL)
		updateStatFS();

	for (i = 0; i < gNumStatFS; i++) {
		if (FskStrCompare(gStatFS[i].f_mntonname, pathIn) == 0) {
			err = FskFSVolumeGetInfo(i, pathOut, nameOut, volumeType, isRemovable, capacity, freeSpace);
			break;
		}
	}

	return err;
}

static Boolean
find_and_set_info(CFMutableDictionaryRef properties, CFStringRef key, char **info)
{
	CFStringRef cfInfo;
	Boolean findInfo = false;

	cfInfo = CFDictionaryGetValue(properties, key);
	if (cfInfo) {
		if (info) {
			char strInfo[256];

			CFStringGetCString(cfInfo, strInfo, sizeof(strInfo), kCFStringEncodingASCII);
			*info = FskStrDoCopy(strInfo);
		}
		findInfo = true;
	}

	return findInfo;
}

FskErr FskFSVolumeGetDeviceInfo(UInt32 volumeID, char **vendor, char **product, char **revision, char **vendorSpecific)
{
	FskErr err = kFskErrNone;

	mach_port_t				masterPort	= MACH_PORT_NULL;
	io_service_t			service;
	io_iterator_t			iterator = 0;
	io_object_t				obj;
	char					bsdNode[256];
	CFMutableDictionaryRef properties = NULL;
	Boolean isFound = false;
	char *p;

	if (vendor)
		*vendor = NULL;
	if (product)
		*product = NULL;
	if (revision)
		*revision = NULL;
	if (vendorSpecific)
		*vendorSpecific = NULL;

	if (volumeID >= (UInt32)gNumStatFS)
		return kFskErrInvalidParameter;
	p = gStatFS[volumeID].f_mntfromname;
	if (FskStrStr(p, "/dev/") == p)
		p += 5;
	FskStrCopy(bsdNode, p);

	err = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (err != kIOReturnSuccess) {
		BAIL(kFskErrUnknown);
	}

	err = IOServiceGetMatchingServices(masterPort, IOBSDNameMatching(masterPort, 0, bsdNode),
									   &iterator);
	if (err != kIOReturnSuccess) {
		BAIL(kFskErrUnknown);
	}

	// There is a 1:1 map from bsd node to IOMedia, so just get the service from the iterator.
	obj = IOIteratorNext(iterator);
	IOObjectRelease(iterator);

	if (obj == 0)
		goto bail;

	// Create an iterator across all parents of the service object passed in.
	err = IORegistryEntryCreateIterator(obj,
										kIOServicePlane,
										kIORegistryIterateRecursively | kIORegistryIterateParents,
										&iterator);
	IOObjectRelease(obj);
	if (KERN_SUCCESS != err) {
		BAIL(kFskErrUnknown);
	}

	while ((service = IOIteratorNext(iterator)) != 0) {
		properties = NULL;

		err = IORegistryEntryCreateCFProperties(service, &properties, kCFAllocatorDefault, kNilOptions);
		if (err == noErr) {
			if (find_and_set_info(properties, CFSTR("Vendor Identification"), vendor)) {
				find_and_set_info(properties, CFSTR("Product Identification"), product);
				find_and_set_info(properties, CFSTR("Product Revision Level"), revision);
				isFound = true;
				goto NextService;
			}

			if (find_and_set_info(properties, CFSTR("device model"), product)) {
				find_and_set_info(properties, CFSTR("device revision"), revision);
				isFound = true;
				goto NextService;
			}
		}

	NextService:
		if (properties)
			CFRelease(properties);

		IOObjectRelease(service);

		if (isFound)
			break;
	}   // while

bail:
	if (iterator)
		IOObjectRelease(iterator);

	if (masterPort)
		mach_port_deallocate(mach_task_self(), masterPort);

	return err;
}

FskErr FskFSVolumeGetID(const char *fullPath, UInt32 *volumeID)
{
	int i;

	for (i = 0; i < gNumStatFS; i++) {
		if (FskStrCompare(gStatFS[i].f_mntonname, fullPath) == 0) {
			*volumeID = i;
			return kFskErrNone;
		}
	}
	return kFskErrInvalidParameter;
}

FskErr FskFSVolumeEject(const char *fullPath, UInt32 flags)
{
	FskCocoaEjectAtPath(fullPath);
	return kFskErrNone;
}
#endif	/* !TARGET_OS_IPHONE */

FskErr FskFSDirectoryChangeNotifierNew(const char *path, UInt32 flags, FskDirectoryChangeNotifierCallbackProc callback, void *refCon, FskDirectoryChangeNotifier *dirChangeNotifier)
{
	FskErr err = kFskErrNone;
	FskFSDirectoryChangeNotifier directoryChangeNotifier;
	struct kevent eventToAdd;
	int errNum;
	CFFileDescriptorContext context = {0, NULL, NULL, NULL, NULL};
	CFRunLoopSourceRef rls;

	err = FskMemPtrNewClear(sizeof(FskFSDirectoryChangeNotifierRecord), (FskMemPtr *)(&directoryChangeNotifier));
	BAIL_IF_ERR(err);

	directoryChangeNotifier->base.dispatch.dispatch = &gFSDispatch;
	directoryChangeNotifier->callback = callback;
	directoryChangeNotifier->refCon = refCon;
	directoryChangeNotifier->path = FskStrDoCopy(path);
	directoryChangeNotifier->dirFD = -1;
	directoryChangeNotifier->kq = -1;
	directoryChangeNotifier->dirKQRef = NULL;

	directoryChangeNotifier->dirFD = open(path, O_EVTONLY);
	if (directoryChangeNotifier->dirFD < 0)
	{
		BAIL(kFskErrFileNotFound);
	}
    
	directoryChangeNotifier->kq = kqueue();
	if (directoryChangeNotifier->kq < 0)
	{
		BAIL(kFskErrOperationFailed);
	}
    
	eventToAdd.ident = directoryChangeNotifier->dirFD;
	eventToAdd.filter = EVFILT_VNODE;
	eventToAdd.flags = EV_ADD | EV_CLEAR;
	eventToAdd.fflags = NOTE_WRITE;
	eventToAdd.data = 0;
	eventToAdd.udata = NULL;

	errNum = kevent(directoryChangeNotifier->kq, &eventToAdd, 1, NULL, 0, NULL);
	if (errNum != 0)
	{
		BAIL(kFskErrOperationFailed);
	}

	context.info = directoryChangeNotifier;
	directoryChangeNotifier->dirKQRef = CFFileDescriptorCreate(NULL, directoryChangeNotifier->kq, true, KQCallback, &context);
	if (directoryChangeNotifier->dirKQRef == NULL)
	{
		BAIL(kFskErrOperationFailed);
	}

	rls = CFFileDescriptorCreateRunLoopSource(NULL, directoryChangeNotifier->dirKQRef, 0);
	if (rls == NULL)
	{
		BAIL(kFskErrOperationFailed);
	}

	CFRunLoopAddSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);
	CFRelease(rls);
	CFFileDescriptorEnableCallBacks(directoryChangeNotifier->dirKQRef, kCFFileDescriptorReadCallBack);

bail:
	if (err)
	{
		FskFSDirectoryChangeNotifierDispose((FskDirectoryChangeNotifier)directoryChangeNotifier);
		directoryChangeNotifier = NULL;
	}

	*dirChangeNotifier = (FskDirectoryChangeNotifier)directoryChangeNotifier;
	return err;
}

FskErr FskFSDirectoryChangeNotifierDispose(FskDirectoryChangeNotifier dirChangeNotifier)
{
	FskFSDirectoryChangeNotifier directoryChangeNotifier;

	if (dirChangeNotifier == NULL) return kFskErrInvalidParameter;
	directoryChangeNotifier = (FskFSDirectoryChangeNotifier)dirChangeNotifier;

	if (directoryChangeNotifier->dirKQRef != NULL)
	{
		CFFileDescriptorInvalidate(directoryChangeNotifier->dirKQRef);
		CFRelease(directoryChangeNotifier->dirKQRef);
	}

	if(directoryChangeNotifier->dirFD != -1)
	{
		close(directoryChangeNotifier->dirFD);
	}
	FskMemPtrDispose(directoryChangeNotifier->path);
	FskMemPtrDispose(directoryChangeNotifier);

	return kFskErrNone;
}

static void KQCallback(CFFileDescriptorRef kqRef, CFOptionFlags callBackTypes, void *info)
{
	FskFSDirectoryChangeNotifier notifier = (FskFSDirectoryChangeNotifier)info;
	struct kevent event;
	struct timespec timeout = {0, 0};
	int eventCount;

	assert(kqRef == notifier->dirKQRef);
	assert(callBackTypes == kCFFileDescriptorReadCallBack);

	eventCount = kevent(notifier->kq, NULL, 0, &event, 1, &timeout);
	assert((eventCount >= 0) && (eventCount < 2));

	notifier->callback(0, NULL, notifier->refCon);

	CFFileDescriptorEnableCallBacks(notifier->dirKQRef, kCFFileDescriptorReadCallBack);
}

FskErr FskFSFileTerminate()
{
	if (gStatFS != NULL) {
		FskMemPtrDispose(gStatFS);
		gStatFS = NULL;
	}
	return kFskErrNone;
}
