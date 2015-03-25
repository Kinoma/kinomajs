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
#ifdef __cplusplus
extern "C" {
#endif
#define __FSKWINDOW_PRIV__
#include "Fsk.h"
#include "FskUtilities.h"
#include "FskErrors.h"
#include "FskDragDropWin.h"
#include "FskPlatformImplementation.h"

#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <oleidl.h>
#include <objidl.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <shlobj.h>

class CDropTarget : public IDropTarget
{
public:
	CDropTarget();
	~CDropTarget();

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) {
		*ppvObject = NULL;
		if( IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget) ) {
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() { return ++m_dwRef;	}
	STDMETHOD_(ULONG, Release)() { return --m_dwRef; }

	// drop target methods
	STDMETHOD(DragEnter)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);


public:
	void *m_refCon;
	FskDragDropTargetProc m_DropTargetProc;
	HWND m_hWnd;
	FskWindow m_fskWin;
	IDropTargetHelper *m_pdth;
	Boolean m_useNativeProxy;
	POINT m_lastPt;

private:
	STDMETHOD(GetFileList)(LPDATAOBJECT pDataObject);
	STDMETHOD(DumpFileList)();

	DWORD m_dwRef;
	FskDragDropFile m_fileList;
};


static BOOL gInited = false;
static FskErr FskDragDropWinInit();
static void FskDragDropWinTerm();
static void ResolveLink(WCHAR *szLinkFullPath);
static DWORD dropEffectFromCursor(UInt32 cursorShape);

/* --------------------------------------------------------------------------------------------- */

FskErr FskDragDropWinInit()
{
	FskErr err = kFskErrNone;

	if (!gInited) {
		HRESULT hr;
		UINT dwErrMode;
		dwErrMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
		hr = OleInitialize(NULL);
		SetErrorMode(dwErrMode);
		if (S_OK != hr)
			err = kFskErrOperationFailed;
		gInited = true;
	}

	return err;
}

/* --------------------------------------------------------------------------------------------- */

void FskDragDropWinTerm()
{
	if (gInited) {
		OleUninitialize();
		gInited = false;
	}
}

/* --------------------------------------------------------------------------------------------- */

FskErr FskDragDropTargetWinRegisterNativeWindow(FskWindow win, FskDragDropTargetProc dropTargetProc, void *userData)
{
	CDropTarget *dropTarget;
	FskErr err;

	err = FskDragDropWinInit();
	BAIL_IF_ERR(err);

	if (++win->dragUseCount == 1) {
		HWND hWnd = (HWND)FskWindowGetNativeWindow(win);
		dropTarget = new CDropTarget();
		dropTarget->m_refCon = userData;
		dropTarget->m_DropTargetProc = dropTargetProc;
		dropTarget->m_hWnd = hWnd;
		dropTarget->m_fskWin = win;

		RegisterDragDrop(hWnd, dropTarget);
		win->dropTarget = dropTarget;
	}

bail:
	return err;
}

/* --------------------------------------------------------------------------------------------- */

void FskDragDropTargetWinUnregisterNativeWindow(FskWindow win)
{
	CDropTarget *dropTarget;
	HWND hWnd = (HWND)FskWindowGetNativeWindow(win);

	if ((win->useCount <= 0) || (--win->dragUseCount == 0)) {
		dropTarget = (CDropTarget*)win->dropTarget;
		if (NULL != dropTarget) {
			RevokeDragDrop(hWnd);
			delete dropTarget;
			win->dropTarget = NULL;
		}
	}
}

/* --------------------------------------------------------------------------------------------- */

void FskDragDropTargetWinUseNativeProxy(FskWindow win, Boolean useNativeProxy)
{
	CDropTarget *dropTarget;

	dropTarget = (CDropTarget*)win->dropTarget;
	if (0 != dropTarget) {
		dropTarget->m_useNativeProxy = useNativeProxy;
	}
}

/* --------------------------------------------------------------------------------------------- */

CDropTarget::~CDropTarget()
{
	DumpFileList();
	if (0 != m_pdth) m_pdth->Release(); 
}

/* --------------------------------------------------------------------------------------------- */

CDropTarget::CDropTarget()
{
	m_dwRef=0;
	m_fileList=0;
	m_pdth = 0;
	m_useNativeProxy = false;
	m_refCon = NULL;
	m_DropTargetProc = NULL;
	m_hWnd = NULL;
	m_fskWin = NULL;
	m_lastPt.x = m_lastPt.y = -1;
    CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
                     IID_IDropTargetHelper, (LPVOID*)&m_pdth);
}

/* --------------------------------------------------------------------------------------------- */

// Grab the list of files from the data source
STDMETHODIMP CDropTarget::GetFileList(LPDATAOBJECT pDataObject)
{
	FORMATETC fmtetc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmed;
	WCHAR szFileName[_MAX_PATH + 1];

	DumpFileList();

	if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
		if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK) {
			HDROP hdrop = (HDROP)GlobalLock(stgmed.hGlobal);
			if (NULL != hdrop) {
				UINT nFiles;
				
				nFiles = DragQueryFile(hdrop, (UINT)-1, NULL, 0);
				if (nFiles > 0) {
						
					// Tuck away the file list
					for (UINT nNames = 0; nNames < nFiles; nNames++) {
						FskDragDropFile droppedFile;

						ZeroMemory(szFileName, sizeof(szFileName));
						DragQueryFileW(hdrop, nNames, szFileName, _MAX_PATH + 1);

						// Resolve shortcuts
						WCHAR *ext = wcsrchr(szFileName, '.');
						if (ext && (0 == _wcsicmp(ext, L".lnk"))) {
							ResolveLink(szFileName);
						}

						// Add trailing delimeter to directories
						if (FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesW(szFileName)) {
							if ('\\' != szFileName[wcslen(szFileName)-1]) {
								wcscat(szFileName, L"\\");
							}
						}

						// Fixup path delimeters and encode to UTF-8
						FskMemPtrNewClear(sizeof(FskDragDropFileRecord), (FskMemPtr*)&droppedFile);
						droppedFile->fullPathName = fixUpPathForFsk(szFileName);
						FskListAppend((FskList *)&m_fileList, droppedFile);
					}
				}
				GlobalUnlock(hdrop);
			}
			ReleaseStgMedium(&stgmed);
		}
	}
	return S_OK;
}

