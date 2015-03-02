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

#ifndef __UMC_H264_SLICE_DECODING_H
#define __UMC_H264_SLICE_DECODING_H

#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_dec.h"
#include "umc_h264_bitstream.h"
#include "umc_automatic_mutex.h"
#include "umc_event.h"

namespace UMC
{

// Slice decoding constant
enum
{
    NUMBER_OF_ROWS              = 4,
    NUMBER_OF_PIECES            = 8,
    NUMBER_OF_DEBLOCKERS        = 2
};

// Task ID enumerator
enum
{
    // whole slice is processed
    TASK_PROCESS                = 0,
    // piece of slice is decoded
    TASK_DEC,
    // piece of future frame's slice is decoded
    TASK_DEC_NEXT,
    // piece of slice is reconstructed
    TASK_REC,
    // whole slice is deblocked
    TASK_DEB_SLICE,
    // piece of slice is deblocked
    TASK_DEB,
    // piece of slice is deblocked by several threads
    TASK_DEB_THREADED,
    // whole frame is deblocked (when there is the slice groups)
    TASK_DEB_FRAME,
    // whole frame is deblocked (when there is the slice groups)
    TASK_DEB_FRAME_THREADED
    // whole frame is deblocked by several threads (when there is the slice groups)
};

#pragma pack(16)

// Declaration of thread-unsafe array class of simple types
template<class T>
class H264Array
{
public:
    // Default constructor
    H264Array(void)
    {
        m_Null = 0;
        m_pArray = NULL;
        m_nItemCount = 0;

    }

    // Destructor
    virtual
    ~H264Array(void)
    {
        Release();

    }

    // Initialize array
    bool Init(size_t nInitialCount)
    {
        if (false == Reallocate(nInitialCount))
            return false;

        return true;

    }

    // Index operator
    T &operator [] (int iIndex)
    {
        // need to increase array size
        if ((0 > iIndex) ||
            (((size_t) iIndex >= m_nItemCount) &&
             (false == Reallocate(iIndex))))
        {
            return m_Null;
        }

        return m_pArray[iIndex];

    }

    size_t GetItemCount(void)
    {
        return m_nItemCount;

    }

    // Reset object to initial state
    virtual
    void Reset(void)
    {
        if (m_pArray)
            memset(m_pArray, 0, sizeof(T) * m_nItemCount);
    }

protected:
    // Release object
    void Release(void)
    {
        if (m_pArray)
        {
            // we need to reset array before destruction
            Reset();

            delete [] m_pArray;
            m_pArray = NULL;
            m_nItemCount = 0;
        }
    }

    // Increase array size
    virtual
    bool Reallocate(size_t nCount)
    {
        T *pTemp;
        size_t nAllocated;

        if (m_nItemCount > nCount)
            return true;

        // allocate a little bit more
        nAllocated = max(nCount * 2, 4);
        pTemp = new T [nAllocated];
        if (NULL == pTemp)
            return false;
        // save array values
        if (m_pArray)
        {
            memcpy(pTemp, m_pArray, sizeof(T) * m_nItemCount);
            delete [] m_pArray;
        }
        // reset rest values
        memset(pTemp + m_nItemCount, 0, sizeof(T) * (nAllocated - m_nItemCount));

        // save values
        m_pArray = pTemp;
        m_nItemCount = nAllocated;

        return true;

    }

    T m_Null;                                                   // (T) NULL element
    T *m_pArray;                                                // (T *) pointer to array of items
    size_t m_nItemCount;                                        // (size_t) number of items in array

};

// Declaration of thread-unsafe array class of complex types types (like pointers)
template<class T>
class H264ItemArray : public H264Array<T *>
{
public:
    // Default constructor
    H264ItemArray(void)
    {

    }

    // Destructor
    virtual
    ~H264ItemArray(void)
    {
        // It was a beauty class. But some gcc compilers can't
        // parse inherited templates. We have to make this template look ugly.
        H264Array<T *>::Release();

    }

