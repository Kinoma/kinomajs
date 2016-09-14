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
#include "kpr.h"

#if TARGET_OS_MAC
#include <termios.h>
#include <sys/ioctl.h>
//
//#include <CoreFoundation/CoreFoundation.h>
// 
#include <IOKit/IOKitLib.h>
//#include <IOKit/IOMessage.h>
//#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
//
#include <IOKit/serial/IOSerialKeys.h>
//#include <IOKit/serial/ioss.h>
#include <IOKit/IOBSD.h>
//#include <IOKit/hid/IOHIDDevice.h>

#include "kprBehavior.h"
#include "kprMessage.h"

typedef struct KprSerialDescriptionStruct KprSerialDescriptionRecord, *KprSerialDescription;
typedef struct KprSerialNotifierStruct KprSerialNotifierRecord, *KprSerialNotifier;
typedef struct KprSerialDeviceStruct KprSerialDeviceRecord, *KprSerialDevice;

// KprSerialDescription
struct KprSerialDescriptionStruct {
	KprSerialNotifier notifier;
	SInt32 vendor;
	SInt32 product;
	char* name;
	char* path;
	io_object_t notification;
};

static FskErr KprSerialDescriptionNew(KprSerialDescription *it,  KprSerialNotifier notifier, SInt32 vendor, SInt32 product, char* name, char* path);
static void KprSerialDescriptionDispose(KprSerialDescription self);

// KprSerialNotifier

struct KprSerialNotifierStruct {
	KprSerialNotifier next;
	xsSlot behavior;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	IONotificationPortRef notificationPort;
	io_iterator_t ioIterator;
};

static FskErr KprSerialNotifierNew(KprSerialNotifier *it);
static void KprSerialNotifierDispose(KprSerialNotifier self);
static void Serial_nofifier_register_callback(void *refcon, io_iterator_t iterator);
static void Serial_nofifier_unregister_callback(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);

// KprSerialDevice
struct KprSerialDeviceStruct {
	xsSlot behavior;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	char* path;
	int fd;
	CFSocketRef socket;
	CFRunLoopSourceRef source;
	struct termios options;
	char* buffer;
	UInt32 bufferSize;
	UInt32 bufferIndex;
};

static FskErr KprSerialDeviceNew(KprSerialDevice *it,  char* path);
static void KprSerialDeviceDispose(KprSerialDevice self);
static void KprSerialDeviceClose(KprSerialDevice self);
static FskErr KprSerialDeviceOpen(KprSerialDevice self, UInt32 baud, UInt32 bits, char* parity, UInt32 stop);

static speed_t getBaud(int baud);

#if 0
#pragma mark - Serial Description
#endif

FskErr KprSerialDescriptionNew(KprSerialDescription *it,  KprSerialNotifier notifier, SInt32 vendor, SInt32 product, char* name, char* path)
{
	FskErr err = kFskErrNone;
	KprSerialDescription self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprSerialDescriptionRecord), it));
	self = *it;
	self->notifier = notifier;
	self->vendor = vendor;
	self->product = product;
	if (name) {
		self->name = FskStrDoCopy(name);
		bailIfNULL(self->name);
	}
	self->path = FskStrDoCopy(path);
	bailIfNULL(self->path);
	self->notification = 0;
bail:
	if (err)
		KprSerialDescriptionDispose(self);
	return err;
}

void KprSerialDescriptionDispose(KprSerialDescription self)
{
	kern_return_t kr;
	if (!self) return;
	if (self->name)
		FskMemPtrDisposeAt(&self->name);
	if (self->path)
		FskMemPtrDisposeAt(&self->path);
	if (self->notification)
		kr = IOObjectRelease(self->notification);
	FskMemPtrDispose(self);
}

#if 0
#pragma mark - Serial Notifier
#endif

FskErr KprSerialNotifierNew(KprSerialNotifier *it)
{
	FskErr err = kFskErrNone;
	KprSerialNotifier self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprSerialNotifierRecord), it));
	self = *it;
bail:
	if (err)
		KprSerialNotifierDispose(self);
	return err;
}

void KprSerialNotifierDispose(KprSerialNotifier self)
{
	if (!self) return;
	if (self->ioIterator) {
		IOObjectRelease(self->ioIterator);
		self->ioIterator = 0;
	}
	if (self->notificationPort) {
		IONotificationPortDestroy(self->notificationPort);
		self->notificationPort = NULL;
	}
	FskMemPtrDispose(self);
}

void Serial_Nofifier(xsMachine *the)
{
	KprSerialNotifier self = NULL;
	xsTry {
		xsThrowIfFskErr(KprSerialNotifierNew(&self));
		xsSetHostData(xsResult, self);
		self->the = the;
		self->slot = xsResult;
		self->code = the->code;
		self->behavior = xsUndefined;
		xsRemember(self->slot);
	}
	xsCatch {
	}
}

void Serial_nofifier(void *it)
{
	KprSerialNotifier self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprSerialNotifierDispose(self);
	}
}

