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
#include <winsock2.h>
#undef WINVER
#include "FskMain.h"
#include "FskTextConvert.h"
#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHTTPClient.h"
#include "kprShell.h"
#include "kprURL.h"
#include <shobjidl.h> 
#include <wlanapi.h> 
#include <iphlpapi.h>

void KPR_system_getClipboardText(xsMachine* the)
{
	HWND window = (HWND)FskWindowGetNativeWindow(gShell->window);
	HGLOBAL data;
	PWSTR wideString;
	xsIntegerValue length;
	xsStringValue buffer, src, dst;
	char c;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (OpenClipboard(window)) {
			data = GetClipboardData(CF_UNICODETEXT);
			wideString = (PWSTR)GlobalLock(data);
			if (wideString) {
				length = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
				if (FskMemPtrNew(length, &buffer) == kFskErrNone) {
					WideCharToMultiByte(CP_UTF8, 0, wideString, -1, buffer, length, NULL, NULL);
					src = buffer;
					dst = buffer;
					while ((c = *src++)) {
						if ((c == 13) && (*src == 10)) {
							*dst++ = 10;
							src++;
						}
						else
							*dst++ = c;
					}
					*dst = 0;
					xsResult = xsString(buffer);
					FskMemPtrDispose(buffer);
				}
				GlobalUnlock(wideString);
			}
			CloseClipboard();
		}
	}
	if (!xsTest(xsResult))
		xsResult = xsString("");
}

void KPR_system_setClipboardText(xsMachine* the)
{
	HWND window = (HWND)FskWindowGetNativeWindow(gShell->window);
	xsStringValue string, src, buffer, dst;
	char c;
	xsIntegerValue length;
	HGLOBAL data;
	if (OpenClipboard(window)) {
		EmptyClipboard(); 
		if (xsToInteger(xsArgc) > 0) {
			string = xsToString(xsArg(0));
			src = string;
			length = 0;
			while ((c = *src++)) {
				length++;
				if (c == 10)
					length++;
			}
			length++;
			if (FskMemPtrNew(length, &buffer) == kFskErrNone) {
				src = string;
				dst = buffer;
				while ((c = *src++)) {
					if (c == 10)
						*dst++ = 13;
					*dst++ = c;
				}
				*dst = 0;
				length = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
				data = GlobalAlloc(GMEM_MOVEABLE, 2 * length);
				if (data) {
					MultiByteToWideChar(CP_UTF8, 0, buffer, -1, (LPWSTR)GlobalLock(data), length);
					GlobalUnlock(data);
					SetClipboardData(CF_UNICODETEXT, data);
				}
				FskMemPtrDispose(buffer);
			}
		}
		CloseClipboard();
	}
}

static PWSTR xsStringToWideString(xsStringValue string)
{
	xsIntegerValue stringLength;
	xsIntegerValue wideStringLength;
	stringLength = FskStrLen(string);
	PWSTR wideString;
	wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	wideString = (PWSTR)CoTaskMemAlloc(wideStringLength * 2);
	if (wideString) {
		MultiByteToWideChar(CP_UTF8, 0, string, -1, wideString, wideStringLength);
		//wideString[wideStringLength - 1] = 0;
	}
	return wideString;
}
	