    // Allocate one more item
    T * &AllocateOneMore(void)
    {
        bool bOk = true;
        size_t i;

        while (bOk)
        {
            // find free space
            for (i = 0; i < H264Array<T *>::m_nItemCount; i += 1)
            {
                if (NULL == H264Array<T *>::m_pArray[i])
                {
                    H264Array<T *>::m_pArray[i] = new T();
                    if (NULL == H264Array<T *>::m_pArray[i])
                    {
                        return H264Array<T *>::m_Null;
                    }

                    return H264Array<T *>::m_pArray[i];
                }
            }

            Reallocate(i);
        }

        // illegal case. It must never hapend
        return H264Array<T *>::m_Null;

    }

    size_t GetItemCount(void)
    {
        return H264Array<T *>::m_nItemCount;

    }

    // Reset object to initial state
    virtual
    void Reset(void)
    {
        if (H264Array<T *>::m_pArray)
        {
            size_t i;

            for (i = 0; i < H264Array<T *>::m_nItemCount; i += 1)
            {
                if (H264Array<T *>::m_pArray[i])
                    delete H264Array<T *>::m_pArray[i];
                H264Array<T *>::m_pArray[i] = NULL;
            }
        }

        H264Array<T *>::Reset();
    }

protected:
    // Increase array size
    virtual
    bool Reallocate(size_t nCount)
    {
        T **pTemp;
        size_t nAllocated;

        if (H264Array<T *>::m_nItemCount > nCount)
            return true;

        // allocate a little bit more
        nAllocated = max(nCount * 2, 4);
        pTemp = new T * [nAllocated];
        if (NULL == pTemp)
            return false;
        // save array values
        if (H264Array<T *>::m_pArray)
        {
            memcpy(pTemp, H264Array<T *>::m_pArray, sizeof(T *) * H264Array<T *>::m_nItemCount);
            delete [] H264Array<T *>::m_pArray;
        }
        // reset rest values
        memset(pTemp + H264Array<T *>::m_nItemCount,
               0,
               sizeof(T *) * (nAllocated - H264Array<T *>::m_nItemCount));

        // save values
        H264Array<T *>::m_pArray = pTemp;
        H264Array<T *>::m_nItemCount = nAllocated;

        return true;
    }
};

// forward declaration of internal types
struct H264RefListInfo;

// declaration of types
typedef Ipp32s FactorArray[MAX_NUM_REF_FRAMES];
typedef Ipp32s FactorArrayAFF[2][2][2][MAX_NUM_REF_FRAMES];

class H264Task;
class H264MemoryPiece;

class H264ThreadedDeblockingTools
{
public:
    // Default constructor
    H264ThreadedDeblockingTools(void);
    // Destructor
    ~H264ThreadedDeblockingTools(void);

    // Initialize tools
    bool Init(Ipp32s iConsumerNumber);
    // Reset tools when threaded deblocking is started
    void Reset(Ipp32s iFirstMB, Ipp32s iMaxMB, Ipp32s iDebUnit, Ipp32s iMBWidth);

    // Get next task
    bool GetMBToProcess(H264Task *pTask);
    // Set deblocked macroblocks
    void SetProcessedMB(H264Task *pTask);
    // Ask current segment deblocking finish
    bool IsDeblockingDone(void);

protected:
    // Release object
    void Release(void);

    // Get next task for currect thread
    bool GetMB(Ipp32s iThreadNumber, Ipp32s &iFirstMB, Ipp32s &iMBToProcess);
    // Set deblocked macroblocks for current thread
    void SetMB(Ipp32s iThreadNumber, Ipp32s iFirstMB, Ipp32s iMBToProcess);

    Ipp32s m_iConsumerNumber;                                   // (Ipp32s) number of consumers
    H264Array<Ipp32s> m_iCurMBToDeb;                            // (H264Array<Ipp32s>) array of current MB number to de-blocking
    Ipp32s m_iMaxMB;                                            // (Ipp32s) maximum MB number in slice
    Ipp32s m_iDebUnit;                                          // (Ipp32s) minimal unit of deblocking process
    Ipp32s m_iMBWidth;                                          // (Ipp32s) width of MB row

