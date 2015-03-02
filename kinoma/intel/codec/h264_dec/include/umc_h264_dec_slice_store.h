/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_H264_DEC_SLICE_STORE_H
#define __UMC_H264_DEC_SLICE_STORE_H

#include "ippdefs.h"
#include "vm_types.h"
#include "umc_structures.h"
#include "umc_h264_dec.h" /* DEBUG : require to move H264DecodedFrame to separate file */
#include "umc_media_data_ex.h"
#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_slice_decoding.h"

namespace UMC
{

// forward declaration of internal types
class H264Slice;
class H264Task;
class AutomaticMutex;

enum H264Status
{
    // we require one more free frame
    H264_NEED_FREE_FRAME,
    // we require one more free frame for complimentary field
    H264_NEED_FREE_FIELD,
    // source data is only headers
    H264_DECODE_HEADERS,
    // non-VCL unit requires to process
    H264_NON_VCL_NAL_UNIT,
    // data is not enough
    H264_NOT_ENOUGH_DATA,
    // all is OK (or YES answer)
    H264_OK,
    // something is wrong (or NO answer)
    H264_FAILED,
    // error, show must stoped
    H264_ERROR
};

#pragma pack(16)

class H264MemoryPiece
{
    friend class H264SliceStore_;
    friend class H264SliceStore_MP4;

public:
    // Default constructor
    H264MemoryPiece(void)
    {
        m_pSourceBuffer = NULL;
        m_nSourceSize = 0;
    }

    // Destructor
    ~H264MemoryPiece(void)
    {
        Release();
    }

    // Allocate memory piece
    bool Allocate(size_t nSize)
    {
        if ((m_pSourceBuffer) &&
            (m_nSourceSize >= nSize))
            return true;

        // release before allocation
        Release();

        // allocate little more
        m_pSourceBuffer = ippsMalloc_8u_x((Ipp32s) nSize * 2);
        if (NULL == m_pSourceBuffer)
            return false;
        m_nSourceSize = nSize;

        return true;
    }

    // Get next element
    H264MemoryPiece *GetNext(void){return m_pNext;}
    // Obtain data pointer
    Ipp8u *GetPointer(void){return m_pSourceBuffer;}

protected:
    Ipp8u *m_pSourceBuffer;                                     // (Ipp8u *) pointer to source memory
    size_t m_nSourceSize;                                       // (size_t) allocated memory size
    H264MemoryPiece *m_pNext;                                   // (H264MemoryPiece *) pointer to next memory piece

    // Release object
    void Release(void)
    {
        if (m_pSourceBuffer)
            ippsFree_x(m_pSourceBuffer);
        m_pSourceBuffer = NULL;
        m_nSourceSize = 0;
    }
};

struct H264NonVCLUnit
{
    Ipp32s m_iStartCode;                                        // (Ipp32s) start code of NAL unit
    H264MemoryPiece *m_pSource;                                 // (H264MemoryPiece *) source of NAL unit
    size_t m_nSize;                                             // (size_t) size of NAL unit
    H264NonVCLUnit *m_pNext;                                    // (H264NonVCLUnit *) pointer to next unit in the queue
};

class H264SliceStore_
{
public:
    // Default constructor
    H264SliceStore_(H264PicParamSet *pPicParamSet, H264SeqParamSet *pSeqParamSet, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList);
    // Destructor
    virtual ~H264SliceStore_(void);

    // Initialize store
    bool Init(Ipp32s iConsumerNumber);

    // add data to source store
    //***H264Status AddSource(MediaData * (&pSource) ); //***bnie , bool bDataSwapped);
    // Add next source frame to store
    H264Status AddSourceData(MediaData3_V51 * (&pSource) );//***bnie:, bool bOwnMemory = false);
    // Get source to decode
    H264Status GetSource(MediaData_V51 * (&pSource), H264VideoDecoder::H264DecoderFrame * (&pCurrentFrame), bool bEndOfStream);

