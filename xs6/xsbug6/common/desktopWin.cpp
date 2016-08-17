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

void KPR_Files_toPath(xsMachine *the)
{
	char* path;
	char* slash;
	char c;
	xsThrowIfFskErr(KprURLToPath(xsToString(xsArg(0)), &path));
	slash = path;
	while ((c = *slash)) {
		if (c == '/')
			*slash = '\\';
		slash++;
	}
	xsResult = xsString(path);
	FskMemPtrDispose(path);
}

void KPR_Files_toURI(xsMachine *the)
{
	char* path = xsToString(xsArg(0));
	char* slash;
	char c;
	FskErr err;
	char* url;
	slash = path;
	while ((c = *slash)) {
		if (c == '\\')
			*slash = '/';
		slash++;
	}
	err = KprPathToURL(path, &url);
	slash = path;
	while ((c = *slash)) {
		if (c == '/')
			*slash = '\\';
		slash++;
	}
	xsThrowIfFskErr(err);
	xsResult = xsString(url);
	FskMemPtrDispose(url);
}

void KPR_system_gotoFront(xsMachine *the)
{
}

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