    H264Array<bool> m_bThreadWorking;                           // (H264Array<bool>) array of "thread does threaded deblocking" flag for threaded version
};

#pragma pack(8)

class H264Slice
{
    // It is OK. H264SliceStore is owner of H264Slice object.
    // He can do what he wants.
    friend class H264SliceStore_;
    friend class H264SliceStore_MP4;

public:
    H264Slice(void);
    virtual ~H264Slice(void);

    bool Init(Ipp32s iConsumerNumber);// Initialize slice
    bool SetSourceData(Ipp8u const *pSource, Ipp32s const bit_offset, Ipp32s const nSourceSize, Ipp32s const iNumber);// Set slice source data
    void SetDestinationFrame(H264VideoDecoder::H264DecoderFrame *pCurrentFrame, H264DecoderLocalMacroblockDescriptor &mbinfo);// Set destination frame
    void SetSliceNumber(Ipp32s iSliceNumber);// Set current slice number
    //
    // method(s) to obtain slice specific information
    //
    H264SliceHeader *GetSliceHeader(void){return &m_SliceHeader;} // Obtain pointer to slice header
    H264Bitstream *GetBitStream(void){return &m_BitStream;}// Obtain bit stream object
    PredWeightTable *GetPredWeigthTable(Ipp32s iNum){return m_PredWeight[iNum & 1];}// Obtain prediction weigth table
    Ipp32s GetFirstMBNumber(void){return m_iFirstMB;} // Obtain first MB number
    Ipp32s GetMBWidth(void){return m_iMBWidth;} // Obtain MB width
    Ipp32s GetMBRowWidth(void){return (m_iMBWidth /** (m_SliceHeader.MbaffFrameFlag + 1)*/);} // Obtain MB row width
    Ipp32s GetMBHeight(void){return m_iMBHeight;} // Obtain MB height
    Ipp32s GetPicParamSet(void){return m_CurrentPicParamSet;}// Obtain current picture parameter set number
    Ipp32s GetSeqParamSet(void){return m_CurrentSeqParamSet;}// Obtain current sequence parameter set number
    H264PicParamSet *GetPicParam(void){return m_pPicParamSet + m_CurrentPicParamSet;}// Obtain current picture parameter set
    
    H264PicParamSet *GetPicParam2(void){return m_pPicParamSet;}/* DEBUG : redundant method */
   
    H264SeqParamSet *GetSeqParam(void){return m_pSeqParamSet + m_CurrentSeqParamSet;} // Obtain current sequence parameter set
    /* DEBUG : redundant method */
    H264SeqParamSet *GetSeqParam2(void){return m_pSeqParamSet;}
    H264VideoDecoder::H264DecoderFrame *GetCurrentFrame(void){return m_pCurrentFrame;}// Obtain current destination frame
    Ipp32s GetSliceNum(void){return m_iNumber;}// Obtain slice number
    //***bnie: Ipp32s GetFieldIndex_kinoma_always_zero(void){return 0/*m_field_index*/;}// Obtain owning slice field index
    bool NeedToCheckSliceEdges(void){return m_bNeedToCheckMBSliceEdges;}// Need to check slice edges
    bool GetDeblockingCondition(void);// Do we can doing deblocking
    
