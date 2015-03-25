
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __DataExtraction_h__
#define __DataExtraction_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IVideoDataExtraction_FWD_DEFINED__
#define __IVideoDataExtraction_FWD_DEFINED__
typedef interface IVideoDataExtraction IVideoDataExtraction;
#endif 	/* __IVideoDataExtraction_FWD_DEFINED__ */


#ifndef __RawVideoExtractor_FWD_DEFINED__
#define __RawVideoExtractor_FWD_DEFINED__

#ifdef __cplusplus
typedef class RawVideoExtractor RawVideoExtractor;
#else
typedef struct RawVideoExtractor RawVideoExtractor;
#endif /* __cplusplus */

#endif 	/* __RawVideoExtractor_FWD_DEFINED__ */


#ifndef __RawVideoExtractorProp_FWD_DEFINED__
#define __RawVideoExtractorProp_FWD_DEFINED__

#ifdef __cplusplus
typedef class RawVideoExtractorProp RawVideoExtractorProp;
#else
typedef struct RawVideoExtractorProp RawVideoExtractorProp;
#endif /* __cplusplus */

#endif 	/* __RawVideoExtractorProp_FWD_DEFINED__ */


#ifndef __IAudioDataExtraction_FWD_DEFINED__
#define __IAudioDataExtraction_FWD_DEFINED__
typedef interface IAudioDataExtraction IAudioDataExtraction;
#endif 	/* __IAudioDataExtraction_FWD_DEFINED__ */


#ifndef __RawAudioExtractor_FWD_DEFINED__
#define __RawAudioExtractor_FWD_DEFINED__

#ifdef __cplusplus
typedef class RawAudioExtractor RawAudioExtractor;
#else
typedef struct RawAudioExtractor RawAudioExtractor;
#endif /* __cplusplus */

#endif 	/* __RawAudioExtractor_FWD_DEFINED__ */


#ifndef __RawAudioExtractorProp_FWD_DEFINED__
#define __RawAudioExtractorProp_FWD_DEFINED__

#ifdef __cplusplus
typedef class RawAudioExtractorProp RawAudioExtractorProp;
#else
typedef struct RawAudioExtractorProp RawAudioExtractorProp;
#endif /* __cplusplus */

#endif 	/* __RawAudioExtractorProp_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "strmif.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_DataExtraction_0000 */
/* [local] */ 

#undef GetCurrentTime


extern RPC_IF_HANDLE __MIDL_itf_DataExtraction_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_DataExtraction_0000_v0_0_s_ifspec;

#ifndef __IVideoDataExtraction_INTERFACE_DEFINED__
#define __IVideoDataExtraction_INTERFACE_DEFINED__