    // Add next encoded data & exchange current frame
    H264Status ExchangeFrame(MediaData_V51 * (&pSource),
                             bool bDataSwapped,
                             H264VideoDecoder::H264DecoderFrame * (&pCurrentFrame));
    // Get used size of last decoded frame
    //size_t GetUsedSize();
    void RecycleSlice();
	// Add one more decoded frame to slice store (frame level parallelization)
    H264Status AddFreeFrame(H264VideoDecoder::H264DecoderFrame *pFrame);
    // Set current destination frame
    void SetDestinationFrame(H264VideoDecoder::H264DecoderFrame *pCurrentFrame, H264DecoderLocalMacroblockDescriptor &mbinfo);
    // Switch decoding frame
    void SwitchFrame(void);

    // Do we need additional ?
    bool NeedAdditionalFrame(void);
    /* DEBUG : temporary method */
    void SetMBMap(H264DecoderMBAddr *pMBMap)
    {
        Ipp32s i;

        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
            m_pMainQueue[i]->m_mbinfo.active_next_mb_table = pMBMap;
    }

    //
    // Consumer method(s)
    //

    // Get next slice to process
    bool GetNextSlice(H264Task *pTask);

#ifndef DROP_MULTI_THREAD_nonono  
	// Get next working task
    bool GetNextTask(H264Task *pTask);
    // Task was performed
    void AddPerformedTask(H264Task *pTask);
#endif

    // Get specific slice header
    H264SliceHeader *GetSliceHeader(Ipp32s iSliceNumber);
	
	AdaptiveMarkingInfo *m_AdaptiveMarkingInfo_shadow;	//***bnie: kinoma added	4/1/09

protected:
    // Release object
    void Release(void);
#ifndef DROP_MULTI_THREAD   
	// Analyze statistics & make decision about frame-parallelization
    bool IsItTimeToFrameParallelization(void);
#endif
    //
    //virtual
    //H264Status IsHeaderDetected(MediaData *pSource);//***bnie , bool bDataSwapped);
    // Wrap header into new media data & process it
    //virtual
    //H264Status WrapHeader(MediaData *(&pSource));;//***bnie , bool bDataSwapped);
    // Swap header into our memory
    //virtual
    //H264Status SwapHeader(size_t &nDstSize, Ipp8u *pbSrc, size_t nSrcSize);
    // Swap encoded data into our memory
    //***bnie virtual
     //***bnie H264Status SwapData(MediaData * (&pSource));
    // Copy encoded data into our memory
#ifndef DROP_MULTI_THREAD 
	virtual
    H264Status CopyData(MediaData * (&pSource));
#endif
	// Separate frames in incoming buffer
    //***virtual
    //***H264Status SeparateData(MediaData * (&pSource));
    // Add non-VCL unit to store
    //***H264Status AddNonVCLUnit(MediaData *pSource, bool bOwnMemory = false);
    // Prepare next source for decoding
    H264Status PrepareNextSource(MediaData_V51 * (&pSource), H264VideoDecoder::H264DecoderFrame * (&pCurrentFrame), bool bEndOfStream);
    // Allocate one more slice
    bool AllocateSlice(void);
    // Allocate one more non-VCL unit
    //***bool AllocateNonVCLUnit(void);
    // Get first slice number for complementary field
    Ipp32s GetFirstSliceNumber(void);
    // Add one more slice to decoding work (when slice sizes are unknown)
    void AddNextSlice(void);

    // Get free piece of memory
    //*** H264MemoryPiece *GetFreeMemoryPiece(H264MemoryPiece * (&pList), size_t nSize);
    // Add filled piece of memory
    void AddFilledMemoryPiece(H264MemoryPiece * (&pList), H264MemoryPiece *pMemoryPiece);
    // Allocate array of macroblock intra types
    bool AllocateMBIntraTypes(Ipp32s iIndex, Ipp32s iMBNumber);

    // Get next available slice to decoding
    bool GetNextSliceToDecoding(H264Task *pTask);
    // Get next available slice to deblocking
    bool GetNextSliceToDeblocking(H264Task *pTask);
    // Get next decoding task
    bool GetDecodingTask(H264Task *pTask, Ipp32s iMode);
    // Get next reconstruct task
    bool GetReconstructTask(H264Task *pTask);
    // Get next deblocking task
    bool GetDeblockingTask(H264Task *pTask);
    // Get next frame threaded deblocking task
    bool GetFrameDeblockingTaskThreaded(H264Task *pTask);
    // Get next slice threaded deblocking task
    bool GetSliceDeblockingTaskThreaded(H264Task *pTask);
    // Get next additional decoding task
    bool GetAdditionalDecodingTask(H264Task *pTask);