    FactorArray &GetDistScaleFactor(void){return m_DistScaleFactor;}// Obtain scale factor arrays
    FactorArray &GetDistScaleFactorMV(void){return m_DistScaleFactorMV;}
    FactorArrayAFF &GetDistScaleFactorAFF(void){return m_DistScaleFactorAFF;}
    FactorArrayAFF &GetDistScaleFactorMVAFF(void){return m_DistScaleFactorMVAFF;}
    Ipp32s GetMaxMB(void){return m_iMaxMB;} // Obtain maximum of macroblock
    H264DecoderLocalMacroblockDescriptor &GetMBInfo(void){return m_mbinfo;}// Obtain local MB information
    Ipp32u *GetMBIntraTypes(void){return m_pMBIntraTypes;} // Obtain pointer to MB intra types
    bool IsSliceGroups(void){return (1 < m_pPicParamSet[m_CurrentPicParamSet].num_slice_groups);};// Check slice organization
    bool IsSizeUnknown(void){return m_bUnknownSize;}; // Is slice's size unknown
    bool DeblockThroughBoundaries(void){return (DEBLOCK_FILTER_ON_NO_SLICE_EDGES != m_SliceHeader.disable_deblocking_filter_idc);}; // Do we require to do deblocking through slice boundaries
    bool IsReferenceReady(void){return m_bReferenceReady;};// Is reference list ready
    void UpdateReferenceList(H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList);// Update reference list

    //
    // Segment decoding mode's variables
    //

    // Obtain decoding state variables
    void GetStateVariables(Ipp32s &iMBSkipCount, Ipp32s &iQuantPrev, Ipp32s &iPassFDDecode);
    void GetStateVariables(Ipp32s &iMBSkipFlag, Ipp32s &iQuantPrev, bool &bSkipNextFDF, Ipp32s &iPrevDQuant);
    // Save decoding state variables
    void SetStateVariables(Ipp32s iMBSkipCount, Ipp32s iQuantPrev, Ipp32s iPassFDDecode);
    void SetStateVariables(Ipp32s iMBSkipFlag, Ipp32s iQuantPrev, bool bSkipNextFDF, Ipp32s iPrevDQuant);

protected:
    // Release object
    void Release(void);

