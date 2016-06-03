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

#include "FskUUID.h"
#include "FskFiles.h"
#include "FskNetInterface.h"
#include "kpr.h"
#include "kprMessage.h"

#if MINITV
#include <sys/un.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>


static FskErr k4SystemCommand(char* command, char **buffer)
{
	char line[1035];
	FILE *fp = NULL;
	char *out = NULL;
	FskErr err = kFskErrNone;

	fp = popen(command, "r");
	bailIfNULL(fp);

	while (fgets(line, sizeof(line)-1, fp) != NULL) {
		FskDebugStr("%s", line);
		if (!out)
			FskMemPtrNewClear(1, &out);
		bailIfError(FskMemPtrRealloc(FskStrLen(out) + FskStrLen(line) + 1, &out));
		FskStrCat(out, line);
	}

bail:
	if (fp)
		pclose(fp);
	if (err)
		FskMemPtrDisposeAt(&out);
	*buffer = out;
	return err;
}

static Boolean k4MD5Check(char* path, char* md5){
	char buffer[1024];
	char* fileMD5 = NULL;
	unsigned int len;
	Boolean result = false;

	snprintf(buffer, sizeof(buffer), "md5sum %s | grep -om1 '^[0-9a-f]*'", path);
	k4SystemCommand(buffer, &fileMD5);

	len = strlen(fileMD5);
	if (fileMD5[len - 1] == '\n')
	    fileMD5[len - 1] = '\0';


	if (FskStrCompare(fileMD5, md5) != 0) {
		result = false;
	} else {
		result = true;
	}

	FskMemPtrDispose(fileMD5);
	return result;
}

static Boolean k4MD5CheckTwo(char* path1, char* path2){
	char buffer[1024];
	char* fileMD51 = NULL;
	char* fileMD52 = NULL;
	Boolean result = false;

	snprintf(buffer, sizeof(buffer), "md5sum %s | grep -om1 '^[0-9a-f]*'", path1);
	k4SystemCommand(buffer, &fileMD51);

	snprintf(buffer, sizeof(buffer), "md5sum %s | grep -om1 '^[0-9a-f]*'", path2);
	k4SystemCommand(buffer, &fileMD52);

	if (FskStrCompare(fileMD51, fileMD52) != 0) {
		result = false;
	} else {
		result = true;
	}

	FskMemPtrDispose(fileMD51);
	FskMemPtrDispose(fileMD52);
	return result;
}

#endif

#if 0
#pragma mark - extension
#endif

#define bailIfFalse(X) { if ((X) == false) { goto bail; } }

void KprK4Invoke(KprService service, KprMessage message);
void KprK4Start(KprService service, FskThread thread, xsMachine* the);
void KprK4Stop(KprService service UNUSED);

KprServiceRecord gKprK4Service = {
	NULL,
	kprServicesThread,
	"xkpr://k4/",
	NULL,
	NULL,
	KprServiceAccept,
	KprServiceCancel,
	KprK4Invoke,
	KprK4Start,
	KprK4Stop
};

#if 0
#pragma mark - KprK4
#endif

static void KprK4SetTimezone(char *timezone)
{
	FskTimeTzset(timezone);
}

static void KprK4SetDate(UInt32 secsSinceEpoch)
{
	FskTimeRecord time;
	FskTimeClear(&time);
	time.seconds = secsSinceEpoch;
	FskTimeStime(&time);
}