void Serial_nofifier_callback(KprSerialNotifier self, char* function, KprSerialDescription description, Boolean dispose)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(2);
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			xsVar(1) = xsAccess(self->slot);
			if (xsFindResult(xsVar(0), xsID(function))) {
				xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsVar(1), xsID("vendor"), xsInteger(description->vendor));
				xsSet(xsVar(1), xsID("product"), xsInteger(description->product));
				if (description->name)
					xsSet(xsVar(1), xsID("name"), xsString(description->name));
				if (description->path)
					xsSet(xsVar(1), xsID("path"), xsString(description->path));
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
		}
		xsCatch {
		}
	}
bail:
	xsEndHostSandboxCode();
	if (dispose)
		KprSerialDescriptionDispose(description);
}

void Serial_nofifier_get_behavior(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void Serial_nofifier_set_behavior(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
//	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsForget(self->behavior);
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

void Serial_nofifier_register_callback(void *refcon, io_iterator_t iterator)
{
	FskErr err = kFskErrNone;
	KprSerialNotifier self = refcon;
    io_service_t usbDevice;
    while ((usbDevice = IOIteratorNext(iterator))) {
		kern_return_t kr;
		io_registry_entry_t parent1, parent2;
		CFTypeRef typeRef;
		CFNumberRef numRef;
		SInt32 vendor = 0;
		SInt32 product = 0;
		char* name= NULL;
		char* path= NULL;

		parent2 = usbDevice;
		while (KERN_SUCCESS == IORegistryEntryGetParentEntry(parent2, kIOServicePlane, &parent1)) {
			if (!vendor && (numRef = IORegistryEntryCreateCFProperty(parent1, CFSTR(kUSBVendorID), kCFAllocatorDefault, 0))) {
				CFNumberGetValue(numRef, kCFNumberSInt32Type, &vendor);
				CFRelease(numRef);
			}
			if (!product && (numRef = IORegistryEntryCreateCFProperty(parent1, CFSTR(kUSBProductID), kCFAllocatorDefault, 0))) {
				CFNumberGetValue(numRef, kCFNumberSInt32Type, &product);
				CFRelease(numRef);
			}
			if (!FskStrLen(name) && (typeRef = IORegistryEntryCreateCFProperty(parent1, CFSTR("Product Name"), kCFAllocatorDefault, 0))) {
				CFIndex length = CFStringGetLength(typeRef);
				CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
				bailIfError(FskMemPtrNewClear(maxSize, &name));
				CFStringGetCString(typeRef, name, maxSize, kCFStringEncodingUTF8);
				CFRelease(typeRef);
			}
			if (vendor && product && FskStrLen(name))
				break;
			parent2 = parent1;
		}
		if (vendor && product) {
			if ((typeRef = IORegistryEntryCreateCFProperty(usbDevice, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0))) {
				CFIndex length = CFStringGetLength(typeRef);
				CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
				bailIfError(FskMemPtrNewClear(maxSize, &path));
				CFStringGetCString(typeRef, path, maxSize, kCFStringEncodingUTF8);
				KprSerialDescription description = NULL;
				KprSerialDescriptionNew(&description, self, vendor, product, name, path);
				if (IOServiceAddInterestNotification(self->notificationPort, usbDevice, kIOGeneralInterest, Serial_nofifier_unregister_callback, description, &description->notification) == KERN_SUCCESS)
					FskThreadPostCallback(FskThreadGetCurrent(), (FskThreadCallback)Serial_nofifier_callback, self, "onSerialRegistered", description, (void*)false);
			}
		}
bail:
		FskMemPtrDispose(name);
		FskMemPtrDispose(path);
		kr = IOObjectRelease(usbDevice);
	}
}

void Serial_nofifier_unregister_callback(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{
	KprSerialDescription description = refCon;
	KprSerialNotifier self = description->notifier;
	FskThreadPostCallback(FskThreadGetCurrent(), (FskThreadCallback)Serial_nofifier_callback, self, "onSerialUnregistered", description, (void*)true);
}

void Serial_nofifier_start(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	CFRunLoopSourceRef runLoopSource;
	kern_return_t result = KERN_FAILURE;
    CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);
	self->notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    runLoopSource = IONotificationPortGetRunLoopSource(self->notificationPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	result = IOServiceAddMatchingNotification(self->notificationPort, kIOPublishNotification, matchingDict, Serial_nofifier_register_callback, self, &self->ioIterator);
	Serial_nofifier_register_callback(self, self->ioIterator);
	FskDebugStr("%s", __FUNCTION__);
}

void Serial_nofifier_stop(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	
	IOObjectRelease(self->ioIterator);
	self->ioIterator = 0;
	IONotificationPortDestroy(self->notificationPort);
	self->notificationPort = NULL;
}

#if 0
#pragma mark - Serial Serial
#endif

#define kSerialDeviceReadBufferSize 1023

FskErr KprSerialDeviceNew(KprSerialDevice *it, char* path)
{
	FskErr err = kFskErrNone;
	KprSerialDevice self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprSerialDeviceRecord), it));
	self = *it;
	self->path = FskStrDoCopy(path);
	bailIfNULL(self->path);
	self->fd = -1;
	bailIfError(FskMemPtrNewClear(kSerialDeviceReadBufferSize + 1, &self->buffer));
	self->bufferSize = kSerialDeviceReadBufferSize;
	self->bufferIndex = 0;
bail:
	if (err)
		KprSerialDeviceDispose(self);
	return err;
}