void KPR_system_alert(xsMachine* the)
{
	int argc = xsToInteger(xsArgc);
	MSGBOXPARAMSW params;
	xsStringValue string;
	xsIntegerValue result;
	xsVars(1);
	params.cbSize = sizeof(params);
	params.hwndOwner = NULL;
	params.hInstance = FskMainGetHInstance();
	params.lpszText = NULL;
	params.lpszCaption = xsStringToWideString("Kinoma Code");
	params.dwStyle = MB_ICONSTOP;
	params.dwContextHelpId = 0;
	params.lpfnMsgBoxCallback = NULL;
	params.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_type, &string)) {
			if (!FskStrCompare(string, "about"))
				params.dwStyle = MB_ICONINFORMATION;
			else if (!FskStrCompare(string, "stop"))
				params.dwStyle = MB_ICONSTOP;
			else if (!FskStrCompare(string, "note"))
				params.dwStyle = MB_ICONEXCLAMATION;
		}
		if (xsFindResult(xsArg(0), xsID_prompt)) {
			xsVar(0) = xsResult;
		}
		if (xsFindResult(xsArg(0), xsID_info)) {
			xsVar(0) = xsCall1(xsVar(0), xsID("concat"), xsString("\n\n"));
			xsVar(0) = xsCall1(xsVar(0), xsID("concat"), xsResult);
		}
		params.lpszText = xsStringToWideString(xsToString(xsVar(0)));
		if (xsFindResult(xsArg(0), xsID_buttons)) {
			if (xsIsInstanceOf(xsResult, xsArrayPrototype)) {
				xsIntegerValue c = xsToInteger(xsGet(xsResult, xsID_length));
				if (c == 3)
					params.dwStyle |= MB_YESNOCANCEL;
				else if (c == 2)
					params.dwStyle |= MB_OKCANCEL;
				else
					params.dwStyle |= MB_OK;
			}
		}
	}
	result = MessageBoxIndirectW(&params);
	if (params.lpszText)
		CoTaskMemFree((LPVOID *)params.lpszText);
	if (params.lpszCaption)
		CoTaskMemFree((LPVOID *)params.lpszCaption);
	if ((argc > 1) && xsTest(xsArg(1)))
		(void)xsCallFunction1(xsArg(1), xsNull, ((result == IDYES) || (result == IDOK)) ? xsTrue : (result == IDNO) ? xsFalse : xsUndefined);
}

void KPR_system_beginModal(xsMachine* the)
{
}

void KPR_system_endModal(xsMachine* the)
{
}

void KPR_system_openDirectory(xsMachine* the)
{
	HRESULT hr;
	xsIntegerValue argc = xsToInteger(xsArgc);
	IFileOpenDialog *pFileOpen = NULL;
	IShellItem *pItem = NULL;
	xsStringValue string;
	PWSTR wideString = NULL;
	xsIntegerValue urlLength;
	xsStringValue url = NULL;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (LPVOID *)&pFileOpen);
	if (!SUCCEEDED(hr)) goto bail;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_prompt, &string)) {
			wideString = xsStringToWideString(string);
			hr = pFileOpen->SetTitle(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
		}
		if (xsFindString(xsArg(0), xsID_message, &string)) {
			wideString = xsStringToWideString(string);
			hr = pFileOpen->SetTitle(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
		}
		if (xsFindString(xsArg(0), xsID_url, &string)) {
			wideString = xsStringToWideString(string);
			hr = SHCreateItemFromParsingName(wideString, NULL, IID_IShellItem, (LPVOID *)&pItem);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
			hr = pFileOpen->SetFolder(pItem);
			if (!SUCCEEDED(hr)) goto bail;
			pItem->Release();
			pItem = NULL;
		}
	}
	hr = pFileOpen->SetOptions(FOS_PICKFOLDERS);
	if (!SUCCEEDED(hr)) goto bail;
	hr = pFileOpen->Show(GetForegroundWindow());
	if (!SUCCEEDED(hr)) goto bail;
	hr = pFileOpen->GetResult(&pItem);
	if (!SUCCEEDED(hr)) goto bail;
	hr = pItem->GetDisplayName(SIGDN_URL, &wideString);
	if (!SUCCEEDED(hr)) goto bail;
	
	urlLength = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
	FskMemPtrNew(urlLength + 1, &url);
	WideCharToMultiByte(CP_UTF8, 0, wideString, -1, url, urlLength, NULL, NULL);
	url[urlLength - 1] = '/';
	url[urlLength] = 0;
	(void)xsCallFunction1(xsArg(1), xsNull, xsString(url));
	
bail:
	FskMemPtrDispose(url);
	if (wideString)
		CoTaskMemFree(wideString);
	if (pItem)
		pItem->Release();
	if (pFileOpen)
		pFileOpen->Release();
}