static FskErr KprK4GetDeviceID(char **deviceID)
{
	FskErr err = kFskErrNone;
	const char *fileName = "k4.deviceid";
	char *directory = NULL;
	char *path = NULL;
	char *uuidStr = NULL;
	FskFileMapping map = NULL;
	char *buffer;
	FskInt64 size;
	FskUUIDRecord uuid;
	FskFileInfo fileInfo;

	// First try to use the saved UUID
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &directory));
	bailIfError(FskMemPtrNew(FskStrLen(directory) + FskStrLen(fileName) + 1, &path));
	FskStrCopy(path, directory);
	FskStrCat(path, fileName);
	if (kFskErrNone == FskFileGetFileInfo(path, &fileInfo)) {
		if (kFskErrNone == FskFileMap(path, (unsigned char**)&buffer, &size, 0, &map)) {
			*deviceID = FskStrDoCopy(buffer);
			goto bail;
		}
	}

	// If no saved UUID then generate a new one
	bailIfError(FskUUIDCreate(&uuid));
	//uuidStr = FskUUIDtoString(&uuid);
	uuidStr = FskUUIDtoString_844412(&uuid);
	*deviceID = FskStrDoCopy(uuidStr);
	if (NULL == *deviceID)
		BAIL(kFskErrOperationFailed);

	// Save the new UUID for next time
	if (NULL != path) {
		if (kFskErrNone == FskFileCreate(path)) {
			FskFile fref = NULL;
			if (kFskErrNone == FskFileOpen(path, kFskFilePermissionReadWrite, &fref)) {
				FskFileWrite(fref, FskStrLen(uuidStr) + 1, (const void *)uuidStr, NULL);
				FskFileClose(fref);
			}
		}
	}

bail:
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	FskMemPtrDispose(directory);
	FskMemPtrDispose(uuidStr);
	return err;
}

static FskErr KprK4GetMAC(char **MAC)
{
	FskErr err = kFskErrNone;

#if MINITV
	UInt32 i, count;
	char line[1024];
	FILE *fp = NULL;

	*MAC = NULL;

	// First try to get MAC address from FskNetInterface
	count = FskNetInterfaceEnumerate();
	for (i = 0; i < count; i++) {
		FskNetInterface iface;
		char mac[32];
		FskNetInterfaceDescribe(i, &iface);
		if (0 == FskStrCompare("mlan0", iface->name)) {
			FskStrNumToHex(iface->MAC[0], mac, 2);
			mac[2] = ':';
			FskStrNumToHex(iface->MAC[1], &mac[3], 2);
			mac[5] = ':';
			FskStrNumToHex(iface->MAC[2], &mac[6], 2);
			mac[8] = ':';
			FskStrNumToHex(iface->MAC[3], &mac[9], 2);
			mac[11] = ':';
			FskStrNumToHex(iface->MAC[4], &mac[12], 2);
			mac[14] = ':';
			FskStrNumToHex(iface->MAC[5], &mac[15], 2);
			mac[17] = '\0';
			*MAC = FskStrDoCopy(mac);
		}
		FskNetInterfaceDescriptionDispose(iface);
		if (NULL != *MAC)
			goto bail;
	}

	// Next try getting MAC from "ifconfig -a"
	fp = popen("ifconfig -a", "r");
	bailIfNULL(fp);
	while (fgets(line, sizeof(line)-1, fp) != NULL) {
		if (0 == FskStrCompareWithLength(line, "mlan0", 5)) {
			char *p;
			if ((p = FskStrStr(line, "HWaddr ")) != NULL) {
				p += 7;
				p = FskStrStripHeadSpace(p);
				p = FskStrStripTailSpace(p);
				*MAC = FskStrDoCopy(p);
				goto bail;
			}
		}
	}
#else
	*MAC = FskStrDoCopy("66:55:44:33:22:11");	// for compatibility with kprWiFi mockup implementation
	goto bail;
#endif

bail:
#if MINITV
	if (fp)
		pclose(fp);
#endif
	if (NULL == *MAC)
		err = kFskErrNotFound;

	return err;
}

#if 0
// @@ Unused functions
static void KprK4Unmount(char *mountPoint)
{
#if MINITV
	char *cmd;
	cmd = FskStrDoCat("umount ", mountPoint);
fprintf(stderr, "KprK4Unmount -- %s\n", cmd);
	system(cmd);
#endif
}