void KprSerialDeviceDispose(KprSerialDevice self)
{
	if (!self) return;
	KprSerialDeviceClose(self);
	FskMemPtrDispose(self->path);
	FskMemPtrDispose(self);
}

void KprSerialDeviceCallback(KprSerialDevice self)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(1);
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			if (xsFindResult(xsVar(0), xsID("onSerialData"))) {
				self->buffer[self->bufferIndex] = 0;
				(void)xsCallFunction1(xsResult, xsVar(0), xsString(self->buffer));
			}
		}
		xsCatch {
		}
	}
bail:
	xsEndHostSandboxCode();
	self->bufferIndex = 0;
}

void KprSerialDeviceClose(KprSerialDevice self)
{
	if (self->fd != -1) {
		close(self->fd);
		self->fd = -1;
	}
}

void KprSerialDeviceRead(KprSerialDevice self, char* buffer, int size)
{
	int i;
	Boolean escape = false;
	int type = 0;
	
	for (i = 0; i < size; i++) {
		if (escape) {
			switch (type) {
				case 0:
					type = (buffer[i] == 0x5B) ? 1 : 2;
					break;
				case 1:
					if ((buffer[i] >= 0x40) && (buffer[i] <= 0x7E))
						escape = false;
					break;
				case 2:
					if ((buffer[i] >= 0x40) && (buffer[i] <= 0x5F))
						escape = false;
					break;
			}
		}
		else if (buffer[i] == 0x1B) { // ESC
			escape = true;
			type = 0;
		}
		else {
			self->buffer[self->bufferIndex++] = buffer[i];
			if (self->bufferIndex == self->bufferSize - 1)
				KprSerialDeviceCallback(self);
		}
	}
}


static void KprSerialDeviceReadCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	KprSerialDevice self = context;
	int size = -1;
	int total = 0;
	char buffer[kSerialDeviceReadBufferSize];

	while (total < kSerialDeviceReadBufferSize) {
		size = read(self->fd, buffer + total, kSerialDeviceReadBufferSize - total);
		if (size == 0)
			break;
		if (size < 0) {
			snprintf(buffer, kSerialDeviceReadBufferSize, "\n# Error reading serial port %s - %s (%d).\n", self->path, strerror(errno), errno);
			total = FskStrLen(buffer);
			goto bail;
		}
		else {
			total += size;
			if (total == kSerialDeviceReadBufferSize) {
				KprSerialDeviceRead(self, buffer, total);
				total = 0;
			}
		}
	}
bail:
	if (total)
		KprSerialDeviceRead(self, buffer, total);
	if (self->bufferIndex)
		KprSerialDeviceCallback(self);
}

FskErr KprSerialDeviceOpen(KprSerialDevice self, UInt32 baud, UInt32 bits, char* parity, UInt32 stop)
{
	FskErr err = kFskErrNone;
	int fd = -1;

    fd = open(self->path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        FskDebugStr("Error opening serial port %s - %s(%d).\n", self->path, strerror(errno), errno);
        bailIfError(kFskErrBadData);
    }
    
    if (ioctl(fd, TIOCEXCL) == -1) {
        FskDebugStr("Error setting TIOCEXCL on %s - %s(%d).\n", self->path, strerror(errno), errno);
        bailIfError(kFskErrBadData);
    }

    cfmakeraw(&self->options);
    self->options.c_cc[VMIN] = 0;
    self->options.c_cc[VTIME] = 10;
    
	// baud rate
    cfsetspeed(&self->options, getBaud(baud));
	// bits per character
	self->options.c_cflag &= ~CSIZE;
	if (bits == 5)
		self->options.c_cflag |= CS5;
	else if (bits == 6)
		self->options.c_cflag |= CS6;
	else if (bits == 7)
		self->options.c_cflag |= CS7;
	else if (bits == 8)
		self->options.c_cflag |= CS8;
	else
        bailIfError(kFskErrBadData);
	// parity
	if (!parity || !FskStrCompare(parity, "N"))
		self->options.c_cflag &= ~(PARENB | PARODD);
	else {
		self->options.c_cflag |= PARENB;
		if (!FskStrCompare(parity, "O"))
			self->options.c_cflag |= PARODD;
		else if (!FskStrCompare(parity, "E"))
			self->options.c_cflag &= ~PARODD;
		else
			bailIfError(kFskErrBadData);
	}
	// stop bits
	if (stop == 1)
		self->options.c_cflag &= ~CSTOPB;
	else if (stop == 2)
		self->options.c_cflag |= ~CSTOPB;
	else
        bailIfError(kFskErrBadData);

    // Cause the new self->options to take effect immediately.
    if (tcsetattr(fd, TCSANOW, &self->options) == -1) {
        FskDebugStr("Error setting tty attributes %s - %s(%d).\n", self->path, strerror(errno), errno);
        bailIfError(kFskErrBadData);
    }
	
	self->fd = fd;
	
bail:
	return err;
}

