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
#include "kpr.h"
#include "kprMessage.h"
#include "FskNetUtils.h"
#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(KprWiFi, KprWiFi);

void KprWiFiInvoke(KprService service, KprMessage message);
void KprWiFiStart(KprService service, FskThread thread, xsMachine* the);
void KprWiFiStop(KprService service UNUSED);

static FskMutex gKprWPAMutex = NULL;

KprServiceRecord gWiFiService = {
	NULL,
	kprServicesThread,
	"xkpr://wifi/",
	NULL,
	NULL,
	KprServiceAccept,
	KprServiceCancel,
	KprWiFiInvoke,
	KprWiFiStart,
	KprWiFiStop,
	NULL,
	NULL,
	NULL
};

#define kRequestSize 0x200
#define kResponseSize 0x4000

#if  MINITV
#include <sys/un.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct wpa_ctrl {
	int s;
	struct sockaddr_un local;
	struct sockaddr_un dest;
	char* request;
	UInt32 requestSize;
	char* response;
	UInt32 responseSize;
};

#ifdef BG3CDP
#define CONFIG_CTRL_IFACE "/var/run/wpa_supplicant/wlan0"
#else
#define CONFIG_CTRL_IFACE "/var/run/wpa_supplicant/mlan0"
#endif

#define CONFIG_CTRL_IFACE_CLIENT_PREFIX "wpa_ctrl_"
#define CONFIG_CTRL_IFACE_CLIENT_DIR "/tmp"

struct wpa_ctrl * g_ctrl = NULL;

void KprWPAStart()
{
	FskErr err = kFskErrNone;

	struct wpa_ctrl* ctrl = NULL;
	static int counter = 0;
	int ret;
	int tries = 0;

	FskMutexAcquire(gKprWPAMutex);

	if (g_ctrl) goto bail;

	bailIfError(FskMemPtrNewClear(sizeof(*ctrl), &ctrl))

	ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
	bailIfError(ctrl->s < 0);

	ctrl->local.sun_family = AF_UNIX;
	counter++;
try_again:
	ret = snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
			  CONFIG_CTRL_IFACE_CLIENT_DIR "/"
			  CONFIG_CTRL_IFACE_CLIENT_PREFIX "%d-%d",
			  (int) getpid(), counter);
	if (ret < 0 || (size_t) ret >= sizeof(ctrl->local.sun_path)) {
		BAIL(kFskErrMemFull);
	}
	tries++;
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
		    sizeof(ctrl->local)) < 0) {
		if (errno == EADDRINUSE && tries < 2) {
			/*
			 * getpid() returns unique identifier for this instance
			 * of wpa_ctrl, so the existing socket file must have
			 * been left by unclean termination of an earlier run.
			 * Remove the file and try again.
			 */
			unlink(ctrl->local.sun_path);
			goto try_again;
		}
		BAIL(kFskErrMemFull);
	}


	ctrl->dest.sun_family = AF_UNIX;
	bailIfError(FskStrLen(CONFIG_CTRL_IFACE) + 1 > sizeof(ctrl->dest.sun_path));
	FskStrCopy(ctrl->dest.sun_path, CONFIG_CTRL_IFACE);
	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
		    sizeof(ctrl->dest)) < 0) {
		unlink(ctrl->local.sun_path);
		BAIL(kFskErrMemFull);
	}
	ctrl->requestSize = kRequestSize;
	ctrl->responseSize = kResponseSize;
	bailIfError(FskMemPtrNewClear(ctrl->requestSize, &ctrl->request))
	bailIfError(FskMemPtrNewClear(ctrl->responseSize, &ctrl->response))
    g_ctrl = ctrl;
bail:
	if (err) {
		if (ctrl) {
			if (ctrl->s >= 0)
				close(ctrl->s);
			FskMemPtrDispose(ctrl);
		}
	}
	FskMutexRelease(gKprWPAMutex);
}