static void KprK4Mount(char *device, char *mountPoint)
{
#if MINITV
	char cmd[512];
	snprintf(cmd, 512, "mount %s %s\n", device, mountPoint);
fprintf(stderr, "KprK4Mount -- %s\n", cmd);
	system(cmd);
#endif
}
#endif

#if 0
#pragma mark - xs
#endif

void KPR_K4_set_timezone(xsMachine *the)
{
	char *timezone = xsToString(xsArg(0));
	KprK4SetTimezone(timezone);
}

void KPR_K4_set_date(xsMachine *the)
{
	UInt32 secsSinceEpoch = xsToInteger(xsArg(0));
	KprK4SetDate(secsSinceEpoch);
}

void KPR_K4_reboot(xsMachine *the)
{
#if MINITV
	system("/sbin/reboot");
#endif
}

void KPR_K4_shutdown(xsMachine *the)
{
#if MINITV
    system("poweroff");
#endif
}

void KPR_K4_hibernate(xsMachine *the)
{
#if MINITV
#define K4_HIBERNATE_PATH "/bin/khibernate.sh"
	xsVars(1);
	FskFileInfo fileInfo;

	xsVar(0) = xsGet(xsGlobal, xsID("WPA"));
	if (xsFindResult(xsVar(0), xsID("stop")))
		(void)xsCallFunction0(xsResult, xsVar(0));
	if (kFskErrNone == FskFileGetFileInfo(K4_HIBERNATE_PATH, &fileInfo))
		system(K4_HIBERNATE_PATH);
	if (xsFindResult(xsVar(0), xsID("start")))
		(void)xsCallFunction0(xsResult, xsVar(0));
#endif
}

void KPR_K4_muxAllGPIO(xsMachine *the){
#if MINITV
	system("echo GPIO > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp96");
	system("echo GPIO > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp97");
	system("echo GPIO > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp104");
	system("echo GPIO > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp90_93");
	system("echo GPIO > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp98_99");
#endif
}

void KPR_K4_muxUART(xsMachine *the){
#if MINITV
	system("echo UART > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp98_99");
#endif
}

void KPR_K4_muxUARTFlip(xsMachine *the){
#if MINITV
	system("echo UARTFLIP > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp98_99");
#endif
}

void KPR_K4_muxSPI(xsMachine *the){
#if MINITV
	system("echo SSP > /sys/class/mfp_ctrl/mfp_ctrl_device/mfp90_93");
#endif
}

void KPR_K4_get_deviceID(xsMachine *the)
{
	char *deviceID = NULL;

	xsThrowIfFskErr(KprK4GetDeviceID(&deviceID));
	xsResult = xsString(deviceID);
	FskMemPtrDispose(deviceID);
}

void KPR_K4_get_MAC(xsMachine *the)
{
	char *MAC = NULL;

	xsThrowIfFskErr(KprK4GetMAC(&MAC));
	xsResult = xsString(MAC);
	FskMemPtrDispose(MAC);
}