/* --------------------------------------------------------------------------------------------- */

STDMETHODIMP CDropTarget::DumpFileList()
{
	while (NULL != m_fileList) {
		FskDragDropFile fileRecord = m_fileList;
		FskListRemove((FskList *)&m_fileList, fileRecord);
		FskMemPtrDispose(fileRecord->fullPathName);
		FskMemPtrDispose(fileRecord);
	}
	return S_OK;
}

/* --------------------------------------------------------------------------------------------- */
// Stupid OLE classes required to implement drag/drop on Windows
/* --------------------------------------------------------------------------------------------- */

STDMETHODIMP CDropTarget::DragEnter(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
	m_lastPt.x = m_lastPt.y = -1;

	GetFileList(pDataObject);

	POINT p = {pt.x, pt.y};
	if (m_useNativeProxy && m_pdth) {
        m_pdth->DragEnter(m_hWnd, pDataObject, &p, DROPEFFECT_COPY);
    }

	ScreenToClient(m_hWnd, &p);

	if (this->m_DropTargetProc) {
		(*this->m_DropTargetProc)(kFskDragDropTargetEnterWindow, p.x, p.y, m_fileList, this->m_refCon);
	}

	*pdwEffect = dropEffectFromCursor(m_fskWin->cursorShape);

	return S_OK;
}

/* --------------------------------------------------------------------------------------------- */

STDMETHODIMP CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
	POINT p = {pt.x, pt.y};
    if (m_useNativeProxy && m_pdth) {
        m_pdth->DragOver(&p, DROPEFFECT_COPY);
    }

	ScreenToClient(m_hWnd, &p);
	if (p.x != m_lastPt.x || p.y != m_lastPt.y) {
		if (this->m_DropTargetProc) {
			(*this->m_DropTargetProc)(kFskDragDropTargetOverWindow, p.x, p.y, m_fileList, this->m_refCon);
		}
		m_lastPt = p;
	}

	*pdwEffect = dropEffectFromCursor(m_fskWin->cursorShape);

	return S_OK;
}

/* --------------------------------------------------------------------------------------------- */

STDMETHODIMP CDropTarget::DragLeave()
{
    if (m_useNativeProxy && m_pdth) {
        m_pdth->DragLeave();
    }

	if (this->m_DropTargetProc) {
		(*this->m_DropTargetProc)(kFskDragDropTargetLeaveWindow, 0, 0, m_fileList, this->m_refCon);
	}

	DumpFileList();

	return S_OK;
}

/* --------------------------------------------------------------------------------------------- */

STDMETHODIMP CDropTarget::Drop(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
	POINT p = {pt.x, pt.y};
	if (m_useNativeProxy && m_pdth) {
		m_pdth->Drop(pDataObject, &p, DROPEFFECT_COPY);
	}

	ScreenToClient(m_hWnd, &p);

	if (this->m_DropTargetProc) {
		(*this->m_DropTargetProc)(kFskDragDropTargetDropInWindow, p.x, p.y, m_fileList, this->m_refCon);
	}

	*pdwEffect = dropEffectFromCursor(m_fskWin->cursorShape);

	return S_OK;
}

/* --------------------------------------------------------------------------------------------- */

// From the Shell Explorer's Cookbook
static void ResolveLink(WCHAR *szLinkFullPath)
{
   IShellLink* psl = 0;
   IPersistFile* ppf = 0; 

   // create a link manager object and request its interface
   HRESULT hr = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, (void**)&psl);
   if (S_OK != hr) goto bail;

   // associate the manager object with the link file in hand
   // Get a pointer to the IPersistFile interface. 
   hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
   if (S_OK != hr) goto bail;

   // "load" the name and resove the link
   hr = ppf->Load(szLinkFullPath, STGM_READ);
   if (S_OK != hr) goto bail;
   hr = psl->Resolve(NULL, SLR_UPDATE);
   if (S_OK != hr) goto bail;

   // Get the path to the link target.    
   WIN32_FIND_DATAW ffd; // we get those free of charge
   WCHAR buf[MAX_PATH]; // could have simply reused 'wsz'...
   hr = psl->GetPath((char *)buf, MAX_PATH, (WIN32_FIND_DATAA *)&ffd, 0);
   if (S_OK != hr) goto bail;
   wcscpy(szLinkFullPath, buf);

bail:
   // Release all interface pointers (both belong to the same object)
   if (NULL != ppf) ppf->Release();
   if (NULL != psl) psl->Release(); 
}

DWORD dropEffectFromCursor(UInt32 cursorShape)
{
	switch (cursorShape) {
		case kFskCursorAliasArrow:
				return DROPEFFECT_LINK;

		case kFskCursorCopyArrow:
				return DROPEFFECT_COPY;

		case kFskCursorNotAllowed:
				return DROPEFFECT_NONE;

		default:
				return DROPEFFECT_MOVE; 
	}
}