FskErr KprSerialDeviceWrite(KprSerialDevice self, char* data)
{
	FskErr err = kFskErrNone;
	UInt32 length = FskStrLen(data);
	ssize_t numBytes;
	numBytes = write(self->fd, data, length);
//	FskDebugStr("KprSerialDeviceWrite %lu - %lu.\n", length, numBytes);
	return err;
}

void Serial_Device(xsMachine *the)
{
	KprSerialDevice self = NULL;
	char* path = xsToString(xsArg(0));
	xsTry {
		xsThrowIfFskErr(KprSerialDeviceNew(&self, path));
		xsSetHostData(xsResult, self);
		self->the = the;
		self->slot = xsResult;
		self->code = the->code;
		self->behavior = xsUndefined;
		xsRemember(self->slot);
	}
	xsCatch {
	}
}

void Serial_device(void *it)
{
	KprSerialDevice self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprSerialDeviceDispose(self);
	}
}

void Serial_device_get_behavior(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void Serial_device_set_behavior(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
//	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsForget(self->behavior);
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

void Serial_device_close(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	if (self->source) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self->source, kCFRunLoopCommonModes);
		CFRelease(self->source);
		self->source = NULL;
	}
	if (self->socket) {
		CFSocketInvalidate(self->socket);
		CFRelease(self->socket);
		self->socket = NULL;
	}
	KprSerialDeviceClose(self);
}

void Serial_device_open(xsMachine *the)
{
	CFSocketContext context;
	KprSerialDevice self = xsGetHostData(xsThis);
	UInt32 baud = 115200;
	UInt32 bits = 8;
	UInt32 stop = 1;
	char* parity = NULL;
	
	SInt32 argc = xsToInteger(xsArgc);
	if (argc > 0)
		baud = xsToInteger(xsArg(0));
	if (argc > 1)
		bits = xsToInteger(xsArg(1));
	if ((argc > 2) && (xsTest(xsArg(2))))
		parity = xsToString(xsArg(2));
	if (argc > 3)
		stop = xsToInteger(xsArg(3));

	xsThrowIfFskErr(KprSerialDeviceOpen(self, baud, bits, parity, stop));
	FskMemSet(&context, 0, sizeof(CFSocketContext));
	context.info = (void*)self;
	self->socket = CFSocketCreateWithNative(kCFAllocatorDefault, self->fd, kCFSocketReadCallBack, KprSerialDeviceReadCallback, &context);
	self->source = CFSocketCreateRunLoopSource(NULL, self->socket, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), self->source, kCFRunLoopCommonModes);
}

void Serial_device_write(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	char* data = xsToString(xsArg(0));
	xsThrowIfFskErr(KprSerialDeviceWrite(self, data));
}

speed_t getBaud(int baud)
{
	switch (baud) {
		case 0:
			return B0;
		case 50:
			return B50;
		case 75:
			return B75;
		case 110:
			return B110;
		case 134:
			return B134;
		case 150:
			return B150;
		case 200:
			return B200;
		case 300:
			return B300;
		case 600:
			return B600;
		case 1200:
			return B1200;
		case 1800:
			return B1800;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
#if TARGET_OS_LINUX
		case 460800:
			return B460800;
		case 500000:
			return B500000;
		case 576000:
			return B576000;
		case 921600:
			return B921600;
		case 1000000:
			return B1000000;
		case 1152000:
			return B1152000;
		case 1500000:
			return B1500000;
		case 2000000:
			return B2000000;
		case 2500000:
			return B2500000;
		case 3000000:
			return B3000000;
		case 3500000:
			return B3500000;
		case 4000000:
			return B4000000;
#endif
		default:
			return B0;
	}
}
#elif TARGET_OS_WIN32

// References
// http://www.ftdichip.com/Support/Documents/AppNotes/AN_152_Detecting_USB_%20Device_Insertion_and_Removal.pdf
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363432(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/ff802693.aspx
// http://www.naughter.com/enumser.html
// http://stackoverflow.com/questions/10390151/how-do-i-use-commtimeouts-to-wait-until-bytes-are-available-but-read-more-than-o

#include <dbt.h>
#include <SetupAPI.h>
#include "FskMain.h"
#include "kprBehavior.h"
#include "kprMessage.h"

typedef struct KprSerialDescriptionStruct KprSerialDescriptionRecord, *KprSerialDescription;
typedef struct KprSerialDeviceStruct KprSerialDeviceRecord, *KprSerialDevice;
typedef struct KprSerialNotifierStruct KprSerialNotifierRecord, *KprSerialNotifier;

static void LogLastError(void) {
	DWORD dwErr = GetLastError();
	char buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 255, NULL);
	FskStrCat(buf, "\n");
	OutputDebugString(buf);
}