void KPR_K4_captureScreen(xsMachine *the)
{
    FskErr err;
    FskWindow w;
    FskPort port;
    FskBitmap bits = NULL;
	FskImageCompress comp = NULL;
    FskRectangleRecord bounds;
    void *data = NULL;
    UInt32 dataSize;
    static FskColorRGBARecord white = {255, 255, 255, 255};
    int i;
    Boolean immediate = xsToBoolean(xsArg(0));

    w = FskWindowGetActive();
    xsThrowIfNULL(w);

    if (immediate) {     // once to flush the cache
        FskWindowCopyToBitmap(w, NULL, true, &bits);
        FskBitmapDispose(bits);
    }

    err = FskWindowCopyToBitmap(w, NULL, true, &bits);
    BAIL_IF_ERR(err);

    FskBitmapGetBounds(bits, &bounds);

    // flash
    if (!immediate) {
        FskBitmapDispose(bits);
        bits = NULL;

        FskWindowCopyToBitmap(w, NULL, true, &bits);
    }

    if (bits) {
        port = FskWindowGetPort(w);
        FskPortBeginDrawing(port, &white);
        FskPortEndDrawing(port);

        // fade in
        for (i = 0; i <= 10; i++) {
            static FskColorRGBARecord black = {0, 0, 0, 255};
            FskGraphicsModeParametersRecord params = {sizeof(FskGraphicsModeParametersRecord), (SInt32)(i * 25.5)};

            FskPortBeginDrawing(port, &black);
            FskPortSetGraphicsMode(port, kFskGraphicsModeCopy, &params);
            FskPortBitmapDraw(port, bits, NULL, &bounds);
            FskPortEndDrawing(port);
        }
    }

    // compress
    if (xsToInteger(xsArgc) >= 5) {
        FskRectangleRecord srcRect;

        FskRectangleSet(&srcRect, xsToInteger(xsArg(1)), xsToInteger(xsArg(2)), xsToInteger(xsArg(3)), xsToInteger(xsArg(4)));
        if ((srcRect.width != bounds.width) || (srcRect.height != bounds.height)) {
            FskPort p;
            FskBitmap cropped;
            FskRectangleRecord dstRect = {0, 0, srcRect.width, srcRect.height};

            FskBitmapNew(srcRect.width, srcRect.height, kFskBitmapFormatDefaultNoAlpha, &cropped);
            FskPortNew(&p, cropped, NULL);
            FskPortBeginDrawing(p, NULL);
            FskPortBitmapDraw(p, bits, &srcRect, &dstRect);
            FskPortEndDrawing(p);
            FskPortDispose(p);
            FskBitmapDispose(bits);
            bits = cropped;
            bounds = dstRect;
        }
    }

    BAIL_IF_ERR(FskImageCompressNew(&comp, 0, "image/bmp", bounds.width, bounds.height));

	BAIL_IF_ERR(FskImageCompressFrame(comp, bits, &data, &dataSize, NULL, NULL, NULL, NULL, NULL));

	xsResult = xsArrayBuffer(data, dataSize);

bail:
    FskBitmapDispose(bits);
    FskMemPtrDispose(data);
    FskImageCompressDispose(comp);
    xsThrowIfFskErr(err);
}

static Boolean checkFileExists(char* path){
#if MINITV
	if ( access( path, F_OK ) != -1){
		return true;
	}else{
		return false;
	}
#else
	return false;
#endif
}

void KPR_K4_onSD(xsMachine *the){
#if MINITV
	if ( checkFileExists( "/etc/onSD" ) ){
		xsResult = xsTrue;
	}else{
		xsResult = xsFalse;
	}
#else
	xsIntegerValue argc = xsToInteger(xsArgc);
	if ( argc > 0 ){
		xsResult = xsArg(0);
	}else{
		xsResult = xsFalse;
	}
#endif
}

void KPR_K4_is8887(xsMachine *the){
#if MINITV
    int result;
    xsResult = xsFalse;
    result = system("grep -q SD8887 /proc/mwlan/mlan0/info");
    if (result == 0) xsResult = xsTrue;
#else
    xsIntegerValue argc = xsToInteger(xsArgc);
    if ( argc > 0 ){
        xsResult = xsArg(0);
    }else{
        xsResult = xsFalse;
    }
#endif
}

void KPR_K4_isYocto(xsMachine *the){
#if MINITV
    int result;
    xsResult = xsFalse;
    result = system("grep -q Poky /etc/issue");
    if (result == 0) xsResult = xsTrue;
#else
    xsIntegerValue argc = xsToInteger(xsArgc);
    if ( argc > 0 ){
        xsResult = xsArg(0);
    }else{
        xsResult = xsFalse;
    }
#endif
}