void KprWPAStop()
{
	FskMutexAcquire(gKprWPAMutex);
	if (g_ctrl) {
		struct wpa_ctrl* ctrl = g_ctrl;
		unlink(ctrl->local.sun_path);
		close(ctrl->s);
		FskMemPtrDispose(ctrl->request);
		FskMemPtrDispose(ctrl->response);
		FskMemPtrDisposeAt(&g_ctrl);
	}
	FskMutexRelease(gKprWPAMutex);
}

static FskErr parseWPALines(char *text, char ***outputLines, UInt32 *lineCount)
{
	FskErr err = kFskErrNone;
	UInt8 *start = (UInt8*)text;
	UInt8 *end = (UInt8*)text + FskStrLen(text);
	UInt8 *current = start;
	UInt8 **lines = NULL;
	UInt32 lineIndex = 0;
	while (current < end) {
		UInt8 *p = current;
		while (p < end && *p != '\n')
			++p;
		*p = 0;
		bailIfError(FskMemPtrRealloc((lineIndex + 1) * sizeof(UInt8*), &lines));
		lines[lineIndex] = current;
		++lineIndex;
		current = p + 1;
	}
bail:
	if (err) {
		FskMemPtrDisposeAt(&lines);
		lineIndex = 0;
	}
	*outputLines = (char**)lines;
	*lineCount = lineIndex;
	return err;
}

static FskErr parseStatusResults(char *response, char **parsed)
{
	FskErr err;
	char **lines = NULL;
	UInt32 i, lineCount;
	char lineBuffer[1024];
	char *json = NULL;

	bailIfError(parseWPALines(response, &lines, &lineCount));
	bailIfError(FskMemPtrNew(4, &json));
	json[0] = '{';
	json[1] = 0;

	for (i = 0; i < lineCount; ++i) {
		char *name, *value, *walker;
		char *line = lines[i];
		name = walker = line;
		while (*walker != '=')
			++walker;
		*walker++ = 0;
		value = walker;
		if (0 == FskStrCompare(name, "id"))
			snprintf(lineBuffer, sizeof(lineBuffer), "\"id\":%ld", (unsigned long)FskStrToNum(value));
		else
			snprintf(lineBuffer, sizeof(lineBuffer), "\"%s\":\"%s\"", name, value);
		bailIfError(FskMemPtrRealloc(FskStrLen(json) + FskStrLen(lineBuffer) + 4, &json));
		if (0 != i)
			FskStrCat(json, ",");
		FskStrCat(json, lineBuffer);
	}

	FskStrCat(json, "}");

bail:
	if (err)
		FskMemPtrDisposeAt(&json);
	*parsed = json;
	FskMemPtrDispose(lines);
	return err;
}

static FskErr parseListNetworksResults(char *response, char **parsed)
{
	#define kListNetworksResultsTemplate "{\"network_id\":%ld,\"ssid\":\"%s\",\"bssid\":\"%s\",\"flags\":\"%s\"}"
	FskErr err;
	UInt32 i, lineCount;
	char **lines = NULL;
	char *json = NULL;

	bailIfError(parseWPALines(response, &lines, &lineCount));
	bailIfError(FskMemPtrNew(4, &json));
	json[0] = '[';
	json[1] = 0;
	for (i = 1; i < lineCount; ++i) {
		char *network_id, *ssid, *bssid, *flags;
		SInt32 id;
		char lineBuffer[1024];
		char *walker;
		char *line = lines[i];
		network_id = walker = line;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		ssid = walker;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		bssid = walker;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		flags = walker;
		id = FskStrToNum(network_id);
		snprintf(lineBuffer, sizeof(lineBuffer), kListNetworksResultsTemplate, (unsigned long)id, ssid, bssid, flags);
		bailIfError(FskMemPtrRealloc(FskStrLen(json) + FskStrLen(lineBuffer) + 4, &json));
		if (1 != i)
			FskStrCat(json, ",");
		FskStrCat(json, lineBuffer);
	}
	FskStrCat(json, "]");

bail:
	if (err)
		FskMemPtrDisposeAt(&json);
	*parsed = json;
	FskMemPtrDispose(lines);
	return err;
}