    // Decode slice header
    bool DecodeSliceHeader(bool bFullInitialization = true);
    // Reference list(s) management functions & tools
    Status UpdateRefPicList(H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList);
    void InitPSliceRefPicList(Ipp32s /*NumL0RefActive*/, H264VideoDecoder::H264DecoderFrame **pRefPicList, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList);
    void InitBSliceRefPicLists(Ipp32s /*NumL0RefActive*/, Ipp32s /*NumL1RefActive*/, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList, H264RefListInfo &rli);
    //***kinoma optimization for packet loss robustness  --bnie 6/11/2008
    void InitPSliceRefPicList_default(Ipp32s /*NumL0RefActive*/, H264VideoDecoder::H264DecoderFrame **pRefPicList, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList);
    void InitBSliceRefPicLists_default(Ipp32s /*NumL0RefActive*/, Ipp32s /*NumL1RefActive*/, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList, H264RefListInfo &rli);
    void InitDistScaleFactor(Ipp32s NumL0RefActive, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void AdjustRefPicListForFields(H264VideoDecoder::H264DecoderFrame **pRefPicList, Ipp8s *pFields, H264RefListInfo &rli);
    void ReOrderRefPicList(H264VideoDecoder::H264DecoderFrame **pRefPicList, RefPicListReorderInfo *pReorderInfo, Ipp32s MaxPicNum, Ipp32s NumRefActive);
   
	H264Slice *m_pNext;                                         // (H264Slice *) pointer to next slice in list
	RefPicListReorderInfo m_ReorderInfoL0;                        // (RefPicListReorderInfo) reference list 0 info
    RefPicListReorderInfo m_ReorderInfoL1;                        // (RefPicListReorderInfo) reference list 1 info

	AdaptiveMarkingInfo *m_AdaptiveMarkingInfo_shadow;	//***bnie: kinoma added	4/1/09

    // Reference list management functions
    Status UpdateRefPicList(H264SliceHeader SliceHeader, RefPicListReorderInfo *pReorderInfo_L0, RefPicListReorderInfo *pReorderInfo_L1, Ipp32u uSliceNum);

    H264SliceHeader m_SliceHeader;                              // (H264SliceHeader) slice header
    H264Bitstream m_BitStream;                                  // (H264Bitstream) slice bit stream

    PredWeightTable m_PredWeight[2][MAX_NUM_REF_FRAMES];        // (PredWeightTable []) prediction weight table

    FactorArray m_DistScaleFactor;
    FactorArray m_DistScaleFactorMV;
    FactorArrayAFF m_DistScaleFactorAFF;                        // [curmb field],[ref1field],[ref0field]
    FactorArrayAFF m_DistScaleFactorMVAFF;                      // [curmb field],[ref1field],[ref0field]

    H264PicParamSet *m_pPicParamSet;                            // (H264PicParamSet *) pointer to array of picture parameters sets
    H264SeqParamSet *m_pSeqParamSet;                            // (H264SeqParamSet *) pointer to array of sequence parameters sets

    H264VideoDecoder::H264DecoderFrame *m_pCurrentFrame;        // (H264VideoDecoder::H264DecoderFrame *) pointer to destination frame
    H264DecoderLocalMacroblockDescriptor m_mbinfo;              // (H264DecoderLocalMacroblockDescriptor) current frame MB information
    Ipp32u *m_pMBIntraTypes;                                    // (Ipp32u *) array of macroblock intra types

    Ipp32s m_iMBWidth;                                          // (Ipp32s) width in macroblock units
    Ipp32s m_iMBHeight;                                         // (Ipp32s) height in macroblock units
    Ipp32s m_CurrentPicParamSet;                                // (Ipp32s) current picture parameter set
    Ipp32s m_CurrentSeqParamSet;                                // (Ipp32s) current sequence parameter set

    Ipp32s m_iNumber;                                           // (Ipp32s) current slice number
    Ipp32s m_iFirstMB;                                          // (Ipp32s) first MB number in slice
    Ipp32s m_iMaxMB;                                            // (Ipp32s) last unavailable  MB number in slice
    bool m_bUnknownSize;                                        // (bool) size of current slice is unknown
    Ipp32s m_iAvailableMB;                                      // (Ipp32s) available number of macroblocks (used in "unknown mode")

    Ipp32s m_iCurMBToDec;                                       // (Ipp32s) current MB number to decode
    Ipp32s m_iCurMBToRec;                                       // (Ipp32s) current MB number to reconstruct
    Ipp32s m_iCurMBToDeb;                                       // (Ipp32s *) current MB number to de-blocking

    bool m_bNeedToUpdateRefPicList;                             // (bool) reference list requires to update
    bool m_bReferenceReady;                                     // (bool) reference list is updated

    bool m_bInProcess;                                          // (bool) slice is under whole decoding
    bool m_bDecVacant;                                          // (bool) decoding is vacant
    bool m_bRecVacant;                                          // (bool) reconstruct is vacant
    bool m_bDebVacant;                                          // (bool) de-blocking is vacant
    bool m_bFirstDebThreadedCall;                               // (bool) "first threaded deblocking call" flag
    bool m_bPermanentTurnOffDeblocking;                         // (bool) "disable deblocking" flag

    H264ThreadedDeblockingTools m_DebTools;                     // (H264ThreadedDeblockingTools) threaded deblocking tools
/*
    H264ItemArray<Event> m_eLeft;                               // (H264ItemArray<Event>) array of event to threaded deblocking
    H264ItemArray<Event> m_eRight;                              // (H264ItemArray<Event>) array of event to threaded deblocking
*/
    Ipp16s *m_pAllocatedBuffer;                                 // (Ipp16s *) allocated buffer
    Ipp32s m_iAllocatedMB;                                      // (Ipp32s) size of allocated buffer in macroblock

    Ipp16s *(m_pDecBuffer[NUMBER_OF_ROWS]);                     // (Ipp16s *([])) array of decoding buffers
    Ipp16s *(m_pRecBuffer[NUMBER_OF_ROWS]);                     // (Ipp16s *([])) array of reconstruction buffers
    H264MemoryPiece *m_pAddBuffer;                              // (Ipp16s *([])) list of additional decoding buffers

    // through-decoding variable(s)
    Ipp32s m_nMBSkipCount;                                      // (Ipp32u) current count of skipped macro blocks
    Ipp32s m_nPassFDFDecode;                                    // (Ipp32u)
    Ipp32s m_nQuantPrev;                                        // (Ipp32u) quantize value of previous macro block
    Ipp32s m_prev_dquant;
    //***bnie: Ipp32s m_field_index_kinoma_always_zero;                                       // (Ipp32s) current field index
    bool m_bSkipNextFDF;
    bool m_bNeedToCheckMBSliceEdges;                            // (bool) need to check inter-slice boundaries

    bool m_bDecoded;                                            // (bool) "slice has been decoded" flag
    bool m_bPrevDeblocked;                                      // (bool) "previous slice has been deblocked" flag
    bool m_bDeblocked;                                          // (bool) "slice has been deblocked" flag

    H264MemoryPiece *m_pSource;                                 // (H264MemoryPiece *) pointer to owning memory piece
    //***bnie: double m_dTime;                                             // (double) slice's time stamp
};

#pragma pack()

inline
void H264Slice::GetStateVariables(Ipp32s &iMBSkipCount, Ipp32s &iQuantPrev, Ipp32s &iPassFDDecode)
{
    iMBSkipCount = m_nMBSkipCount;
    iQuantPrev = m_nQuantPrev;
    iPassFDDecode = m_nPassFDFDecode;

} // void H264Slice::GetStateVariables(Ipp32s &iMBSkipCount, Ipp32s &iQuantPrev, Ipp32s &iPassFDDecode)

inline
void H264Slice::GetStateVariables(Ipp32s &iMBSkipFlag, Ipp32s &iQuantPrev, bool &bSkipNextFDF, Ipp32s &iPrevDQuant)
{
    iMBSkipFlag = m_nMBSkipCount;
    iQuantPrev = m_nQuantPrev;
    bSkipNextFDF = m_bSkipNextFDF;
    iPrevDQuant = m_prev_dquant;

} // void H264Slice::GetStateVariables(Ipp32s &iMBSkipFlag, Ipp32s &iQuantPrev, bool &bSkipNextFDF, Ipp32s &iPrevDQuant)

inline
void H264Slice::SetStateVariables(Ipp32s iMBSkipCount, Ipp32s iQuantPrev, Ipp32s iPassFDDecode)
{
    m_nMBSkipCount = iMBSkipCount;
    m_nQuantPrev = iQuantPrev;
    m_nPassFDFDecode = iPassFDDecode;

} // void H264Slice::SetStateVariables(Ipp32s iMBSkipCount, Ipp32s iQuantPrev, Ipp32s iPassFDDecode)

inline
void H264Slice::SetStateVariables(Ipp32s iMBSkipFlag, Ipp32s iQuantPrev, bool bSkipNextFDF, Ipp32s iPrevDQuant)
{
    m_nMBSkipCount = iMBSkipFlag;
    m_nQuantPrev = iQuantPrev;
    m_bSkipNextFDF = bSkipNextFDF;
    m_prev_dquant = iPrevDQuant;

} // void H264Slice::SetStateVariables(Ipp32s iMBSkipFlag, Ipp32s iQuantPrev, bool bSkipNextFDF, Ipp32s iPrevDQuant)
/*
inline
Ipp32s GetFrameNum(H264Slice *pSlice)
{
    H264SliceHeader *pHeader;
    // check error(s)
    VM_ASSERT(pSlice);

    pHeader = pSlice->GetSliceHeader();

    return (0 == pHeader->field_pic_flag) ?
           (pHeader->frame_num) :
           ((pHeader->frame_num << 1) + pHeader->bottom_field_flag);

} // Ipp32s GetFrameNum(H264Slice *pSlice)
*/

inline
bool IsPictureTheSame(H264Slice *pSliceOne, H264Slice *pSliceTwo)
{
    H264SliceHeader *pOne = pSliceOne->GetSliceHeader();
    H264SliceHeader *pTwo = pSliceTwo->GetSliceHeader();

    // this function checks two slices are from same picture or not
    // 7.4.1.2.4 part of h264 standart

    if ((pOne->frame_num != pTwo->frame_num) ||
        (pOne->first_mb_in_slice == pTwo->first_mb_in_slice) ||
        (pOne->pic_parameter_set_id != pTwo->pic_parameter_set_id) ||
        //***bnie: (pOne->field_pic_flag != pTwo->field_pic_flag) ||
		//***bniebnie
        (pOne->bottom_field_flag_kinoma_always_zero != pTwo->bottom_field_flag_kinoma_always_zero))
        return false;

    if ((pOne->nal_ref_idc != pTwo->nal_ref_idc) &&
        (0 == min(pOne->nal_ref_idc, pTwo->nal_ref_idc)))
        return false;

    if (0 == pSliceOne->GetSeqParam()->pic_order_cnt_type)
    {
        if ((pOne->pic_order_cnt_lsb != pTwo->pic_order_cnt_lsb) ||
            (pOne->delta_pic_order_cnt_bottom != pTwo->delta_pic_order_cnt_bottom))
            return false;
    }
    else
    {
        if ((pOne->delta_pic_order_cnt[0] != pTwo->delta_pic_order_cnt[0]) ||
            (pOne->delta_pic_order_cnt[1] != pTwo->delta_pic_order_cnt[1]))
            return false;
    }

    if (pOne->nal_unit_type != pTwo->nal_unit_type)
    {
        if ((NAL_UT_IDR_SLICE == pOne->nal_unit_type) ||
            (NAL_UT_IDR_SLICE == pTwo->nal_unit_type))
            return false;
    }
    else if (NAL_UT_IDR_SLICE == pOne->nal_unit_type)
    {
        if (pOne->idr_pic_id != pTwo->idr_pic_id)
            return false;
    }

    return true;

} // bool IsPictureTheSame(H264SliceHeader *pOne, H264SliceHeader *pTwo)

// Declaration of internal class(es)
class H264SegmentDecoder;
class H264SegmentDecoderMultiThreaded;

#pragma pack(16)

class H264Task
{
public:
    // Default constructor
    H264Task(Ipp32s iThreadNumber) :
      m_iThreadNumber(iThreadNumber)
    {
        m_pSlice = NULL;

        pFunctionSingle = NULL;
#ifndef DROP_MULTI_THREAD
		pFunctionMulti = NULL;
#endif
		m_pBuffer = NULL;
        m_pBufferEnd = NULL;

        m_iFirstMB = -1;
        m_iMaxMB = -1;
        m_iMBToProcess = -1;
        m_iTaskID = 0;
        m_bDone = false;
    }