void KPR_K4_flashFrontPins(char* response, UInt32* length){
#if MINITV
    char src[1024];
    char command[1024];
    int test = -1;

    FskStrCopy(src, FskGetApplicationPath());
    FskStrCat(src, "/fsFixes/FrontPins.hex");

    FskStrCopy(command, "/bin/pic_programmer ");
    FskStrCat(command, src);

	if (checkFileExists(src)){
		test = system(command);
		system("sync");
	}

	if (test == 0){
		FskStrCopy(response, "{\"status\":true}");
		*length = 15;
	}else{
		FskStrCopy(response, "{\"status\":false}");
		*length = 16;
	}
#else
	FskStrCopy(response, "{\"status\":true}");
	*length = 15;
#endif
}

void KPR_K4_fixWiFi(){
#if MINITV
	char buffer[1024];
	char src1[1024];
	char src2[1024];
	char* dest1 = "/lib/modules/3.10.52/extra/mlan.ko";
	char* dest2 = "/lib/modules/3.10.52/extra/sd8xxx.ko";

	FskStrCopy(src1, FskGetApplicationPath());
    FskStrCat(src1, "/fsFixes/mlan.ko");

	FskStrCopy(src2, FskGetApplicationPath());
    FskStrCat(src2, "/fsFixes/sd8xxx.ko");

	if (checkFileExists(dest1) && checkFileExists(dest2)){
		if (checkFileExists(src1)){
			if (! k4MD5CheckTwo(src1, dest1)){
				snprintf(buffer, 1024, "mv %s %s", src1, dest1);
				system(buffer);
			}
		}

		if (checkFileExists(src2)){
			if (! k4MD5CheckTwo(src2, dest2)){
				snprintf(buffer, 1024, "mv %s %s", src2, dest2);
				system(buffer);
			}
		}
	}

	system("sync");
#endif
}

void KPR_K4_SDInserted(xsMachine *the){
#if MINITV
	int result;
	result = system("ls /dev/mmc*");
	if (result == 0){
		xsResult = xsTrue;
	}else{
		xsResult = xsFalse;
	}
#else
	xsIntegerValue argc = xsToInteger(xsArgc);
	if ( argc > 0 ){
		xsResult = xsArg(0);
	}else{
		xsResult = xsFalse;
	}
#endif
}

void KPR_K4_isCreateSD(xsMachine *the){
#if MINITV
	xsResult = xsFalse;
	if (checkFileExists("/mnt/SD1/zImage_k4_sd_boot")) xsResult = xsTrue;
	if (checkFileExists("/mnt/SD2/zImage_k4_sd_boot")) xsResult = xsTrue;
	if (checkFileExists("/mnt/SD1/zImage_k4_sd_donotboot")) xsResult = xsTrue;
#else
	xsIntegerValue argc = xsToInteger(xsArgc);
	if ( argc > 0 ){
		xsResult = xsArg(0);
	}else{
		xsResult = xsFalse;
	}
#endif
}

void KPR_K4_isSDBootable(xsMachine *the){
#if MINITV
	int result;
	xsResult = xsFalse;
	result = system("ls /mnt/SD1/zImage_k4_sd_boot");
	if (result == 0) xsResult = xsTrue;
#else
	xsIntegerValue argc = xsToInteger(xsArgc);
	if ( argc > 0 ){
		xsResult = xsArg(0);
	}else{
		xsResult = xsFalse;
	}
#endif
}

void KPR_K4_checkFileExists(xsMachine *the){
#if MINITV
	xsResult = checkFileExists(xsToString(xsArg(0))) ? xsTrue : xsFalse;
#else
	xsResult = xsTrue;
#endif

}