static FskErr parseScanResults(char *response, char **parsed)
{
	#define kScanResultsTemplate "{\"bssid\":\"%s\",\"frequency\":%ld,\"signal_level\":%ld,\"flags\":\"%s\",\"ssid\":\"%s\"}"
	FskErr err;
	char **lines = NULL;
	char *json = NULL;
	UInt32 i, lineCount;

	bailIfError(parseWPALines(response, &lines, &lineCount));
	bailIfError(FskMemPtrNew(3, &json));
	json[0] = '[';
	json[1] = 0;
	for (i = 1; i < lineCount; ++i) {
		char *bssid, *frequency, *signal_level, *flags, *ssid;
		char lineBuffer[1024];
		char *walker;
		char *line = lines[i];
		bssid = walker = line;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		frequency = walker;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		signal_level = walker;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		flags = walker;
		while (*walker != '\t')
			++walker;
		*walker++ = 0;
		ssid = walker;

		// Don't include hidden networks
		if (0 == FskStrCompare("\\x00", ssid))
			continue;

		snprintf(lineBuffer, sizeof(lineBuffer), kScanResultsTemplate, bssid, (unsigned long)FskStrToNum(frequency), (unsigned long)FskStrToNum(signal_level), flags, ssid);
		bailIfError(FskMemPtrRealloc(FskStrLen(json) + FskStrLen(lineBuffer) + 4, &json));
		if (1 != i)
			FskStrCat(json, ",");
		FskStrCat(json, lineBuffer);
	}
	FskStrCat(json, "]");

bail:
	if (err)
		FskMemPtrDisposeAt(&json);
	*parsed = json;
	FskMemPtrDispose(lines);
	return err;
}

FskErr KprMacAddr(char* request, char* response, UInt32 responseSize, UInt32 *responseLength)
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) { /* handle error*/ };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */ }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        // fprintf(stderr, "mac addr ifr_name=%s\n", ifr.ifr_name);
        if (strcmp(ifr.ifr_name, "wlan0")==0) {
          if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
         }
        }
        else { /* handle error */ }
    }

    unsigned char mac_address[6];
    mac_address[4]  =0xff;
    mac_address[5]  =0xff;
    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
    // We only need the last two digits for the system name;
	sprintf(response, "{\"mac_addr\": \"%02x%02x\"}",mac_address[4], mac_address[5]);
	FskKprWiFiPrintfDebug("mac_addr %s", response);
	*responseLength = FskStrLen(response);
	return kFskErrNone;
}
FskErr KprWiFiLevel(char* request, char* response, UInt32 responseSize, UInt32 *responseLength)
{
	char line[1035];
	FILE *fp = NULL;
    FILE *pp = NULL;
	FskErr err = kFskErrNotFound;

 	fp = fopen("/proc/net/wireless", "r");

    if (fp != NULL){
        while (fgets(line, sizeof(line)-1, fp) != NULL) {
            char *linePtr = FskStrStripHeadSpace(line);
            // wlan0 for BG3CDP
            if (FskStrStr(linePtr, "mlan0") || FskStrStr(linePtr, "wlan0")) {
                char interface[32], status[32], link[32], level[32], noise[32];
                level[0] = 0;
                sscanf(linePtr, "%s %s %s %s %s", interface, status, link, level, noise);
                if (FskStrLen(level)) {
                    snprintf(response, responseSize, "{\"signal_level\": %ld}", (unsigned long)FskStrToNum(level));
                    FskKprWiFiPrintfDebug("LEVEL %s", response);
                    *responseLength = FskStrLen(response);
                    err = kFskErrNone;
                }
                break;
            }
        }
    }else{
        pp = popen("iw dev mlan0 link | grep signal", "r");
        if (pp != NULL){
            while (fgets(line, sizeof(line)-1, pp) != NULL){
                char level[32];
                level[0] = 0;
                char *linePtr = FskStrStripHeadSpace(line);
                sscanf(linePtr, "signal: %s dBm", level);
                if (FskStrLen(level)){
                    snprintf(response, responseSize, "{\"signal_level\": %ld}", (unsigned long)FskStrToNum(level));
                    FskKprWiFiPrintfDebug("LEVEL %s", response);
                    *responseLength = FskStrLen(response);
                    err = kFskErrNone;
                }
            }
        }
    }

	if (fp)
		fclose(fp);
    if (pp)
        pclose(pp);
	if (err)
		*responseLength = 0;
	return err;
}

