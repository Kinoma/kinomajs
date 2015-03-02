/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//    Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/
#ifndef __UMC_MODULE_CONTEXT_H__
#define __UMC_MODULE_CONTEXT_H__

#include "umc_dynamic_cast.h"
#include "umc_defs.h"
#if !defined(__SYMBIAN32__) && (defined(_WIN32) || defined(_WIN32_WCE))
#include <windows.h>
#endif // defined(_WIN32) || defined(_WIN32_WCE)

namespace UMC
{

class ModuleContext
{
    DYNAMIC_CAST_DECL_BASE(ModuleContext)
public:

    virtual ~ModuleContext(){}
};

#if !defined(__SYMBIAN32__) && (defined(_WIN32) || defined(_WIN32_WCE))

    class HWNDModuleContext : public ModuleContext
    {
        DYNAMIC_CAST_DECL(HWNDModuleContext, ModuleContext)

    public:
        HWND m_hWnd;
        COLORREF m_ColorKey;
    };

#endif // defined(_WIN32) || defined(_WIN32_WCE)

class LocalReaderContext : public ModuleContext
{
    DYNAMIC_CAST_DECL(LocalReaderContext, ModuleContext)

public:
    LocalReaderContext()
    {
        memset(m_szFileName, 0, UMC::MAXIMUM_PATH);
    }

    vm_char  m_szFileName[UMC::MAXIMUM_PATH];
};

class RemoteReaderContext : public ModuleContext
{
    DYNAMIC_CAST_DECL(RemoteReaderContext, ModuleContext)

public:
    RemoteReaderContext()
    {
        memset(m_szServerName, 0, UMC::MAXIMUM_PATH);
        m_uiPortNumber = 0;
        m_transmissionMode = 0;
    }

    unsigned int m_uiPortNumber;        // listening port number
    unsigned int m_transmissionMode;    // choosed type of transmission;

    vm_char m_szServerName[UMC::MAXIMUM_PATH];     // IP addres string
};

}  //  namespace UMC

#endif // __UMC_MODULE_CONTEXT_H__