// KprSerialDescription
struct KprSerialDescriptionStruct {
	KprSerialNotifier notifier;
	SInt32 vendor;
	SInt32 product;
	char* name;
	char* path;
};

static FskErr KprSerialDescriptionNew(KprSerialDescription *it, KprSerialNotifier notifier, SInt32 vendor, SInt32 product, char* name, char* path);
static void KprSerialDescriptionDispose(KprSerialDescription self);

FskErr KprSerialDescriptionNew(KprSerialDescription *it, KprSerialNotifier notifier, SInt32 vendor, SInt32 product, char* name, char* path)
{
	FskErr err = kFskErrNone;
	KprSerialDescription self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprSerialDescriptionRecord), it));
	self = *it;
	self->notifier = notifier;
	self->vendor = vendor;
	self->product = product;
	if (name) {
		self->name = FskStrDoCopy(name);
		bailIfNULL(self->name);
	}
	self->path = FskStrDoCopy(path);
	bailIfNULL(self->path);
bail:
	if (err)
		KprSerialDescriptionDispose(self);
	return err;
}

void KprSerialDescriptionDispose(KprSerialDescription self)
{
	if (!self) return;
	FskMemPtrDispose(self->name);
	FskMemPtrDispose(self->path);
	FskMemPtrDispose(self);
}

// KprSerialNotifier
struct KprSerialNotifierStruct {
	xsSlot behavior;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	HWND hWnd;
	HDEVNOTIFY hDeviceNotify;
};

static FskErr GetCommPortDetails(char *port, DWORD flags, char **name, SInt32 *vendor, SInt32 *product);
static FskErr KprSerialNotifierNew(KprSerialNotifier *it);
static void KprSerialNotifierDispose(KprSerialNotifier self);
static void SerialNotifierPollDevices(KprSerialNotifier self);
static void SerialNotifierAddRemovePort(KprSerialNotifier notifier, PDEV_BROADCAST_HDR pHdr, Boolean add);
static long FAR PASCAL SerialNotifierWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);

// GUID for all USB serial host PnP drivers
static GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };

FskErr KprSerialNotifierNew(KprSerialNotifier *it)
{
	FskErr err = kFskErrNone;
	KprSerialNotifier self = NULL;
	HINSTANCE hInst = FskMainGetHInstance();
	WNDCLASS wc;
	bailIfError(FskMemPtrNewClear(sizeof(KprSerialNotifierRecord), it));
	self = *it;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = SerialNotifierWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "serial-notifier";
	RegisterClass(&wc);
	self->hWnd = CreateWindow("serial-notifier", NULL, WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, self);

bail:
	if (err)
		KprSerialNotifierDispose(self);
	return err;
}

void KprSerialNotifierDispose(KprSerialNotifier self)
{
	if (!self) return;
	if (self->hDeviceNotify) {
		UnregisterDeviceNotification(self->hDeviceNotify);
	}
	if (self->hWnd) {
		DestroyWindow(self->hWnd);
	}
	FskMemPtrDispose(self);
}

void Serial_nofifier(void *it)
{
	KprSerialNotifier self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprSerialNotifierDispose(self);
	}
}

void Serial_nofifier_callback(KprSerialNotifier self, char* function, KprSerialDescription description, Boolean dispose)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(2);
	{
		xsTry{
			xsVar(0) = xsAccess(self->behavior);
			xsVar(1) = xsAccess(self->slot);
			if (xsFindResult(xsVar(0), xsID(function))) {
				xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
				xsSet(xsVar(1), xsID("vendor"), xsInteger(description->vendor));
				xsSet(xsVar(1), xsID("product"), xsInteger(description->product));
				if (description->name)
					xsSet(xsVar(1), xsID("name"), xsString(description->name));
				if (description->path)
					xsSet(xsVar(1), xsID("path"), xsString(description->path));
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
		}
		xsCatch{
		}
	}
bail:
	xsEndHostSandboxCode();
	if (dispose)
		KprSerialDescriptionDispose(description);
}