FskErr KprWPACommand(char* request, char* response, UInt32 responseSize, UInt32* responseLength)
{
	FskErr err = kFskErrNone;
	struct wpa_ctrl* ctrl;
	struct timeval tv;
	int size = 0;
	fd_set rfds;
	char *json = NULL;

	FskMutexAcquire(gKprWPAMutex);
	ctrl = g_ctrl;

	FskKprWiFiPrintfDebug("REQUEST %s", request);
	bailIfNULL(ctrl);
	bailIfError(send(ctrl->s, request, FskStrLen(request), 0) < 0);

	for (;;) {
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(ctrl->s, &rfds);
		bailIfError(select(ctrl->s + 1, &rfds, NULL, NULL, &tv) < 0);
		bailIfError(!FD_ISSET(ctrl->s, &rfds));
		size = recv(ctrl->s, response, responseSize, 0);
		bailIfError(size < 0);
		if (response[0] == '<') {
			/* This is an unsolicited message from
			 * wpa_supplicant, not the response to the
			 * request. */
			FskKprWiFiPrintfDebug("WPA MESSAGE: %.*s", size, response);
			continue;
		}
		break;
	};
    response[size] = 0;
	*responseLength = (UInt32)size;

	if (!FskStrCompare(request, "STATUS")) {
		bailIfError(parseStatusResults(response, &json));
		FskStrCopy(response, json);
		*responseLength = FskStrLen(json);
	}
	else if (!FskStrCompare(request, "LIST_NETWORKS")) {
		bailIfError(parseListNetworksResults(response, &json));
		FskStrCopy(response, json);
		*responseLength = FskStrLen(json);
	}
	else if (!FskStrCompare(request, "SCAN_RESULTS")) {
		bailIfError(parseScanResults(response, &json));
		FskStrCopy(response, json);
		*responseLength = FskStrLen(json);
	}

	FskKprWiFiPrintfDebug("RESPONSE %s", response);
bail:
	FskMemPtrDispose(json);
	FskMutexRelease(gKprWPAMutex);
	return err;
}