    // Try to set frame to "decoded" state
    void SetFrameUncompressed(void);
    // Check current frame condition
    bool IsFrameUncompressed(void);
    // Try to set frame to "decoded" state
    void SetFrameDeblocked(void);
    // Check current frame condition
    bool IsFrameDeblocked(void);

    //
    // Methods to do flexible thread's work balancing
    //

    // Count up quantity of free decoding buffers
    Ipp32s CountDecodingBuffers(void);
    // Count up quantity of filled reconstruct buffers
    Ipp32s CountReconstructBuffers(void);
    // Test quantity of macroblocks to deblock
    bool AreManyToDeblock(void);
    // Get number of slices to decode
    Ipp32s GetNumberOfSlicesToDecode(void);
    // Get number of slices to reconstruct
    Ipp32s GetNumberOfSlicesToReconstruct(void);

    //
    // Class members
    //

    bool m_bUnknownMode;                                        // (bool) slice sizes are unknown



    // Slice objects
    H264ItemArray<H264Slice> m_pMainQueue;                      // (H264ItemArray<H264Slice>) array of slices to decode
    Ipp32s m_iSlicesInMainQueue;                                // (Ipp32s) number of slices in main queue (current frame)
    H264ItemArray<H264Slice> m_pAdditionalQueue;                // (H264ItemArray<H264Slice>) array of additional slices to decode
    Ipp32s m_iSlicesInAdditionalQueue;                          // (Ipp32s) number of slices in additional queue (future frames)
    H264Slice *m_pFreeSlices;                                   // (H264Slice *) list of unused slices

//***bnie:
public:
	H264Slice * GetQueuedSlice( Ipp32s idx ) { return m_pAdditionalQueue[idx]; }
protected:

#ifndef DROP_NON_VCL
    H264NonVCLUnit *m_pFreeNonVCLUnits;                         // (H264NonVCLUnit *) list of free non-VCL unit structs
    H264NonVCLUnit *m_pNonVCLUnits;                             // (H264NonVCLUnit *) list of free non-VCL unit structs
#endif

    Ipp32s m_iProcessedFrames;                                  // (Ipp32s) number of decoded frames
    Ipp32s m_iAddedFrames;                                      // (Ipp32s) number of frames passed through store
    Ipp32s m_iSlicesInLastPic;                                  // (Ipp32s) number of slices in last decoded picture

    /* DEBUG: need to update to support many frames or frame-level parallelization */
    Ipp8u *m_pbSource;
    size_t m_nSize;
    /* end of DEBUG */

    bool m_bWaitForIDR;                                         // (bool) waiting for reference slice data

    H264PicParamSet * const m_pPicParamSet;                     // (H264SeqParamSet * const) pointer to array of picture headers
    H264SeqParamSet * const m_pSeqParamSet;                     // (H264SeqParamSet * const) pointer to array of sequence headers

	H264VideoDecoder::H264DecoderFrameList * const m_pDecoderFrameList;               // (H264VideoDecoder::H264DecoderFrameList *) pointer to decoded frame list, used to update references

    //***MediaDataEx_2 m_MediaDataEx666;                                  // (MediaDataEx) media data to use into DecodeFrame method
    //***MediaDataEx_2::_MediaDataEx_2 m_MediaDataEx_666;                   // (MediaDataEx::_MediaDataEx) start codes info to use into DecodeFrame method
    Ipp8u *m_pHeaderBuffer;                                     // (Ipp8u *) buffer for stream header(s)
    size_t m_nHeaderBufferSize;                                 // (size_t) size of allocated buffer for stream header(s)
    H264MemoryPiece *m_pFilledMemory;                           // (H264MemoryPiece *) queue of filled memory pieces
    H264MemoryPiece *m_pFreeMemory;                             // (H264MemoryPiece *) list of free memory pieces