void KPR_system_openFile(xsMachine* the)
{
	FskErr err;
	xsIntegerValue argc = xsToInteger(xsArgc);
	xsStringValue prompt = NULL;
	xsStringValue initialPath = NULL;
	xsStringValue path = NULL;
	xsStringValue url = NULL;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindResult(xsArg(0), xsID_prompt))
			prompt = xsToStringCopy(xsResult);
		if (xsFindResult(xsArg(0), xsID_url))
			KprURLToPath(xsToString(xsResult), &initialPath);
	}
	err = FskFileChoose(NULL, prompt, false, initialPath, &path);
	if ((kFskErrNone == err) && (NULL != path)) {
		KprPathToURL(path, &url);
		(void)xsCallFunction1(xsArg(1), xsNull, xsString(url));
	}
	FskMemPtrDispose(url);
	FskMemPtrDispose(path);
	FskMemPtrDispose(initialPath);
	FskMemPtrDispose(prompt);
}

void KPR_system_saveDirectory(xsMachine* the)
{
}

void KPR_system_saveFile(xsMachine* the)
{
	FskErr err;
	xsIntegerValue argc = xsToInteger(xsArgc);
	xsStringValue name = NULL;
	xsStringValue prompt = NULL;
	xsStringValue initialPath = NULL;
	xsStringValue path = NULL;
	xsStringValue url = NULL;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindResult(xsArg(0), xsID_name))
			name = xsToStringCopy(xsResult);
		if (xsFindResult(xsArg(0), xsID_prompt))
			prompt = xsToStringCopy(xsResult);
		if (xsFindResult(xsArg(0), xsID_url))
			KprURLToPath(xsToString(xsResult), &initialPath);
	}
	err = FskFileChooseSave(name, prompt, initialPath, &path);
	if ((kFskErrNone == err) && (NULL != path)) {
		KprPathToURL(path, &url);
		(void)xsCallFunction1(xsArg(1), xsNull, xsString(url));
	}
	FskMemPtrDispose(url);
	FskMemPtrDispose(path);
	FskMemPtrDispose(initialPath);
	FskMemPtrDispose(prompt);
	FskMemPtrDispose(name);
}

typedef struct {
	xsMachine* the;
	xsSlot slot;
	HANDLE hReadPipe;
	HANDLE hWritePipe;
	PROCESS_INFORMATION pi; 
	xsIntegerValue status;
	xsIntegerValue usage;
} KprShellExecRecord, *KprShellExec;

static void KPR_shell_execute_async(KprShellExec exec);
static void KPR_shell_execute_callback(KprShellExec exec);
static void KPR_shell_execute_cancel(xsMachine* the);
static void KPR_shell_execute_destructor(void* data);
static void KPR_shell_execute_stderr(KprShellExec exec, xsStringValue string);
static void KPR_shell_execute_stdout(KprShellExec exec, xsStringValue string);

void KPR_shell_execute_async(KprShellExec exec)
{
#define BUFSIZE 4096
	char buffer[BUFSIZE];
	DWORD result = 0;
	BOOL bSuccess = FALSE; 

	CloseHandle(exec->hWritePipe);
	exec->hWritePipe = NULL;
	for (;;)  {
		if (!ReadFile(exec->hReadPipe, buffer, BUFSIZE, &result, NULL) || (result == 0))
			break;
		buffer[result] = 0;
		exec->usage++;
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KPR_shell_execute_stderr, exec, FskStrDoCopy(buffer), NULL, NULL);
	} 
	WaitForSingleObject(exec->pi.hProcess, INFINITE);
	GetExitCodeProcess(exec->pi.hProcess, &result);
	exec->status = (xsIntegerValue)result;
	CloseHandle(exec->hReadPipe);
	exec->hReadPipe = NULL;
	CloseHandle(exec->pi.hProcess);
	exec->pi.hProcess = NULL;
	CloseHandle(exec->pi.hThread);
	exec->pi.hThread = NULL;
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KPR_shell_execute_callback, exec, NULL, NULL, NULL);
}

