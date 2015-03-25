
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Mon Aug 23 20:15:54 2004
 */
/* Compiler settings for DataExtraction.idl:
    Os, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)

#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IVideoDataExtraction,0x1E160934,0x334B,0x45b9,0x9A,0xE2,0xCD,0xD8,0xE5,0x7A,0xA8,0x4F);


MIDL_DEFINE_GUID(CLSID, CLSID_RawVideoExtractor,0x5B9B4245,0xABFB,0x4532,0xA7,0xBB,0x67,0xC5,0xA2,0x7E,0x16,0xA5);


MIDL_DEFINE_GUID(CLSID, CLSID_RawVideoExtractorProp,0x26EEA3C3,0x4FE8,0x4661,0xA5,0xC0,0xEE,0xDA,0x36,0x50,0x3F,0xE8);


MIDL_DEFINE_GUID(IID, IID_IAudioDataExtraction,0xE7B65A02,0xFBA8,0x4ba3,0x97,0xB4,0xFE,0x29,0x80,0x54,0x8E,0xE8);


MIDL_DEFINE_GUID(CLSID, CLSID_RawAudioExtractor,0x0B976DA6,0x5C09,0x421b,0xAE,0x5C,0x72,0xAD,0x04,0x11,0x1E,0xE2);


MIDL_DEFINE_GUID(CLSID, CLSID_RawAudioExtractorProp,0xDD4518DA,0x0E1B,0x4e1d,0x9E,0x8E,0x1E,0x17,0xB6,0xB1,0xCA,0x68);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