FskErr KprWPAConfigure(FskAssociativeArray query, char* request, UInt32 requestSize, char* response, UInt32 responseSize)
{
	FskErr err = kFskErrNone;
	char* encryption = NULL;
	char* reset = NULL;
	char* adhoc = NULL;
	char* hidden = NULL;
	UInt32 length;
	long index = -1;
	UInt32 scan = 1;

	reset = FskAssociativeArrayElementGetString(query, "reset");
	if (!reset || !FskStrCompare(reset, "true")) {
		char* ptr;
		bailIfError(KprWPACommand("LIST_NETWORKS", response, responseSize, &length));
		for (ptr = FskStrStr(response, "\"network_id\":"); ptr; ptr = FskStrStr(ptr + 13, "\"network_id\":")) {
			snprintf(request, requestSize, "REMOVE_NETWORK %ld", (unsigned long)FskStrToNum(ptr + 13));
			bailIfError(KprWPACommand(request, response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2));
		}
	}
	bailIfError(KprWPACommand("ADD_NETWORK", response, responseSize, &length));
	index = (long)FskStrToNum(response);

	snprintf(request, requestSize, "DISABLE_NETWORK %ld", index);
	bailIfError(KprWPACommand(request, response, responseSize, &length));
	bailIfError(FskStrCompareWithLength(response, "OK", 2));

	snprintf(request, requestSize, "SET_NETWORK %ld ssid \"%s\"", index, FskAssociativeArrayElementGetString(query, "ssid"));
	bailIfError(KprWPACommand(request, response, responseSize, &length));
	bailIfError(FskStrCompareWithLength(response, "OK", 2));

	encryption = FskAssociativeArrayElementGetString(query, "encryption");
	adhoc = FskAssociativeArrayElementGetString(query, "adhoc");
	hidden = FskAssociativeArrayElementGetString(query, "hidden");

	if (!FskStrCompare(adhoc, "true")) {
		char* password = FskAssociativeArrayElementGetString(query, "password");
		scan = 2;
		snprintf(request, requestSize, "SET_NETWORK %ld mode 1", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
		snprintf(request, requestSize, "SET_NETWORK %ld key_mgmt NONE", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
		if (password) {
			snprintf(request, requestSize, "SET_NETWORK %ld wep_key0 %s", index, password);
			bailIfError(KprWPACommand(request, response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2));
			snprintf(request, requestSize, "SET_NETWORK %ld wep_tx_keyidx 0", index);
			bailIfError(KprWPACommand(request, response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2));
		}
	}
	else if (!encryption || !FskStrCompare(encryption, "WPA")) {
		char* password = FskAssociativeArrayElementGetString(query, "password");
		bailIfNULL(password);
		snprintf(request, requestSize, "SET_NETWORK %ld key_mgmt WPA-PSK", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
		snprintf(request, requestSize, "SET_NETWORK %ld psk \"%s\"", index, password);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
	}
	else if (!FskStrCompare(encryption, "WEP")) {
		char* password = FskAssociativeArrayElementGetString(query, "password");
		bailIfNULL(password);
		snprintf(request, requestSize, "SET_NETWORK %ld key_mgmt NONE", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
		snprintf(request, requestSize, "SET_NETWORK %ld wep_key0 %s", index, password);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
		snprintf(request, requestSize, "SET_NETWORK %ld wep_tx_keyidx 0", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
		snprintf(request, requestSize, "SET_NETWORK %ld auth_alg OPEN SHARED", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
	}
	else if (!FskStrCompare(encryption, "NONE")) {
		snprintf(request, requestSize, "SET_NETWORK %ld key_mgmt NONE", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
	}

	if (!FskStrCompare(hidden, "true")) {
		snprintf(request, requestSize, "SET_NETWORK %ld scan_ssid 1", index);
		bailIfError(KprWPACommand(request, response, responseSize, &length));
		bailIfError(FskStrCompareWithLength(response, "OK", 2));
	}

	snprintf(request, requestSize, "AP_SCAN %lu", (unsigned long)scan);
	bailIfError(KprWPACommand(request, response, responseSize, &length));
	bailIfError(FskStrCompareWithLength(response, "OK", 2));
	snprintf(request, requestSize, "SELECT_NETWORK %ld", index);
	bailIfError(KprWPACommand(request, response, responseSize, &length));
	bailIfError(FskStrCompareWithLength(response, "OK", 2));

	bailIfError(KprWPACommand("SAVE_CONFIG", response, responseSize, &length));
	bailIfError(FskStrCompareWithLength(response, "OK", 2));
bail:
	if (err) {
		if (index >= 0) {
			snprintf(request, requestSize, "REMOVE_NETWORK %ld", index);
			KprWPACommand(request, response, responseSize, &length);
		}
		KprWPACommand("RECONFIGURE", response, responseSize, &length);
	}
	response[0] = 0;
	return err;
}

FskErr KprWPAReset(FskAssociativeArray query, char* request, UInt32 requestSize, char* response, UInt32 responseSize)
{
	FskErr err = kFskErrNone;
	UInt32 length;
	do {
		bailIfError(KprWPACommand("REMOVE_NETWORK 0", response, responseSize, &length));
	}
	while (!FskStrCompareWithLength(response, "OK", 2));
	bailIfError(KprWPACommand("RECONFIGURE", response, responseSize, &length));
bail:
	response[0] = 0;
	return err;
}

FskErr KprAccessPoint(FskAssociativeArray query, char* request, UInt32 requestSize, char* response, UInt32 responseSize)
{
	FskErr err = kFskErrNone;
	char* action = FskAssociativeArrayElementGetString(query, "action");
	char* name = FskAssociativeArrayElementGetString(query, "name");
	bailIfNULL(action);
	if (action && name)
		snprintf(request, requestSize, "/modules/uaputl.sh %s %s", action, name);
	else if (action)
		snprintf(request, requestSize, "/modules/uaputl.sh %s", action);
	system(request);
bail:
	response[0] = 0;
	return err;
}

#else

#include "FskNetInterface.h"

void KprWPAStart()
{
}

void KprWPAStop()
{
}

static char* gWPAResponses[] = {
	"{\"bssid\":\"00:11:22:33:44:55\", \"ssid\":\"Kinoma\", \"id\":0, \"mode\":\"station\", \"pairwise_cipher\":\"CCMP\", \"group_cipher\":\"TKIP\", \"key_mgmt\":\"WPA2-PSK\", \"wpa_state\":\"COMPLETED\", \"ip_address\":\"%s\", \"address\":\"66:55:44:33:22:11\"}",

	"[{\"network_id\":0, \"ssid\": \"Kinoma\", \"bssid\":\"any\", \"flags\":\"[CURRENT]\"}]",

	"[{\"bssid\": \"00:11:22:33:44:55\",\"frequency\": 2422, \"signal_level\": -62, \"flags\": \"[ESS]\", \"ssid\":\"Kinoma\"},\
	  {\"bssid\": \"55:66:77:88:99:AA\",\"frequency\": 2422, \"signal_level\": -81, \"flags\": \"[WPA2-PSK-CCMP][ESS]\", \"ssid\":\"KinomaWPA\"},\
	  {\"bssid\": \"55:66:77:88:99:AA\",\"frequency\": 2422, \"signal_level\": -64, \"flags\": \"[IBSS]\", \"ssid\":\"Brian's MacBook Air\"},\
	  {\"bssid\": \"55:66:77:88:99:AA\",\"frequency\": 2422, \"signal_level\": -64, \"flags\": \"[WEP][IBSS]\", \"ssid\":\"Mike's laptop\"},\
	  {\"bssid\": \"AA:BB:CC:DD:EE:FF\",\"frequency\": 2422, \"signal_level\": -83, \"flags\": \"[WEP][ESS]\", \"ssid\":\"KBLDG3\"},\
	  {\"bssid\": \"7c:d1:c3:d1:56:68\",\"frequency\": 2452, \"signal_level\": -50, \"flags\": \"[WPA2-PSK-CCMP][ESS]\", \"ssid\":\"kdevice\"},\
      {\"bssid\": \"66:77:88:99:AA:BB\",\"frequency\": 2422, \"signal_level\": -70, \"flags\": \"[ESS]\", \"ssid\":\"Mayfield Bakery Public\"}]"
};

FskErr KprWPACommand(char* request, char* response, UInt32 responseSize, UInt32* responseLength)
{
	FskErr err = kFskErrNone;
	FskKprWiFiPrintfDebug("REQUEST %s", request);
	if (!FskStrCompare(request, "STATUS")) {
		UInt32 i, count = FskNetInterfaceEnumerate();
		for (i = 0; i < count; i++) {
			char ipAddr[32];
			FskNetInterface iface;
			FskNetInterfaceDescribe(i, &iface);
			if (iface->ip && (0x7f000001 != iface->ip) && iface->status) {
				FskNetIPandPortToString(iface->ip, 0, ipAddr);
				snprintf(response, responseSize, gWPAResponses[0], ipAddr);
				FskNetInterfaceDescriptionDispose(iface);
				goto bail;
			}
			FskNetInterfaceDescriptionDispose(iface);
		}
		snprintf(response, responseSize, gWPAResponses[0], "192.168.0.2");	// we need something for the mockup
	}
	else if (!FskStrCompare(request, "LIST_NETWORKS")) {
		FskStrCopy(response, gWPAResponses[1]);
	}
	else if (!FskStrCompare(request, "SCAN_RESULTS")) {
		FskStrCopy(response, gWPAResponses[2]);
	}
	else {
		response[0] = 0;
		err = kFskErrUnimplemented;
	}
bail:
	FskKprWiFiPrintfDebug("RESPONSE %s", response);
	*responseLength = FskStrLen(response);
	return err;
}

FskErr KprWPAConfigure(FskAssociativeArray query, char* request, UInt32 requestSize, char* response, UInt32 responseSize)
{
	response[0] = 0;
	return kFskErrUnimplemented;
}

FskErr KprAccessPoint(FskAssociativeArray query, char* request, UInt32 requestSize, char* response, UInt32 responseSize)
{
	response[0] = 0;
	return kFskErrUnimplemented;
}


FskErr KprWiFiLevel(char* request, char* response, UInt32 responseSize, UInt32 *responseLength)
{
	FskStrCopy(response, "{\"signal_level\": -64}");
	FskKprWiFiPrintfDebug("LEVEL %s", response);
	*responseLength = FskStrLen(response);
	return kFskErrNone;
}

FskErr KprWPAReset(FskAssociativeArray query, char* request, UInt32 requestSize, char* response, UInt32 responseSize)
{
	response[0] = 0;
	return kFskErrUnimplemented;
}

FskErr KprMacAddr(char* request, char* response, UInt32 responseSize, UInt32 *responseLength)
{
	response[0] = 0;
	return kFskErrUnimplemented;
}

#endif

//--------------------------------------------------
// EXTENSION
//--------------------------------------------------

FskExport(FskErr) kprWiFi_fskLoad(FskLibrary it)
{
	KprServiceRegister(&gWiFiService);
	return kFskErrNone;
}

FskExport(FskErr) kprWiFi_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void KprWiFiStart(KprService service, FskThread thread, xsMachine* the)
{
	service->machine = the;
	service->thread = thread;
    FskMutexNew(&gKprWPAMutex, "WPA");
	KprWPAStart();
}

void KprWiFiStop(KprService service UNUSED)
{
	KprWPAStop();
	FskMutexDispose(gKprWPAMutex);
}

//--------------------------------------------------
// WiFi
//--------------------------------------------------

void KprWiFiInvoke(KprService service, KprMessage message)
{
	FskErr err = kFskErrNone;

	if (KprMessageContinue(message)) {
		UInt32 requestSize = kRequestSize;
		UInt32 responseSize = kResponseSize;
		char request[kRequestSize];
		char response[kResponseSize];
		UInt32 length = 0;
		FskAssociativeArray query = NULL;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryBegin, message)
        // fprintf(stderr, "KprWifiInvoke  name=%s\n", message->parts.name);
		if (message->parts.query)
			bailIfError(KprQueryParse(message->parts.query, &query));
		if (FskStrCompareWithLength("status", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWPACommand("STATUS", response, responseSize, &length));
		}
		else if (FskStrCompareWithLength("list", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWPACommand("LIST_NETWORKS", response, responseSize, &length));
		}
		else if (FskStrCompareWithLength("disable", message->parts.name, message->parts.nameLength) == 0) {
			// query:
			// + network = <id>
			snprintf(request, requestSize, "DISABLE_NETWORK %s", FskAssociativeArrayElementGetString(query, "network"));
			bailIfError(KprWPACommand(request, response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2) != 0);
		}
		else if (FskStrCompareWithLength("enable", message->parts.name, message->parts.nameLength) == 0) {
			// query:
			// + network = <id>
			snprintf(request, requestSize, "ENABLE_NETWORK %s", FskAssociativeArrayElementGetString(query, "network"));
			bailIfError(KprWPACommand(request, response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2) != 0);
		}
		else if (FskStrCompareWithLength("remove", message->parts.name, message->parts.nameLength) == 0) {
			// query:
			// + network = <id>
			snprintf(request, requestSize, "REMOVE_NETWORK %s", FskAssociativeArrayElementGetString(query, "network"));
			bailIfError(KprWPACommand(request, response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2) != 0);
		}
		else if (FskStrCompareWithLength("reset", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWPAReset(query, request, requestSize, response, responseSize));
		}
		else if (FskStrCompareWithLength("scan", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWPACommand("SCAN", response, responseSize, &length));
		}
		else if (FskStrCompareWithLength("scanned", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWPACommand("SCAN_RESULTS", response, responseSize, &length));
		}
		else if (FskStrCompareWithLength("reconfigure", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWPACommand("RECONFIGURE", response, responseSize, &length));
			bailIfError(FskStrCompareWithLength(response, "OK", 2) != 0);
		}
		else if (FskStrCompareWithLength("configure", message->parts.name, message->parts.nameLength) == 0) {
			// query:
			// + ssid = <text>
			// + password =  <text> (optional, default is "")
			// + encryption = "NONE" | "WPA" | "WEP" (optional, default is "WPA")
			// + reset = "true" | "false" (optional, default is "true")
			// + adhoc = "true" | "false" (optional, default is "false")
			bailIfError(KprWPAConfigure(query, request, requestSize, response, responseSize));
		}
		else if (FskStrCompareWithLength("ap", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprAccessPoint(query, request, requestSize, response, responseSize));
		}
		else if (FskStrCompareWithLength("level", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprWiFiLevel(request, response, responseSize, &length));
		}
		else if (FskStrCompareWithLength("mac_addr", message->parts.name, message->parts.nameLength) == 0) {
			bailIfError(KprMacAddr(request, response, responseSize, &length));
		}
		else if (FskStrCompareWithLength("p2p_start", message->parts.name, message->parts.nameLength) == 0) {
            fprintf(stderr, "p2p_start: \n");
            #ifdef BG3CDP
			    int ret = system("system/wifi.sh p2p_start");
			    sprintf(response, "{\"status\": %d}", ret);
            #else
			    FskStrCopy(response, "{\"status\": \"Error: command unsupported in non-bg3.\"}");
            #endif
			length = FskStrLen(response);
            fprintf(stderr, "p2p_start: response=%s\n", response);
		}
		else if (FskStrCompareWithLength("p2p_stop", message->parts.name, message->parts.nameLength) == 0) {
            #ifdef BG3CDP
			    int ret = system("system/wifi.sh p2p_stop");
			    sprintf(response, "{\"status\": %d}", ret);
            #else
			    FskStrCopy(response, "{\"status\": \"Error: command unsupported in non-bg3.\"}");
            #endif
			length = FskStrLen(response);
            fprintf(stderr, "p2p_stop: response=%s\n", response);
		}
		else if (FskStrCompareWithLength("udhcpc_restart", message->parts.name, message->parts.nameLength) == 0) {
			fprintf(stderr, "kprWiFi ===== > udhcpc_restart: \n");
            #ifdef BG3CDP
			    int ret = system("system/wifi.sh udhcpc_restart");
			    sprintf(response, "{\"status\": %d}", ret);
            #else
			    FskStrCopy(response, "{\"status\": \"Error: command unsupported in non-bg3.\"}");
            #endif
			length = FskStrLen(response);
            fprintf(stderr, "kprWiFi ===== > udhcpc_restart: response=%s\n", response);
		}
		else {
			BAIL(kFskErrNotFound);
		}
        // fprintf(stderr, "KprWifiInvoke  response=%s\n", response);
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
        KprMessageTransform(message, gWiFiService.machine);
		KprMessageComplete(message);
	}
}

void WPA_start(xsMachine *the)
{
	KprWPAStart();
}

void WPA_stop(xsMachine *the)
{
	KprWPAStop();
}