void KPR_shell_execute_callback(KprShellExec exec)
{
	exec->usage--;
	if (exec->usage == 0) {
		xsBeginHost(exec->the);
		xsVars(3);
		xsVar(0) = xsAccess(exec->slot);
		xsVar(1) = xsGet(xsVar(0), xsID_callback);
		if (xsTest(xsVar(1))) {
			xsVar(2) = xsInteger(exec->status);
			xsResult = xsCallFunction1(xsVar(1), xsNull, xsVar(2));
		}
		xsForget(exec->slot);
		xsSetHostData(xsVar(0), NULL);
		xsEndHost(exec->the);
		FskMemPtrDispose(exec);
	}
}

void KPR_shell_execute_cancel(xsMachine* the)
{
	KprShellExec exec = (KprShellExec)xsGetHostData(xsThis);
	if (exec->pi.hProcess)
		TerminateProcess(exec->pi.hProcess, 0);
}

void KPR_shell_execute_destructor(void* data)
{
	// never
}

void KPR_shell_execute_stderr(KprShellExec exec, xsStringValue string)
{
	if (string) {
		xsBeginHost(exec->the);
		xsVars(3);
		xsVar(0) = xsAccess(exec->slot);
		xsVar(1) = xsGet(xsVar(0), xsID_stderr);
		if (xsTest(xsVar(1))) {
			xsVar(2) = xsString(string);
			xsResult = xsCallFunction1(xsVar(1), xsNull, xsVar(2));
		}
		xsEndHost(exec->the);
		FskMemPtrDispose(string);
	}
	KPR_shell_execute_callback(exec);
}

void KPR_shell_execute_stdout(KprShellExec exec, xsStringValue string)
{
	if (string) {
		xsBeginHost(exec->the);
		xsVars(3);
		xsVar(0) = xsAccess(exec->slot);
		xsVar(1) = xsGet(xsVar(0), xsID_stdout);
		if (xsTest(xsVar(1))) {
			xsVar(2) = xsString(string);
			xsResult = xsCallFunction1(xsVar(1), xsNull, xsVar(2));
		}
		xsEndHost(exec->the);
		FskMemPtrDispose(string);
	}
	KPR_shell_execute_callback(exec);
}