    H264Slice *m_pSlice;                                        // (H264Slice *) pointer to owning slice

    Status (H264SegmentDecoder::*pFunctionSingle)(Ipp32s nCurMBNumber, Ipp32s &nMaxMBNumber); // (Status (*) (Ipp32s, Ipp32s &)) pointer to working function
#ifndef DROP_MULTI_THREAD
	Status (H264SegmentDecoderMultiThreaded::*pFunctionMulti)(Ipp32s nCurMBNumber, Ipp32s &nMaxMBNumber); // (Status (*) (Ipp32s, Ipp32s &)) pointer to working function
#endif 
	Ipp16s *m_pBuffer;                                          // (Ipp16s *) pointer to working buffer
    Ipp16s *m_pBufferEnd;                                       // (Ipp16s *) pointer to end of working buffer

    Ipp32s m_iThreadNumber;                                     // (Ipp32s) owning thread number
    Ipp32s m_iFirstMB;                                          // (Ipp32s) first MB in slice
    Ipp32s m_iMaxMB;                                            // (Ipp32s) maximum MB number in owning slice
    Ipp32s m_iMBToProcess;                                      // (Ipp32s) number of MB to processing
    Ipp32s m_iTaskID;                                           // (Ipp32s) task identificator
    bool m_bDone;                                               // (bool) task was done
};

#pragma pack()










// Slice data struct used by  decoder for maintaining slice-level
// data for a picture.
struct H264SliceData
{/*
    Ipp32s DistScaleFactor[MAX_NUM_REF_FRAMES];
    Ipp32s DistScaleFactorMV[MAX_NUM_REF_FRAMES];
    Ipp32s DistScaleFactorAFF[2][2][2][MAX_NUM_REF_FRAMES];     // [curmb field],[ref1field],[ref0field]
    Ipp32s DistScaleFactorMVAFF[2][2][2][MAX_NUM_REF_FRAMES];   // [curmb field],[ref1field],[ref0field]
*/
};

class H264SliceDecoderStream
{
/*
public:
    H264Bitstream *m_pBitStream;                                // (H264Bitstream *) pointer to bit stream
    Ipp32u m_nMBSkipCount;                                      // (Ipp32u) current count of skipped macro blocks
    Ipp32u m_nPassFDFDecode;                                    // (Ipp32u)
    bool m_bSkipNextFDF;
    Ipp32u m_nQuantPrev;                                        // (Ipp32u) quantize value of previous macro block
    Ipp32s m_prev_dquant;
    bool m_bDone;                                               // (bool) "slice has been decoded" flag
    bool m_bNeedToCheckMBSliceEdges;                            // (bool) need to check inter-slice boundaries
*/
};

class H264SliceDecoderReset
{
/*
public:
    H264PicParamSet *m_pPicParamSet;                            // (H264PicParamSet *) pointer to array of picture parameters sets
    H264SeqParamSet *m_pSeqParamSet;                            // (H264SeqParamSet *) pointer to array of sequence parameters sets
    H264VideoDecoder::H264DecoderFrame *m_pCurrentFrame;        // (H264VideoDecoder::H264DecoderFrame *) pointer to frame being decoded
    Ipp32u *m_pMBIntraTypes;                                    // (Ipp32u *) pointer to array of intra types
    H264VideoDecoder::H264DecoderFrameList *m_pDecodedFramesList; // (H264VideoDecoder::H264DecodedFrameList *) pointer to decoded frame list

    H264DecoderLocalMacroblockDescriptor *m_pMBInfo;
    Ipp8u m_field_index;
    Ipp8u m_NALRefIDC;
*/
};

class H264SegmentStore;

// class to reset segment decoders
class H264SegmentDecoderReset : public H264SliceDecoderReset
{
/*
public:
    PredWeightTable *m_pPredWeight_L0;                          // (PredWeightTable *) pointer to forward prediction table
    PredWeightTable *m_pPredWeight_L1;                          // (PredWeightTable *) pointer to backward prediction table
    H264SliceDecoderStream *m_pStream;                          // (H264SliceDecoderStream *) pointer to being decoded stream
    H264SliceInfo *m_pSliceInfo;                                // (H264SliceInfo *) current slice information
    H264SliceData *m_pSliceData;                                // (SliceData *) current slice scale factors

    vm_event *m_pReadyDecoding;                                 // (vm_event *) pointer to array of "start decoding" events
    vm_event *m_pReadyReconstruction;                           // (vm_event *) pointer to array of "start reconstruction" events
    vm_semaphore *m_pReadyDeblocking;                           // (vm_semaphore *) pointer to array of "start deblocking" semaphores
    vm_event *m_pDoneLeft;                                      // (vm_event *) pointer to array of "left macroblock done" events
    vm_event *m_pDoneRight;                                     // (vm_event *) pointer to array of "right macroblock done" events

    H264SegmentStore *m_pSegmentStore;
*/
};

enum
{
    COEFFICIENTS_BUFFER_SIZE    = 16 * 51
};

} // namespace UMC

#endif // __UMC_H264_SLICE_DECODING_H