    H264MemoryPiece *m_pFreeBuffers;                            // (H264MemoryPieces *) list of free buffers to decode
    bool m_bDoFrameParallelization;                             // (bool) frame parallelization is working
    H264DecoderLocalMacroblockDescriptor m_mbinfo[2];           // (H264DecoderLocalMacroblockDescriptor []) array of decoding info
    Ipp32u *(m_pMBIntraTypes[2]);                               // (Ipp32u *([])) array of pointers to macroblocks intra types
    Ipp32s m_iMBIntraSizes[2];                                  // (Ipp32s) size of allocated arrays of macroblock intra types

    //
    // Threading tool(s)
    //

    Ipp32s m_iConsumerNumber;                                   // (Ipp32s) number of consumers

#ifndef DROP_MULTI_THREAD
    vm_mutex m_mGuard;                                          // (vm_mutex) guard
    H264ItemArray<Event> m_eWaiting;                            // (H264ItemArray<Event>) waiting threads events
    Ipp32u m_nWaitingThreads;                                   // (Ipp32u) mask of waiting threads
#endif
    //
    // De-blocking threading tool(s)
    //

    bool m_bDoFrameDeblocking;                                  // (bool) we may do frame deblocking
    bool m_bFirstDebThreadedCall;                               // (bool) "first threaded deblocking call" flag
    H264ThreadedDeblockingTools m_DebTools;                     // (H264ThreadedDeblockingTools) threadede deblocking tools

private:
    // We lock assignment operator to avoid any
    // accasionaly assignments
    H264SliceStore_ & operator = (H264SliceStore_ &)
    {
        return *this;

    } // H264SliceStore_ & operator = (H264SliceStore_ &)

};

//***bnie
#if 0	//not used case
class H264SliceStore_MP4 : public H264SliceStore_
{
public:
    // Constructor
    H264SliceStore_MP4(H264PicParamSet *pPicParamSet, H264SeqParamSet *pSeqParamSet, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList);
    // Destuctor
    virtual
    ~H264SliceStore_MP4();

protected:
    // Check input memory for owning stream headers
    virtual
    H264Status IsHeaderDetected(MediaData *pSource, bool bDataSwapped);
    // Wrap header into new media data & process it
    //virtual
    //H264Status WrapHeader(MediaData * &pSource, bool bDataSwapped);
    // Swap header into our memory
    virtual
    H264Status SwapHeader(size_t &nDstSize, Ipp8u *pbSrc, size_t nSrcSize);
    // Swap encoded data into our memory
    virtual
    H264Status SwapData(MediaData * &pSource);

    virtual
    H264Status CopyData(MediaData * (&pSource));
    virtual
    H264Status SeparateData(MediaData * (&pSource));

    typedef struct AVCRecord
    {
        AVCRecord()
        {
            configurationVersion = 1;
            lengthSizeMinusOne = 1;
            numOfSequenceParameterSets = 0;
            numOfPictureParameterSets = 0;
        }

        Ipp8u configurationVersion;
        Ipp8u AVCProfileIndication;
        Ipp8u profile_compatibility;
        Ipp8u AVCLevelIndication;
        Ipp8u lengthSizeMinusOne;
        Ipp8u numOfSequenceParameterSets;
        Ipp8u numOfPictureParameterSets;

    } AVCRecord;

    enum
    {
        D_START_CODE_LENGHT = 4,
        D_BYTES_FOR_HEADER_LENGHT = 2
    };

    Ipp32s BuildNALUnit(size_t lenght, Ipp8u * buf, Ipp8u * write_buf);

    H264Status ReadSample(MediaData *&pSource, bool bHeader = false);
    H264Status ReadAVCDecoderConfigurationRecord(MediaData *&pSource);

    Ipp32s GetLenght(Ipp32s len_bytes_count, Ipp8u * buf);

    bool m_bReadRecord;
    bool m_bDataSwapped;

    AVCRecord avcRecord;
};
#endif

#pragma pack()

} // namespace UMC

#endif // __UMC_H264_DEC_SLICE_STORE_H