void KPR_shell_execute(xsMachine* the)
{
	xsStringValue application = NULL;
	xsStringValue command = NULL;
	xsStringValue directory = NULL;
	xsStringValue environment = NULL;
	xsStringValue string;
	xsIntegerValue length;
	KprShellExec exec = NULL;
	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa; 
	xsVars(5);
	xsTry {
		application = getenv("COMSPEC");
		if (!application)
			xsThrowIfFskErr(kFskErrOperationFailed);
		
		string = xsToString(xsArg(0));
		length = FskStrLen(string) + 1;
		xsThrowIfFskErr(FskMemPtrNew(3 + length, &command));
		memcpy(command, "/c ", 3);
		memcpy(command + 3, string, length);

		if (xsFindString(xsArg(1), xsID_directory, &string)) {
			length = FskStrLen(string) + 1;
			xsThrowIfFskErr(FskMemPtrNew(length, &directory));
			memcpy(directory, string, length);
		}
		if (xsFindResult(xsArg(1), xsID_environment)) {
			xsIntegerValue total = 0, length;
			xsVar(1) = xsEnumerate(xsResult);
			for (;;) {
				xsVar(2) = xsCall0(xsVar(1), xsID("next"));
				xsVar(3) = xsGet(xsVar(2), xsID("done"));
				if (xsTest(xsVar(3)))
					break;
				xsVar(3) = xsGet(xsVar(2), xsID("value"));
				xsVar(4) = xsGetAt(xsResult, xsVar(3));
				total += FskStrLen(xsToString(xsVar(3)));
				total++;
				total += FskStrLen(xsToString(xsVar(4)));
				total++;
			}
			total++;
			xsThrowIfFskErr(FskMemPtrNew(total, &environment));
			total = 0;
			xsVar(1) = xsEnumerate(xsResult);
			for (;;) {
				xsVar(2) = xsCall0(xsVar(1), xsID("next"));
				xsVar(3) = xsGet(xsVar(2), xsID("done"));
				if (xsTest(xsVar(3)))
					break;
				xsVar(3) = xsGet(xsVar(2), xsID("value"));
				xsVar(4) = xsGetAt(xsResult, xsVar(3));
				string = xsToString(xsVar(3));
				length = FskStrLen(string);
				memcpy(environment + total, string, length);
				total += length;
				environment[total++] = '=';
				string = xsToString(xsVar(4));
				length = FskStrLen(string);
				memcpy(environment + total, string, length);
				total += length;
				environment[total++] = 0;
			}
			environment[total++] = 0;
		}
		
		xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprShellExecRecord), &exec));
		xsVar(0) = xsNewHostObject(KPR_shell_execute_destructor);
		exec->the = the;
		exec->slot = xsVar(0);
		xsSetHostData(xsVar(0), exec);
		
		xsResult = xsNewHostFunction(KPR_shell_execute_cancel, 0);
		xsNewHostProperty(xsVar(0), xsID_cancel, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		if (xsFindResult(xsArg(1), xsID_callback)) {
			xsNewHostProperty(xsVar(0), xsID_callback, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		else {
			xsNewHostProperty(xsVar(0), xsID_callback, xsNull, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		if (xsFindResult(xsArg(1), xsID_stderr)) {
			xsNewHostProperty(xsVar(0), xsID_stderr, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		else {
			xsNewHostProperty(xsVar(0), xsID_stderr, xsNull, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		if (xsFindResult(xsArg(1), xsID_stdout)) {
			xsNewHostProperty(xsVar(0), xsID_stdout, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		else {
			xsNewHostProperty(xsVar(0), xsID_stdout, xsNull, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		
		sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
		sa.bInheritHandle = TRUE; 
		sa.lpSecurityDescriptor = NULL; 
		if (!CreatePipe(&(exec->hReadPipe), &(exec->hWritePipe), &sa, 0))
			xsThrowIfFskErr(kFskErrOperationFailed);
		if (!SetHandleInformation(exec->hReadPipe, HANDLE_FLAG_INHERIT, 0))
			xsThrowIfFskErr(kFskErrOperationFailed);
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(STARTUPINFO); 
		si.hStdError = exec->hWritePipe;
		si.hStdOutput = exec->hWritePipe;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.dwFlags = STARTF_USESTDHANDLES;
		if (!CreateProcess(application, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, environment, directory, &si, &(exec->pi)))
			xsThrowIfFskErr(kFskErrOperationFailed);
			
		xsRemember(exec->slot);
		exec->usage++;
		FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KPR_shell_execute_async, exec, NULL, NULL, NULL);
		FskMemPtrDispose(environment);
		FskMemPtrDispose(directory);
		FskMemPtrDispose(command);
	}
	xsCatch {
		if (exec) {
			if (exec->pi.hProcess)
				CloseHandle(exec->pi.hProcess);
			if (exec->pi.hThread)
				CloseHandle(exec->pi.hThread);
			if (exec->hWritePipe)
				CloseHandle(exec->hWritePipe);
			if (exec->hReadPipe)
				CloseHandle(exec->hReadPipe);
			FskMemPtrDispose(exec);
		}
		FskMemPtrDispose(environment);
		FskMemPtrDispose(directory);
		FskMemPtrDispose(command);
		xsThrow(xsException);
	}
}

void KPR_shell_splitError(xsMachine* the)
{
	xsStringValue string, p, q;
	char c;
	xsIndex ids[4];
	size_t offsets[4], lengths[4];
	int i;
	xsVars(1);
	string = p = xsToString(xsArg(0));
	ids[0] = xsID_path;
	offsets[0] = p - string;
	c = *p;
	if (c == '/')
		p = FskStrChr(p, ':');
	else if (('A' <= c) && (c <= 'Z') && (*(p + 1) == ':'))
		p = FskStrChr(p + 2, ':');
	else
		goto bail;
	if (!p) goto bail;
	ids[1] = xsID_line;
	q = p - 1;
	c = *q;
	if (c == ')') {
		q--;
		while ((q > string) && ((c = *q)) && ('0' <= c) && (c <= '9'))
			q--;
		if (c != '(') goto bail;
		lengths[0] = q - string;
		offsets[1] = q + 1 - string;
		lengths[1] = (p - q) - 2;
	}
	else {
		lengths[0] = p - string;
		p++;
		offsets[1] = p - string;
		while (((c = *p)) && ('0' <= c) && (c <= '9'))
			p++;
		if (c != ':') goto bail;
		lengths[1] = (p - string) - offsets[1];
	}
	p++;
	c  = *p;
	if (('0' <= c) && (c <= '9')) {
		p++;
		while (((c = *p)) && ('0' <= c) && (c <= '9'))
			p++;
		if (c != ':') goto bail;
		p++;
		c  = *p;
	}
	if (c != ' ') goto bail;
	p++;
	ids[2] = xsID_kind;
	offsets[2] = p - string;
	p = FskStrChr(p, ':');
	if (!p) goto bail;
	lengths[2] = (p - string) - offsets[2];
	p++;
	c = *p;
	if (c != ' ') goto bail;
	p++;
	ids[3] = xsID_reason;
	offsets[3] = p - string;
	lengths[3] = FskStrLen(p);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	for (i = 0; i < 4; i++) {
		xsVar(0) = xsStringBuffer(NULL, lengths[i]);
		string = xsToString(xsArg(0));
		FskMemCopy(xsToString(xsVar(0)), string + offsets[i], lengths[i]);
		xsNewHostProperty(xsResult, ids[i], xsVar(0), xsDefault, xsDontScript);
	}
bail:
	return;
}

void KPR_system_getWifiInfo(xsMachine* the)
{
    DWORD dwResult = 0;
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2; 
    DWORD dwCurVersion = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    int i;
    PWLAN_INTERFACE_INFO pIfInfo = NULL;
    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
    ULONG length;
	xsVars(1);
    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient); 
    if (dwResult != ERROR_SUCCESS) 
    	goto bail;
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList); 
    if (dwResult != ERROR_SUCCESS)
    	goto bail;
    for (i = 0; i < (int) pIfList->dwNumberOfItems; i++) {
		pIfInfo = (WLAN_INTERFACE_INFO *) &pIfList->InterfaceInfo[i];
   		if (pIfInfo->isState == wlan_interface_state_connected) {
			dwResult = WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid,
										  wlan_intf_opcode_current_connection,
										  NULL,
										  &connectInfoSize,
										  (PVOID *) &pConnectInfo, 
										  &opCode);
			if (dwResult != ERROR_SUCCESS)
				goto bail;
			length = pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength;
			if (length > 0) {
				xsResult = xsNewInstanceOf(xsObjectPrototype);
				xsVar(0) = xsStringBuffer(NULL, length + 1);
				FskMemCopy(xsToString(xsVar(0)), pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID, length);
				xsSet(xsResult, xsID("SSID"), xsVar(0));
			}
   			break;
   		}
   	}
bail:
    if (pConnectInfo != NULL) {
        WlanFreeMemory(pConnectInfo);
        pConnectInfo = NULL;
    }
    if (pIfList != NULL) {
        WlanFreeMemory(pIfList);
        pIfList = NULL;
    }
    if (hClient != NULL) {
        WlanCloseHandle(hClient, NULL);
        hClient = NULL;
    }
}

void KPR_system_networkInterfaceIndexToName(xsMachine* the)
{
	SInt32 index = xsToInteger(xsArg(0));
	PIP_ADAPTER_ADDRESSES pAddresses = NULL, pAddress;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	ULONG family = AF_INET;
	ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
	BAIL_IF_ERR(FskMemPtrNewClear(outBufLen, &pAddresses));
	if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
		FskMemPtrDispose(pAddresses);
		BAIL_IF_ERR(FskMemPtrNewClear(outBufLen, &pAddresses));
	}
	if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == NO_ERROR) {
		for (pAddress = pAddresses; pAddress; pAddress = pAddress->Next)
			if (pAddress->IfIndex == index) break;
	}
	if (pAddress)
		xsResult = xsString(pAddress->AdapterName);
bail:
	FskMemPtrDispose(pAddresses);
	return;
}