void Serial_nofifier_get_behavior(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void Serial_nofifier_set_behavior(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	//	if (xsTypeOf(self->behavior) != xsUndefinedType)
	xsForget(self->behavior);
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

void Serial_nofifier_start(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	FskMemSet(&NotificationFilter, 0, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = WceusbshGUID;
	self->hDeviceNotify = RegisterDeviceNotification(self->hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

	// Poll for devices when starting notifier
	SerialNotifierPollDevices(self);
}

void Serial_nofifier_stop(xsMachine *the)
{
	KprSerialNotifier self = xsGetHostData(xsThis);
	if (self->hDeviceNotify) {
		UnregisterDeviceNotification(self->hDeviceNotify);
		self->hDeviceNotify = NULL;
	}
}

void Serial_Nofifier(xsMachine *the)
{
	KprSerialNotifier self = NULL;
	xsTry{
		xsThrowIfFskErr(KprSerialNotifierNew(&self));
		xsSetHostData(xsResult, self);
		self->the = the;
		self->slot = xsResult;
		self->code = the->code;
		self->behavior = xsUndefined;
		xsRemember(self->slot);
	}
	xsCatch{
	}
}

long FAR PASCAL SerialNotifierWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	PDEV_BROADCAST_HDR pHdr;
	static KprSerialNotifier notifier = NULL;
	switch (msg) {
		case WM_CREATE:
			notifier = (KprSerialNotifier)((LPCREATESTRUCT)lParam)->lpCreateParams;
			break;
		case WM_DEVICECHANGE:
			switch (wParam) {
				case DBT_DEVICEARRIVAL:
				case DBT_DEVICEREMOVECOMPLETE:
					pHdr = (PDEV_BROADCAST_HDR)lParam;
					if (DBT_DEVTYP_PORT == pHdr->dbch_devicetype) {
						PDEV_BROADCAST_PORT pPort = (PDEV_BROADCAST_PORT)pHdr;
						if (pPort->dbcp_name && (0 == FskStrCompareCaseInsensitiveWithLength(pPort->dbcp_name, "COM", 3))) {
							SerialNotifierAddRemovePort(notifier, (char*)pPort->dbcp_name, (DBT_DEVICEARRIVAL == wParam));
						}
					}
					break;
			}
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

void SerialNotifierAddRemovePort(KprSerialNotifier notifier, char *port, Boolean add)
{
	DWORD flags = DIGCF_DEVICEINTERFACE;
	char *name = NULL;
	SInt32 vendor = 0, product = 0;
	KprSerialDescription description = NULL;
	if (add)
		flags |= DIGCF_PRESENT;
	GetCommPortDetails(port, flags, &name, &vendor, &product);
	if (vendor && product) {
		KprSerialDescriptionNew(&description, notifier, vendor, product, name, port);
		FskThreadPostCallback(FskThreadGetCurrent(), (FskThreadCallback)Serial_nofifier_callback, notifier, add ? "onSerialRegistered" : "onSerialUnregistered", description, (void*)true);
	}
	FskMemPtrDispose(name);
}

void SerialNotifierPollDevices(KprSerialNotifier notifier)
{
	UInt32 i;
	for (i = 1; i < 256; ++i) {
		Boolean success = false;
		char szPort[32];
		HANDLE hFile;
		sprintf(szPort, "COM%u", i);
		hFile = CreateFile(szPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		if (INVALID_HANDLE_VALUE == hFile) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_ACCESS_DENIED || dwError == ERROR_GEN_FAILURE || dwError == ERROR_SHARING_VIOLATION || dwError == ERROR_SEM_TIMEOUT)
				success = true;
		}
		else {
			success = true;
			CloseHandle(hFile);
		}
		if (success) {
			SerialNotifierAddRemovePort(notifier, szPort, true);
		}
	}
}

FskErr GetCommPortDetails(char *port, DWORD flags, char **name, SInt32 *vendor, SInt32 *product)
{
	HDEVINFO hDevInfoSet;
	SInt32 _vendor = 0;
	SInt32 _product = 0;
	char *_name = NULL;

	hDevInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, flags);
	if (hDevInfoSet != INVALID_HANDLE_VALUE) {
		BOOL bMoreItems = true;
		int nIndex = 0;
		while (bMoreItems && (!_vendor || !_product)) {
			SP_DEVINFO_DATA devInfo;
			_vendor = _product = 0;
			devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
			bMoreItems = SetupDiEnumDeviceInfo(hDevInfoSet, nIndex, &devInfo);
			if (bMoreItems) {
				HKEY deviceKey;
				deviceKey = SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
				if (INVALID_HANDLE_VALUE != deviceKey) {
					char szPort[32];
					DWORD cbData = sizeof(szPort);
					szPort[0] = 0;
					RegQueryValueEx(deviceKey, "PortName", NULL, NULL, szPort, &cbData);
					if (0 == FskStrCompare(szPort, port)) {
						char szBuffer[128];
						DWORD dwSize = sizeof(szBuffer);
						szBuffer[0] = 0;
						if (TRUE == SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_HARDWAREID, NULL, szBuffer, dwSize, NULL)) {
							char *walker = szBuffer;
							while (*walker) {
								char *p;
								if (NULL != (p = FskStrStr(walker, "VID_"))) {
									_vendor = FskStrHexToNum(p + 4, 4);
								}
								if (NULL != (p = FskStrStr(walker, "PID_"))) {
									_product = FskStrHexToNum(p + 4, 4);
								}
								if (_vendor && _product) {
									DWORD dwSize = sizeof(szBuffer);
									szBuffer[0] = 0;
									SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_FRIENDLYNAME, NULL, szBuffer, dwSize, NULL);
									_name = FskStrDoCopy(szBuffer);
									break;
								}
								walker += FskStrLen(walker) + 1;
							}
						}
					}
					RegCloseKey(deviceKey);
				}
			}
			++nIndex;
		}
		SetupDiDestroyDeviceInfoList(hDevInfoSet);
	}

	*vendor = _vendor;
	*product = _product;
	*name = _name;

	return (_vendor && _product) ? kFskErrNone : kFskErrNotFound;
}

// KprSerialDevice
struct KprSerialDeviceStruct {
	xsSlot behavior;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	char* path;
	char* buffer;
	HANDLE hComm;
	Boolean done;
	FskThread readThread;
	FskThread deviceThread;
	UInt32 bufferSize;
	UInt32 bufferIndex;
};

static UInt32 getBaud(UInt32 baud);
static FskErr KprSerialDeviceNew(KprSerialDevice *it, char* path);
static void KprSerialDeviceDispose(KprSerialDevice self);
static void KprSerialDeviceCallback(KprSerialDevice self);
static void KprSerialDeviceClose(KprSerialDevice self);
static FskErr KprSerialDeviceOpen(KprSerialDevice self, UInt32 baud, UInt32 bits, char* parity, UInt32 stop);
static void KprSerialDeviceRead(KprSerialDevice self, char* buffer, int size);
static void KprSerialReadThread(void* refCon);
static void KprSerialReadCallback(KprSerialDevice self, char *buffer, UInt32 length, Boolean dispose);

#define kSerialDeviceReadBufferSize 1023

void KprSerialDeviceCallback(KprSerialDevice self)
{
	xsBeginHostSandboxCode(self->the, self->code);
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	xsVars(1);
	{
		xsTry{
			xsVar(0) = xsAccess(self->behavior);
			if (xsFindResult(xsVar(0), xsID("onSerialData"))) {
				self->buffer[self->bufferIndex] = 0;
				(void)xsCallFunction1(xsResult, xsVar(0), xsString(self->buffer));
			}
		}
		xsCatch{
		}
	}
bail:
	xsEndHostSandboxCode();
	self->bufferIndex = 0;
}

FskErr KprSerialDeviceNew(KprSerialDevice *it, char* path)
{
	FskErr err = kFskErrNone;
	KprSerialDevice self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprSerialDeviceRecord), it));
	self = *it;
	self->path = FskStrDoCopy(path);
	bailIfNULL(self->path);
	bailIfError(FskMemPtrNewClear(kSerialDeviceReadBufferSize + 1, &self->buffer));
	self->bufferSize = kSerialDeviceReadBufferSize;
	self->bufferIndex = 0;
	self->deviceThread = FskThreadGetCurrent();