void KPR_K4_changeSDBootable(Boolean boot, char* response, UInt32* length){
	Boolean okay = true;

#if MINITV
	int result;
	if (boot){
		if (checkFileExists("/mnt/SD1/zImage_k4_sd_donotboot")){
			result = system("mv /mnt/SD1/zImage_k4_sd_donotboot /mnt/SD1/zImage_k4_sd_boot");
		}else{
			result = system("mv /mnt/SD2/zImage_k4_sd_boot /mnt/SD1/");
		}
	}else{
		result = system("mv /mnt/SD1/zImage_k4_sd_boot /mnt/SD1/zImage_k4_sd_donotboot");
	}
	if (result != 0) okay = false;
	system("sync");
#endif

	if ( okay ){
		response = "0";
		*length = 1;
	}else{
		response = "-1";
		*length = 2;
	}
}

void KPR_K4_doSDCardPrepAndPop(char* path, char* response, UInt32* length, char* md5, Boolean boot){
	Boolean okay = true;

#if MINITV
	char buffer[200];
	int result = 0;
	okay = k4MD5Check(path, md5);
	bailIfFalse(okay);

	result = system("/bin/prepareSD.sh");
	if (result != 0){ okay = false; goto bail; }

	result = system("/bin/mountSD.sh");
	if (result != 0){ okay = false; goto bail; }

	snprintf(buffer, 200, "mv %s /mnt/SD2/SDFS.tgz", path);
	result = system(buffer);
	if (result != 0){ okay = false; goto bail; }

	system("cd /mnt/SD2/; tar xzpf SDFS.tgz; rm SDFS.tgz");

	if (checkFileExists("/mnt/SD2/zImage_k4_sd_boot")){
		system("mv /mnt/SD2/zImage_k4_sd_boot /mnt/SD1/zImage_k4_sd_donotboot");
	}
	system("sync");

bail:
#endif

	if ( okay ){
		response = "0";
		*length = 1;
	}else{
		response = "-1";
		*length = 2;
	}

}

void KPR_K4_doFirmwareUpdate(char* path, char* response, UInt32* length, char* md5){
	Boolean okay = true;

#if MINITV
	int result = 0;
	char buffer[1000];
	char *preferencesPath = NULL;
	FskErr err = kFskErrNone;

	okay = k4MD5Check(path, md5);
	bailIfFalse(okay);

	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, false, NULL, &preferencesPath));
	snprintf(buffer, sizeof(buffer), "cp -fpr %s /tmp/.preferences/", preferencesPath);
	system(buffer);

	system("mkdir -p /tmp/systemUpdate");

	snprintf(buffer, sizeof(buffer), "mv %s /tmp/systemUpdate/update.tgz", path);
	result = system(buffer);

	if (result != 0) okay = false;

	if (okay) system("cd /tmp/systemUpdate; tar xzf update.tgz; rm update.tgz");

	if ( checkFileExists("/tmp/systemUpdate/NOU") ){
			result = system("flash_eraseall /dev/mtd0");
			if (result == 0) result = system("flashcp /tmp/systemUpdate/NOU /dev/mtd0");
			if (result != 0) okay = false;
			bailIfFalse(okay);
	}

	if ( checkFileExists("/tmp/systemUpdate/zImageSPI") ){
		result = system("/bin/rewriteSPIKernel.sh /tmp/systemUpdate/zImageSPI");
		if (result != 0) okay = false;
		bailIfFalse(okay);
	}

	if ( checkFileExists("/tmp/systemUpdate/SPI.tgz") ){
		result = system("/bin/rewriteUBIFS.sh /tmp/systemUpdate/SPI.tgz");
		if (result != 0) okay = false;
		bailIfFalse(okay);
	}

	if ( checkFileExists("/tmp/.preferences/") ) {
		snprintf(buffer, sizeof(buffer), "cp -fpr /tmp/.preferences/ %s", preferencesPath);
		system(buffer);
		system("sync");
	}

bail:
	FskMemPtrDispose(preferencesPath);
#endif

	if ( okay ){
		response = "0";
		*length = 1;
	}else{
		response = "-1";
		*length = 2;
	}
}