/* interface IVideoDataExtraction */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IVideoDataExtraction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1E160934-334B-45b9-9AE2-CDD8E57AA84F")
    IVideoDataExtraction : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            BOOL bExtract) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetCurrentTime( 
            REFERENCE_TIME *pTime) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextTime( 
            REFERENCE_TIME *pTime) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsActive( 
            BOOL *pbActive) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsConnected( 
            BOOL *pbConnected) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetData( 
            LONG lSizeRequest,
            BYTE *pData,
            LONG *plSizeActual,
            REFERENCE_TIME *pTimeStart,
            REFERENCE_TIME *pTimeEnd) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPictureSize( 
            LONG *plWidth,
            LONG *plHeight) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetFrameRate( 
            double *pdRate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVideoDataExtractionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVideoDataExtraction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVideoDataExtraction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVideoDataExtraction * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IVideoDataExtraction * This,
            BOOL bExtract);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetCurrentTime )( 
            IVideoDataExtraction * This,
            REFERENCE_TIME *pTime);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextTime )( 
            IVideoDataExtraction * This,
            REFERENCE_TIME *pTime);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            IVideoDataExtraction * This,
            BOOL *pbActive);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IsConnected )( 
            IVideoDataExtraction * This,
            BOOL *pbConnected);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IVideoDataExtraction * This,
            LONG lSizeRequest,
            BYTE *pData,
            LONG *plSizeActual,
            REFERENCE_TIME *pTimeStart,
            REFERENCE_TIME *pTimeEnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetPictureSize )( 
            IVideoDataExtraction * This,
            LONG *plWidth,
            LONG *plHeight);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetFrameRate )( 
            IVideoDataExtraction * This,
            double *pdRate);
        
        END_INTERFACE
    } IVideoDataExtractionVtbl;

    interface IVideoDataExtraction
    {
        CONST_VTBL struct IVideoDataExtractionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVideoDataExtraction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVideoDataExtraction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVideoDataExtraction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVideoDataExtraction_Initialize(This,bExtract)	\
    (This)->lpVtbl -> Initialize(This,bExtract)

#define IVideoDataExtraction_GetCurrentTime(This,pTime)	\
    (This)->lpVtbl -> GetCurrentTime(This,pTime)

#define IVideoDataExtraction_GetNextTime(This,pTime)	\
    (This)->lpVtbl -> GetNextTime(This,pTime)

#define IVideoDataExtraction_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)

#define IVideoDataExtraction_IsConnected(This,pbConnected)	\
    (This)->lpVtbl -> IsConnected(This,pbConnected)

#define IVideoDataExtraction_GetData(This,lSizeRequest,pData,plSizeActual,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> GetData(This,lSizeRequest,pData,plSizeActual,pTimeStart,pTimeEnd)

#define IVideoDataExtraction_GetPictureSize(This,plWidth,plHeight)	\
    (This)->lpVtbl -> GetPictureSize(This,plWidth,plHeight)

#define IVideoDataExtraction_GetFrameRate(This,pdRate)	\
    (This)->lpVtbl -> GetFrameRate(This,pdRate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_Initialize_Proxy( 
    IVideoDataExtraction * This,
    BOOL bExtract);


void __RPC_STUB IVideoDataExtraction_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_GetCurrentTime_Proxy( 
    IVideoDataExtraction * This,
    REFERENCE_TIME *pTime);


void __RPC_STUB IVideoDataExtraction_GetCurrentTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_GetNextTime_Proxy( 
    IVideoDataExtraction * This,
    REFERENCE_TIME *pTime);


void __RPC_STUB IVideoDataExtraction_GetNextTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_IsActive_Proxy( 
    IVideoDataExtraction * This,
    BOOL *pbActive);


void __RPC_STUB IVideoDataExtraction_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_IsConnected_Proxy( 
    IVideoDataExtraction * This,
    BOOL *pbConnected);


void __RPC_STUB IVideoDataExtraction_IsConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_GetData_Proxy( 
    IVideoDataExtraction * This,
    LONG lSizeRequest,
    BYTE *pData,
    LONG *plSizeActual,
    REFERENCE_TIME *pTimeStart,
    REFERENCE_TIME *pTimeEnd);


void __RPC_STUB IVideoDataExtraction_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_GetPictureSize_Proxy( 
    IVideoDataExtraction * This,
    LONG *plWidth,
    LONG *plHeight);


void __RPC_STUB IVideoDataExtraction_GetPictureSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVideoDataExtraction_GetFrameRate_Proxy( 
    IVideoDataExtraction * This,
    double *pdRate);


void __RPC_STUB IVideoDataExtraction_GetFrameRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVideoDataExtraction_INTERFACE_DEFINED__ */


#ifndef __IAudioDataExtraction_INTERFACE_DEFINED__
#define __IAudioDataExtraction_INTERFACE_DEFINED__

/* interface IAudioDataExtraction */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAudioDataExtraction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E7B65A02-FBA8-4ba3-97B4-FE2980548EE8")
    IAudioDataExtraction : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            BOOL bExtract) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetCurrentTime( 
            REFERENCE_TIME *pTime) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextTime( 
            REFERENCE_TIME *pTime) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsActive( 
            BOOL *pbActive) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsConnected( 
            BOOL *pbConnected) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetData( 
            LONG lSizeRequest,
            BYTE *pData,
            LONG *plSizeActual,
            REFERENCE_TIME *pTimeStart,
            REFERENCE_TIME *pTimeEnd) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSampleSize( 
            LONG *plSize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNumChannels( 
            LONG *plChannels) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSamplingRate( 
            LONG *plRate) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RestrictFormat( 
            LONG lSampleSize,
            LONG lChannels,
            LONG lSamplingRate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAudioDataExtractionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAudioDataExtraction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAudioDataExtraction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAudioDataExtraction * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IAudioDataExtraction * This,
            BOOL bExtract);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetCurrentTime )( 
            IAudioDataExtraction * This,
            REFERENCE_TIME *pTime);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextTime )( 
            IAudioDataExtraction * This,
            REFERENCE_TIME *pTime);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            IAudioDataExtraction * This,
            BOOL *pbActive);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IsConnected )( 
            IAudioDataExtraction * This,
            BOOL *pbConnected);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IAudioDataExtraction * This,
            LONG lSizeRequest,
            BYTE *pData,
            LONG *plSizeActual,
            REFERENCE_TIME *pTimeStart,
            REFERENCE_TIME *pTimeEnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSampleSize )( 
            IAudioDataExtraction * This,
            LONG *plSize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNumChannels )( 
            IAudioDataExtraction * This,
            LONG *plChannels);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSamplingRate )( 
            IAudioDataExtraction * This,
            LONG *plRate);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RestrictFormat )( 
            IAudioDataExtraction * This,
            LONG lSampleSize,
            LONG lChannels,
            LONG lSamplingRate);
        
        END_INTERFACE
    } IAudioDataExtractionVtbl;

    interface IAudioDataExtraction
    {
        CONST_VTBL struct IAudioDataExtractionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAudioDataExtraction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAudioDataExtraction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAudioDataExtraction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAudioDataExtraction_Initialize(This,bExtract)	\
    (This)->lpVtbl -> Initialize(This,bExtract)

#define IAudioDataExtraction_GetCurrentTime(This,pTime)	\
    (This)->lpVtbl -> GetCurrentTime(This,pTime)

#define IAudioDataExtraction_GetNextTime(This,pTime)	\
    (This)->lpVtbl -> GetNextTime(This,pTime)

#define IAudioDataExtraction_IsActive(This,pbActive)	\
    (This)->lpVtbl -> IsActive(This,pbActive)

#define IAudioDataExtraction_IsConnected(This,pbConnected)	\
    (This)->lpVtbl -> IsConnected(This,pbConnected)

#define IAudioDataExtraction_GetData(This,lSizeRequest,pData,plSizeActual,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> GetData(This,lSizeRequest,pData,plSizeActual,pTimeStart,pTimeEnd)

#define IAudioDataExtraction_GetSampleSize(This,plSize)	\
    (This)->lpVtbl -> GetSampleSize(This,plSize)

#define IAudioDataExtraction_GetNumChannels(This,plChannels)	\
    (This)->lpVtbl -> GetNumChannels(This,plChannels)

#define IAudioDataExtraction_GetSamplingRate(This,plRate)	\
    (This)->lpVtbl -> GetSamplingRate(This,plRate)

#define IAudioDataExtraction_RestrictFormat(This,lSampleSize,lChannels,lSamplingRate)	\
    (This)->lpVtbl -> RestrictFormat(This,lSampleSize,lChannels,lSamplingRate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_Initialize_Proxy( 
    IAudioDataExtraction * This,
    BOOL bExtract);


void __RPC_STUB IAudioDataExtraction_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_GetCurrentTime_Proxy( 
    IAudioDataExtraction * This,
    REFERENCE_TIME *pTime);


void __RPC_STUB IAudioDataExtraction_GetCurrentTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_GetNextTime_Proxy( 
    IAudioDataExtraction * This,
    REFERENCE_TIME *pTime);


void __RPC_STUB IAudioDataExtraction_GetNextTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_IsActive_Proxy( 
    IAudioDataExtraction * This,
    BOOL *pbActive);


void __RPC_STUB IAudioDataExtraction_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_IsConnected_Proxy( 
    IAudioDataExtraction * This,
    BOOL *pbConnected);


void __RPC_STUB IAudioDataExtraction_IsConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_GetData_Proxy( 
    IAudioDataExtraction * This,
    LONG lSizeRequest,
    BYTE *pData,
    LONG *plSizeActual,
    REFERENCE_TIME *pTimeStart,
    REFERENCE_TIME *pTimeEnd);


void __RPC_STUB IAudioDataExtraction_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_GetSampleSize_Proxy( 
    IAudioDataExtraction * This,
    LONG *plSize);


void __RPC_STUB IAudioDataExtraction_GetSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_GetNumChannels_Proxy( 
    IAudioDataExtraction * This,
    LONG *plChannels);


void __RPC_STUB IAudioDataExtraction_GetNumChannels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_GetSamplingRate_Proxy( 
    IAudioDataExtraction * This,
    LONG *plRate);


void __RPC_STUB IAudioDataExtraction_GetSamplingRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAudioDataExtraction_RestrictFormat_Proxy( 
    IAudioDataExtraction * This,
    LONG lSampleSize,
    LONG lChannels,
    LONG lSamplingRate);


void __RPC_STUB IAudioDataExtraction_RestrictFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAudioDataExtraction_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