bail:
	if (err)
		KprSerialDeviceDispose(self);
	return err;
}

FskErr KprSerialDeviceOpen(KprSerialDevice self, UInt32 baud, UInt32 bits, char* parity, UInt32 stop)
{
	DCB dcb;
	COMMTIMEOUTS cto;
	HANDLE hComm = NULL;
	FskErr err = kFskErrNone;

	hComm = CreateFile(self->path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (hComm == INVALID_HANDLE_VALUE)
		bailIfError(kFskErrBadData);

	FskMemSet(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = getBaud(baud);
	dcb.ByteSize = bits;
	if (!FskStrCompare(parity, "N"))
		dcb.Parity = NOPARITY;
	else if (!FskStrCompare(parity, "E"))
		dcb.Parity = EVENPARITY;
	else if (!FskStrCompare(parity, "O"))
		dcb.Parity = ODDPARITY;
	else
		bailIfError(kFskErrBadData);
	if (1 == stop)
		dcb.StopBits = ONESTOPBIT;
	else if (2 == stop)
		dcb.StopBits = TWOSTOPBITS;
	else
		bailIfError(kFskErrBadData);
	if (!SetCommState(hComm, &dcb))
		bailIfError(kFskErrBadData);

	GetCommTimeouts(hComm, &cto);
	cto.ReadIntervalTimeout = MAXDWORD;
	cto.ReadTotalTimeoutConstant = 20;
	cto.ReadTotalTimeoutMultiplier = MAXDWORD;
	if (!SetCommTimeouts(hComm, &cto))
		bailIfError(kFskErrBadData);

	self->hComm = hComm;
	bailIfError(FskThreadCreate(&self->readThread, KprSerialReadThread, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit, self, "KprSerialReadThread"));

bail:
	if (err)
		KprSerialDeviceClose(self);
	return err;
}

void KprSerialDeviceClose(KprSerialDevice self)
{
	if (self->readThread) {
		self->done = true;
		FskThreadJoin(self->readThread);
		self->readThread = NULL;
	}
	if (self->hComm) {
		CloseHandle(self->hComm);
		self->hComm = NULL;
	}
}

void KprSerialDeviceDispose(KprSerialDevice self)
{
	if (!self) return;
	KprSerialDeviceClose(self);
	FskMemPtrDispose(self->path);
	FskMemPtrDispose(self);
}

void KprSerialDeviceRead(KprSerialDevice self, char* buffer, int size)
{
	int i;
	Boolean escape = false;
	int type = 0;

	for (i = 0; i < size; i++) {
		if (escape) {
			switch (type) {
			case 0:
				type = (buffer[i] == 0x5B) ? 1 : 2;
				break;
			case 1:
				if ((buffer[i] >= 0x40) && (buffer[i] <= 0x7E))
					escape = false;
				break;
			case 2:
				if ((buffer[i] >= 0x40) && (buffer[i] <= 0x5F))
					escape = false;
				break;
			}
		}
		else if (buffer[i] == 0x1B) { // ESC
			escape = true;
			type = 0;
		}
		else {
			self->buffer[self->bufferIndex++] = buffer[i];
			if (self->bufferIndex == self->bufferSize - 1)
				KprSerialDeviceCallback(self);
		}
	}
	if (self->bufferIndex)
		KprSerialDeviceCallback(self);
}

void KprSerialReadCallback(KprSerialDevice self, char *buffer, UInt32 length, Boolean dispose)
{
	KprSerialDeviceRead(self, buffer, length);
	if (dispose)
		FskMemPtrDispose(buffer);
}

void KprSerialReadThread(void* refCon)
{
	KprSerialDevice self = (KprSerialDevice)refCon;
	DWORD dwRead;
	char lpBuf[kSerialDeviceReadBufferSize];

	FskThreadInitializationComplete(FskThreadGetCurrent());

	while (!self->done) {
		if (!ReadFile(self->hComm, lpBuf, kSerialDeviceReadBufferSize, &dwRead, NULL)) {
			//@@ something went wrong
		}
		else if (dwRead) {
			FskMemPtr bytes;
			if (kFskErrNone == FskMemPtrNewFromData(dwRead, lpBuf, &bytes)) {
				FskThreadPostCallback(self->deviceThread, (FskThreadCallback)KprSerialReadCallback, self, bytes, (void*)dwRead, (void*)true);
			}
		}
	}
}

FskErr KprSerialDeviceWrite(KprSerialDevice self, char* data)
{
	FskErr err = kFskErrNone;
	DWORD length = FskStrLen(data);
	DWORD dwWritten;
	if (!WriteFile(self->hComm, data, length, &dwWritten, NULL)) {
		err = kFskErrOperationFailed;
	}
	return err;
}

void Serial_device(void *it)
{
	KprSerialDevice self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprSerialDeviceDispose(self);
	}
}

void Serial_device_get_behavior(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void Serial_device_set_behavior(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	//	if (xsTypeOf(self->behavior) != xsUndefinedType)
	xsForget(self->behavior);
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

void Serial_device_open(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	UInt32 baud = 115200;
	UInt32 bits = 8;
	UInt32 stop = 1;
	char* parity = NULL;

	SInt32 argc = xsToInteger(xsArgc);
	if (argc > 0)
		baud = xsToInteger(xsArg(0));
	if (argc > 1)
		bits = xsToInteger(xsArg(1));
	if ((argc > 2) && (xsTest(xsArg(2))))
		parity = xsToString(xsArg(2));
	if (argc > 3)
		stop = xsToInteger(xsArg(3));

	xsThrowIfFskErr(KprSerialDeviceOpen(self, baud, bits, parity, stop));
}

void Serial_device_close(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	KprSerialDeviceClose(self);
}

void Serial_device_write(xsMachine *the)
{
	KprSerialDevice self = xsGetHostData(xsThis);
	char* data = xsToString(xsArg(0));
	xsThrowIfFskErr(KprSerialDeviceWrite(self, data));
}

void Serial_Device(xsMachine *the)
{
	KprSerialDevice self = NULL;
	char* path = xsToString(xsArg(0));
	xsTry{
		xsThrowIfFskErr(KprSerialDeviceNew(&self, path));
		xsSetHostData(xsResult, self);
		self->the = the;
		self->slot = xsResult;
		self->code = the->code;
		self->behavior = xsUndefined;
		xsRemember(self->slot);
	}
	xsCatch{
	}
}

UInt32 getBaud(UInt32 baud)
{
	switch (baud) {
		case 110: return CBR_110;
		case 300: return CBR_300;
		case 600: return CBR_600;
		case 1200: return CBR_1200;
		case 4800: return CBR_4800; 
		case 9600: return CBR_9600; 
		case 14400: return CBR_14400; 
		case 19200: return CBR_19200; 
		case 38400: return CBR_38400; 
		case 56000: return CBR_56000; 
		case 57600: return CBR_57600; 
		case 115200: return CBR_115200; 
		case 128000: return CBR_128000; 
		case 256000: return CBR_256000; 
		default:
			return 0;
	}
}

#else

#include "kprBehavior.h"
#include "kprMessage.h"

void Serial_nofifier(void *data)
{
}

void Serial_nofifier_get_behavior(xsMachine *the)
{
}

void Serial_nofifier_set_behavior(xsMachine *the)
{
}

void Serial_nofifier_start(xsMachine *the)
{
}

void Serial_nofifier_stop(xsMachine *the)
{
}

void Serial_Nofifier(xsMachine *the)
{
}

void Serial_device(void *data)
{
}

void Serial_device_get_behavior(xsMachine *the)
{
}

void Serial_device_set_behavior(xsMachine *the)
{
}

void Serial_device_open(xsMachine *the)
{
}

void Serial_device_close(xsMachine *the)
{
}

void Serial_device_write(xsMachine *the)
{
}

void Serial_Device(xsMachine *the)
{
}

#endif