void KPR_K4_doLauncherUpdate(char* path, char* response, UInt32* length, char* md5){
	Boolean okay = true;

#if MINITV
	char buffer[200];
	okay = k4MD5Check(path, md5);
	bailIfFalse(okay);

	system("mv /K.tgz /tmp/OLDK.tgz");
	snprintf(buffer, 200, "mv %s /K.tgz", path);
	if (system(buffer) != 0) okay = false;
	if (okay){
		system("rm /tmp/OLDK.tgz");
	}else{
		system("mv /tmp/OLDK.tgz /K.tgz");
	}
	system("sync");

bail:
#endif

	if (okay){
		response = "0";
		*length = 1;
	}else{
		response = "-1";
		*length = 2;
	}
}

//Service Stuff
FskExport(FskErr) kprK4_fskLoad(FskLibrary library)
{
	KprServiceRegister(&gKprK4Service);
	return kFskErrNone;
}


FskExport(FskErr) kprK4_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void KprK4Start(KprService service, FskThread thread, xsMachine* the)
{
	service->machine = the;
	service->thread = thread;
}

void KprK4Stop(KprService service UNUSED)
{

}

void KprK4Invoke(KprService service, KprMessage message)
{
	FskErr err = kFskErrNone;

	if (KprMessageContinue(message)) {
		UInt32 length = 0;
		char response[200];
		char* value = NULL;
		char* md5 = NULL;

		FskAssociativeArray query = NULL;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryBegin, message)
		if (message->parts.query)
			bailIfError(KprQueryParse(message->parts.query, &query));

		if (FskStrCompareWithLength("doLauncherUpdate", message->parts.name, message->parts.nameLength) == 0) {
			value = FskAssociativeArrayElementGetString(query, "path");
			md5 = FskAssociativeArrayElementGetString(query, "md5");
			KPR_K4_doLauncherUpdate(value, response, &length, md5);
		}else if (FskStrCompareWithLength("doFirmwareUpdate", message->parts.name, message->parts.nameLength) == 0) {
			value = FskAssociativeArrayElementGetString(query, "path");
			md5 = FskAssociativeArrayElementGetString(query, "md5");
			KPR_K4_doFirmwareUpdate(value, response, &length, md5);
		}else if (FskStrCompareWithLength("makeSD", message->parts.name, message->parts.nameLength) == 0) {
			value = FskAssociativeArrayElementGetString(query, "path");
			md5 = FskAssociativeArrayElementGetString(query, "md5");
			KPR_K4_doSDCardPrepAndPop(value, response, &length, md5, false);
		}else if (FskStrCompareWithLength("fixWiFi", message->parts.name, message->parts.nameLength) == 0) {
			KPR_K4_fixWiFi();
			FskMemMove(response, "1", 2);
			length = 2;
		}else if (FskStrCompareWithLength("setSDBootable", message->parts.name, message->parts.nameLength) == 0) {
			value = FskAssociativeArrayElementGetString(query, "bootable");
			if (FskStrCompare("true", value) == 0){
				KPR_K4_changeSDBootable(true, response, &length);
			}else if (FskStrCompare("false", value) == 0){
				KPR_K4_changeSDBootable(false, response, &length);
			}else{
				FskMemMove(response, "-1", 3);
				length = 2;
			}
		}else if(FskStrCompareWithLength("flashFrontPins", message->parts.name, message->parts.nameLength) == 0){
			KPR_K4_flashFrontPins(response, &length);
		}else {
			BAIL(kFskErrNotFound);
		}
		KprMessageSetResponseBody(message, response, length);
		message->status = length ? 200 : 204;
	bail:
		FskAssociativeArrayDispose(query);
	}
	if (kFskErrNeedMoreTime != err) {
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		message->error = err;
		if (!message->status)
			message->status = (err == kFskErrNotFound) ? 404 : 500;
        KprMessageTransform(message, gKprK4Service.machine);
		KprMessageComplete(message);
	}
}
