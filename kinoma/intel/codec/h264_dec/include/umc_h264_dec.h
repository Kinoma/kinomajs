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
#ifndef __UMC_H264_DEC_H__
#define __UMC_H264_DEC_H__

#include <stdlib.h>
#include "umc_video_decoder.h"
#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_dec_defs_yuv.h"
#include "umc_h264_dec_tables.h"
#include "umc_h264_bitstream.h"
#include "umc_base_color_space_converter.h"
#include "vm_time.h"

//***
#include "avcC.h"

//#define VM_DEBUG 7

//#define USE_DETAILED_TIMING

#ifdef USE_DETAILED_TIMING
#include "ippcore.h"
static __declspec(naked) vm_tick GetTick()
{
    __asm rdtsc
    __asm ret
}
#define GET_TICKS GetTick();//ippGetCpuClocks()
#endif
#ifdef USE_DETAILED_TIMING
    class TimingInfo
    {
        public:
        TimingInfo()
        {Reset();}

        void Reset()
        {
            m_total_vld_time    = 0;
            m_total_rec_time    = 0;
            m_total_dbl_time    = 0;
            m_avg_vld_time      = 0;
            m_avg_rec_time      = 0;
            m_avg_dbl_time      = 0;
            m_total_mbtype_dec_time = 0;
            m_avg_mbtype_dec_time   = 0;
            m_total_chrmode_dec_time= 0;
            m_avg_chrmode_dec_time  = 0;
            m_total_mv_dec_time     = 0;
            m_avg_mv_dec_time       = 0;
            m_total_cbp_dec_time    = 0;
            m_avg_cbp_dec_time      = 0;
            m_total_qpdelta_dec_time= 0;
            m_avg_qpdelta_dec_time  = 0;
            m_total_coeffs_dec_time = 0;
            m_avg_coeffs_dec_time   = 0;
            m_frame_count           = 0;
            m_total_intrarec_time    = 0;
            m_avg_intrarec_time      = 0;
            m_total_interrec_time    = 0;
            m_avg_interrec_time      = 0;
            m_total_mc_time          = 0;
            m_avg_mc_time           = 0;

            vm_tick start_tick, end_tick;
            start_tick = GET_TICKS;
            end_tick = GET_TICKS;
            m_compensative_const = end_tick - start_tick;

        }

        vm_tick m_total_vld_time;       //total in ms
        vm_tick m_avg_vld_time;         //average per frame in ms

        vm_tick m_total_rec_time;       //total in ms
        vm_tick m_avg_rec_time;         //average per frame in ms

        vm_tick m_total_intrarec_time;  //total in ms
        vm_tick m_avg_intrarec_time;    //average per frame in ms

        vm_tick m_total_interrec_time;  //total in ms
        vm_tick m_avg_interrec_time;    //average per frame in ms

        vm_tick m_total_mc_time;        //total in ms
        vm_tick m_avg_mc_time;          //average per frame in ms

        vm_tick m_total_dbl_time;       //total in ms
        vm_tick m_avg_dbl_time;         //average per frame in ms

        vm_tick m_total_mbtype_dec_time;   //total in ms
        vm_tick m_avg_mbtype_dec_time;     //average per frame in ms

        vm_tick m_total_chrmode_dec_time;   //total in ms
        vm_tick m_avg_chrmode_dec_time;     //average per frame in ms

        vm_tick m_total_mv_dec_time;       //total in ms
        vm_tick m_avg_mv_dec_time;         //average per frame in ms

        vm_tick m_total_cbp_dec_time;       //total in ms
        vm_tick m_avg_cbp_dec_time;         //average per frame in ms

        vm_tick m_total_qpdelta_dec_time;   //total in ms
        vm_tick m_avg_qpdelta_dec_time;     //average per frame in ms

        vm_tick m_total_coeffs_dec_time;    //total in ms
        vm_tick m_avg_coeffs_dec_time;      //average per frame in ms

        float     m_frame_count;
        vm_tick m_compensative_const;
    };
#else
    class TimingInfo
    {
   public:
        TimingInfo()
        {m_frame_count = 0;}
       int     m_frame_count;
    };
#endif

namespace UMC
{

// forward declaration of internal types
class H264SliceDecoder;
class H264SliceStore_;

#pragma pack(16)

class H264VideoDecoder : public VideoDecoder_V51
{
    DYNAMIC_CAST_DECL(H264VideoDecoder, VideoDecoder_V51)

public:

// The H264DecoderFrame class represents a YUV work space which was obtained
// by decoding a bitstream picture.

        class H264DecoderFrame : public H264DecYUVWorkSpace
        {
                H264DecoderFrame       *m_pPreviousFrame;
                H264DecoderFrame       *m_pFutureFrame;
                    // These point to the previous and future reference frames
                    // used to decode this frame.
                    // These are also used to maintain a doubly-linked list of
                    // reference frames in the H264DecoderFrameList class.  So, these
                    // may be non-NULL even if the current frame is an I frame,
                    // for example.  m_pPreviousFrame will always be valid when
                    // decoding a P or B frame, and m_pFutureFrame will always
                    // be valid when decoding a B frame.

                Ipp64f                 m_cumulativeTR;
                    // This is the TR of the picture, relative to the very first
                    // frame decoded after a call to Start_Sequence.  The Ipp64f type
                    // is used to avoid wrap around problems.

                bool                m_isDisplayable;
                    // When true indicates this decoded frame contains a decoded
                    // frame ready for output. The frame should not be overwritten
                    // until wasOutputted is also true.

                bool                m_wasOutputted;
                    // m_wasOutputted indicates whether this decoded frame has
                    // already been sent for output to the calling application.
                    // The frame might not be immediately displayed depending
                    // on whether the application buffers up decoded frames.
                bool                m_wasDisplayed;
                    // m_wasDisplayed indicates whether this decoded frame has
                    // already been displayed.

                bool                m_lockedForDisplay;
                    // m_lockedForDisplay indicates whether this decoded frame
                    // is locked by the application for display purposes.
                    // If it is, the decoder should not overwrite it until
                    // unlocked.

                bool m_bBusy;                                   // (bool) frame is busy (locked for decoding)
            public:
                void SetBusy(bool bBusy){m_bBusy = bBusy;};

                //***double           m_dFrameTime;
                Ipp8u           m_PictureStructureForRef;
                //***Ipp8u           m_PictureStructureForDec;

                Ipp32s           totalMBs;

                // For type 1 calculation of m_PicOrderCnt. m_FrameNum is needed to
                // be used as previous frame num.

                Ipp32s                    m_PicNum[2];
                Ipp32s                    m_LongTermPicNum[2];
                Ipp32s                    m_FrameNum;
                Ipp32s                    m_FrameNumWrap;
                Ipp32s                    m_LongTermFrameIdx;
                Ipp32s                    m_RefPicListResetCount[2];
                Ipp32s                    m_PicOrderCnt[2];    // Display order picture count mod MAX_PIC_ORDER_CNT.
                //***bnie: Ipp32s                    m_SliceType[2];    // Display order picture count mod MAX_PIC_ORDER_CNT.
                Ipp32s                    m_crop_left;
                Ipp32s                    m_crop_right;
                Ipp32s                    m_crop_top;
                Ipp32s                    m_crop_bottom;
                Ipp8s                     m_crop_flag;
                bool                      m_isShortTermRef[2];
                bool                      m_isLongTermRef[2];
                H264DecoderGlobalMacroblocksDescriptor m_mbinfo; //Global MB Data
                //Ipp32u              m_FrameNum;            // Decode order frame label, from slice header
                Ipp8u               m_PQUANT;            // Picture QP (from first slice header)
                Ipp8u               m_PQUANT_S;            // Picture QS (from first slice header)
                sDimensions         m_dimensions;
                //***bnie: Ipp8u               m_bottom_field_flag_01[2];
                    // The above variables are used for management of reference frames
                    // on reference picture lists maintained in m_RefPicList. They are
                    // updated as reference picture management information is decoded
                    // from the bitstream. The picture and frame number variables determine
                    // reference picture ordering on the lists.

                            H264DecoderFrame();
        virtual            ~H264DecoderFrame();


        virtual Status  allocate(const sDimensions &lumaSize,Ipp8u bpp,Ipp8u chroma_format);
                    // This allocate method first clears our state, and then
                    // calls H264DecYUVWorkSpace::allocate.
                    // An existing buffer, if any, is not reallocated if it
                    // is already large enough.


                // The following methods provide access to the H264Decoder's doubly
                // linked list of H264DecoderFrames.  Note that m_pPreviousFrame can
                // be non-NULL even for an I frame.

                H264DecoderFrame       *previous() { return m_pPreviousFrame; }
                H264DecoderFrame       *future()   { return m_pFutureFrame; }

                void                setPrevious(H264DecoderFrame *pPrev)
                {
                    m_pPreviousFrame = pPrev;
                }
                void                setFuture(H264DecoderFrame *pFut)
                {
                    m_pFutureFrame = pFut;
                }

                Ipp64f              cumulativeTR() { return m_cumulativeTR; }
                void                setCumulativeTR(Ipp64f tr) { m_cumulativeTR = tr; }

                bool                wasDisplayed()    { return m_wasDisplayed; }
                void                setWasDisplayed() { m_wasDisplayed = true; }

                //EnumPicCodType      m_PicCodType;
                //sDimensions            m_dimensions;
                bool        isDisplayable()    { return m_isDisplayable; }
                void        SetisDisplayable() { m_isDisplayable = true; SetBusy(false);}
                void        unSetisDisplayable() { m_isDisplayable = false; }

                bool        wasOutputted()    { return m_wasOutputted; }
                void        setWasOutputted() { m_wasOutputted = true; }
                void        unsetWasOutputted() { m_wasOutputted = false; }

                void        LockForDisplay()       { m_lockedForDisplay = true; }
                void        unLockForDisplay()       { m_lockedForDisplay = false; }

                bool        isDisposable()    { return (!m_isShortTermRef[0] &&
                                                        !m_isShortTermRef[1] &&
                                                        !m_isLongTermRef[0] &&
                                                        !m_isLongTermRef[1] &&
                                                        !m_lockedForDisplay &&
                                                        (m_wasOutputted || !m_isDisplayable) &&
                                                        !m_bBusy); }
                            // A decoded frame can be "disposed" if it is not an active reference
                            // and it is not locked by the calling application and it has been
                            // output for display.

                Ipp32s      PicNum(Ipp8u f,Ipp8u force=0) {
                    if ((m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3)
                    {
                        return MIN(m_PicNum[0],m_PicNum[1]);
                    }
                    else if (force==2)
                    {
                        if (isShortTermRef_kinoma_always_frame() /***bnie: && isShortTermRef(1)*/) return MIN(m_PicNum[0],m_PicNum[1]);
                        else if (isShortTermRef_kinoma_always_frame()) return m_PicNum[0];
                        else return m_PicNum[0];
                    }

                    return m_PicNum[f];
                }
                void        setPicNum(Ipp32s PicNum,Ipp8u f)
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                    {
                        m_PicNum[0] = m_PicNum[1]=PicNum;
                    }
                    //***bnie: else
                    //***bnie:     m_PicNum[f] = PicNum;
                }

                    // Updates m_LongTermPicNum for if long term reference, based upon
                    // m_LongTermFrameIdx.

                Ipp32s      FrameNum()
                {
                        return m_FrameNum;
                }
                void        setFrameNum(Ipp32s FrameNum)
                {
                    m_FrameNum = FrameNum;
                }

                Ipp32s      FrameNumWrap()
                {
                    return m_FrameNumWrap;
                }
                void        setFrameNumWrap(Ipp32s FrameNumWrap)
                {
                    m_FrameNumWrap = FrameNumWrap;
                };
                void        UpdateFrameNumWrap(Ipp32s CurrFrameNum, Ipp32s MaxFrameNum,Ipp8u CurrPicStruct);
                    // Updates m_FrameNumWrap and m_PicNum if the frame is a short-term
                    // reference and a frame number wrap has occurred.

                Ipp32s      LongTermFrameIdx()
                {
                    return m_LongTermFrameIdx;
                }
                void        setLongTermFrameIdx(Ipp32s LongTermFrameIdx)
                {
                        m_LongTermFrameIdx = LongTermFrameIdx;
                };

                bool        isShortTermRef_kinoma_always_frame()
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE )
                        return m_isShortTermRef[0] && m_isShortTermRef[1];
                   //***bnie:  else
                   //***bnie:      return m_isShortTermRef[WhichField];
                };
                Ipp8u    isShortTermRef()
                {
                        return m_isShortTermRef[0] + m_isShortTermRef[1]*2;
                };
                void        SetisShortTermRef_kinoma_always_frame()
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                        m_isShortTermRef[0] = m_isShortTermRef[1] = true;
                    //***bnie: else
                    //***bnie:     m_isShortTermRef[WhichField] = true;
                }

                Ipp32s      PicOrderCnt_kinoma_always_frame(/***bnie: int index,Ipp8u force=0*/)
                {
                    //***bnie: if ((/*bnie: m_PictureStructureForRef>=FRM_STRUCTURE && */force==0) || force==3)
                    //***bnie: {
                        return MIN(m_PicOrderCnt[0],m_PicOrderCnt[1]);
                    //***bnie: }
#if 0
                    else if (force==2)
                    {
                        if (isShortTermRef_kinoma_always_frame() /*&& isShortTermRef(1)*/) return MIN(m_PicOrderCnt[0],m_PicOrderCnt[1]);
                        else if (isShortTermRef_kinoma_always_frame()) return m_PicOrderCnt[0];
                        else return m_PicOrderCnt[1];
                    }

                    return m_PicOrderCnt[index];
#endif                
				}

                Ipp32s      DeblockPicID(int index)
                {
#if 0
                    //the constants are subject to change
                    return PicOrderCnt(index,force)*2+FrameNumWrap()*534+FrameNum()*878+PicNum(index,force)*14
                        +RefPicListResetCount(index,force);
#else
                    size_t ret = (size_t) this;
                    return (Ipp32s)(ret + index);

#endif
                }

                void        setPicOrderCnt(Ipp32s PicOrderCnt, int index) {m_PicOrderCnt[index] = PicOrderCnt;};
                bool        isLongTermRef_kinoma_always_frame()//***bnie: Ipp8s WhichField)
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                        return m_isLongTermRef[0] && m_isLongTermRef[1];
                    //***bnie: else
                    //***bnie:     return m_isLongTermRef[WhichField];
                };
                Ipp8u    isLongTermRef()
                {
                    return m_isLongTermRef[0] + m_isLongTermRef[1]*2;
                };
                void        SetisLongTermRef_kinoma_always_frame()
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                        m_isLongTermRef[0] = m_isLongTermRef[1] = true;
                    //***bnie: else
                    //***bnie:     m_isLongTermRef[WhichField] = true;
                }

                void        unSetisShortTermRef_kinoma_always_frame()
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                    //***bnie: {
                        m_isShortTermRef[0] = m_isShortTermRef[1] = false;
                    //***bnie: }
                    //***bnie: else
                    //***bnie:     m_isShortTermRef[WhichField] = false;
                }

                void        unSetisLongTermRef_kinoma_always_frame()
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                    //***bnie: {
                        m_isLongTermRef[0] = m_isLongTermRef[1] = false;
                    //***bnie: }
                    //***bnie: else
                    //***bnie:     m_isLongTermRef[WhichField] = false;
                }
                Ipp32s      LongTermPicNum(Ipp8u f,Ipp8u force=0)
                {
                    if ((m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3)
                    {
                        return MIN(m_LongTermPicNum[0],m_LongTermPicNum[1]);
                    }
                    else if (force==2)
                    {
                        if (isLongTermRef_kinoma_always_frame()/***bnie: && isLongTermRef(1)*/) return MIN(m_LongTermPicNum[0],m_LongTermPicNum[1]);
                        else if (isShortTermRef_kinoma_always_frame()) return m_LongTermPicNum[0];
                        else return m_LongTermPicNum[0];
                    }
                    return m_LongTermPicNum[f];
                }
                void        setLongTermPicNum(Ipp32s LongTermPicNum,Ipp8u f) {m_LongTermPicNum[f] = LongTermPicNum;}
                void        UpdateLongTermPicNum(Ipp8u CurrPicStruct);

                void        IncreaseRefPicListResetCount(Ipp8u f)
                {
                    /*if (m_PictureStructureForRef>=FRM_STRUCTURE)
                    {
                        m_RefPicListResetCount[0]++;
                        m_RefPicListResetCount[1]++;
                    }
                    else*/
                        m_RefPicListResetCount[f]++;
                }
                void        InitRefPicListResetCount_kinoma_always_frame(/***bnie: Ipp8u f*/)
                {
                    //***bnie: if (m_PictureStructureForRef>=FRM_STRUCTURE)
                        m_RefPicListResetCount[0]=m_RefPicListResetCount[1]=0;
                    //***bnie: else
                    //***bnie:     m_RefPicListResetCount[f]=0;
                }
                Ipp32s      RefPicListResetCount_kinoma_always_frame(/***bnie: Ipp8u f,Ipp8u force=0*/)
                {
                   //***bnie: if ((/***bnie: m_PictureStructureForRef>=FRM_STRUCTURE && */force==0)|| force==3)
                        return MAX(m_RefPicListResetCount[0],m_RefPicListResetCount[1]);
                   //***bnie: else
                   //***bnie:     return m_RefPicListResetCount[f];
                }
                Ipp8s           GetNumberByParity(Ipp8s parity)
                {
                    if (parity==-1) return -1;
					return parity;
					//***bnie
                    //***bnieif (m_bottom_field_flag[0]==parity) return 0;
                    //***bnieif (m_bottom_field_flag[1]==parity) return 1;
                    VM_ASSERT(m_PictureStructureForRef>=FRM_STRUCTURE);
                    return 0;
                }

                bool                        m_bIsIDRPic;
                   // Read from slice NAL unit of current picture. True indicates the
                   // picture contains only I or SI slice types.
/*
                Ipp16u                      m_num_slice_start;
                //start slice index of 2nd field
*/

                    // Buffer (within m_pParsedFrameData) used to store information for
                    // each MB in the bitstream.

                // Struct containing list 0 and list 1 reference picture lists for one slice.
                // Length is plus 1 to provide for null termination.
                struct H264H264DecoderRefPicListStruct
                {
                    H264DecoderFrame    *m_RefPicList[MAX_NUM_REF_FRAMES+1];
                    Ipp8s           m_Prediction[MAX_NUM_REF_FRAMES+1];
                };


                struct H264DecoderRefPicList
                {
                    H264H264DecoderRefPicListStruct m_RefPicListL0;
                    H264H264DecoderRefPicListStruct m_RefPicListL1;
                };    // H264DecoderRefPicList

                H264DecoderRefPicList    *m_pRefPicList;
                    // Buffer (within m_pParsedFrameData) used to store reference
                    // picture lists generated while decoding the frame. Contains
                    // a pair of lists (list 0, list 1) for each slice.

                //////////////////////////////////////////////////////////////////////////////
                // GetRefPicList
                // Returns pointer to start of specified ref pic list.
                //////////////////////////////////////////////////////////////////////////////
                H264H264DecoderRefPicListStruct* GetRefPicList(Ipp32u SliceNum, Ipp32u List)
                {
                    H264DecoderFrame::H264H264DecoderRefPicListStruct *pList;

                    if (List)
                        pList = &m_pRefPicList[SliceNum].m_RefPicListL1;
                    else
                        pList = &m_pRefPicList[SliceNum].m_RefPicListL0;

                    return pList;

                }    // RefPicList


				//***kinoma enhancement   --bnie 8/2/2008
				H264DecoderFrame** GetRefPicListSafe( Ipp32u SliceNum, Ipp32u List )	
				{																	
					H264DecoderFrame::H264H264DecoderRefPicListStruct *pList;		
					
					//***bniebniebnie
					if( SliceNum == -1u )
						return NULL;

					pList = GetRefPicList(SliceNum, List); 
					return pList == NULL ? NULL : pList->m_RefPicList;
				}

				Ipp8s * GetPedictionSafe( Ipp32u SliceNum, Ipp32u List )	
				{													
					H264DecoderFrame::H264H264DecoderRefPicListStruct *pList;
																	
					pList = GetRefPicList(SliceNum, List); 
					return pList == NULL ? NULL : pList->m_Prediction;
				}

                    // Returns pointer to start of specified ref pic list.
                Ipp8u                 *m_pParsedFrameData;
                Ipp8u                 *m_pParsedFrameDataNew;
            // This points to a huge, monolithic buffer that contains data
            // derived from parsing the current frame.  It contains motion
            // vectors,  MB info, reference indices, and slice info for the
            // current frame, among other things. When B slices are used it
            // contains L0 and L1 motion vectors and reference indices.

        sDimensions              m_paddedParsedFrameDataSize;
            // m_pParsedFrameData's allocated size is remembered so that a
            // re-allocation is done only if size requirements exceed the
            // existing allocation.
            // m_paddedParsedFrameDataSize contains the image dimensions,
            // rounded up to a multiple of 16, that were used.


        Status                  allocateParsedFrameData(const sDimensions&,Ipp8u);
            // Reallocate m_pParsedFrameData, if necessary, and initialize the
            // various pointers that point into it.

        void                        deallocateParsedFrameData();

        };

// The H264DecoderFrameList class implements a doubly-linked list of
// H264DecoderFrame objects.  It uses the m_pPreviousFrame and m_pFutureFrame
// members of the H264DecoderFrame class to implement the links.

        class H264DecoderFrameList
        {
        public:
            // Default constructor
            H264DecoderFrameList(void);
            // Destructor
            virtual
            ~H264DecoderFrameList(void);

                H264DecoderFrame   *head() { return m_pHead; }
                H264DecoderFrame   *tail() { return m_pTail; }

                bool            isEmpty() { return !m_pHead; }

                H264DecoderFrame   *detachHead();    // Detach the first frame and return a pointer to it,
                                                // or return NULL if the list is empty.

                void            append(H264DecoderFrame *pFrame);
                    // Append the given frame to our tail

                void            insertList(H264DecoderFrameList &src);
                    // Move the given list to the beginning of our list.

                void            destroy(){};

                H264DecoderFrame   *findNextDisposable(void);
                    // Search through the list for the next disposable frame to decode into

                void            insertAtCurrent(H264DecoderFrame *pFrame);
                    // Inserts a frame immediately after the position pointed to by m_pCurrent

                void            resetCurrent(void) { m_pCurrent = m_pTail; }
                    // Resets the position of the current to the tail. This allows
                    // us to start "before" the head when we wrap around.

                void            setCurrent(H264DecoderFrame *pFrame) { m_pCurrent = pFrame; }
                    // Set the position of the current to pFrame

                void            removeAllRef();
                    // Mark all frames as not used as reference frames.
                void            IncreaseRefPicListResetCount(H264DecoderFrame *ExcludeFrame);
                    // Mark all frames as not used as reference frames.

                void            freeOldestShortTermRef();
                    // Mark the oldest short-term reference frame as not used.

                void            freeShortTermRef(Ipp32s PicNum);
                    // Mark the short-term reference frame with specified PicNum
                    // as not used

                void            freeLongTermRef(Ipp32s LongTermPicNum);
                    // Mark the long-term reference frame with specified LongTermPicNum
                    // as not used

                void            freeLongTermRefIdx(Ipp32s LongTermFrameIdx);
                    // Mark the long-term reference frame with specified LongTermFrameIdx
                    // as not used

                void            freeOldLongTermRef(Ipp32s MaxLongTermFrameIdx);
                    // Mark any long-term reference frame with LongTermFrameIdx greater
                    // than MaxLongTermFrameIdx as not used.

                void            changeSTtoLTRef(Ipp32s PicNum, Ipp32s LongTermFrameIdx);
                    // Mark the short-term reference frame with
                    // specified PicNum as long-term with specified long term idx.

                void            countActiveRefs(Ipp32u &NumShortTerm, Ipp32u &NumLongTerm);
                    // Return number of active short and long term reference frames.

                H264DecoderFrame   *findOldestDisplayable();
                    // Search through the list for the oldest displayable frame.

                Ipp32s                countNumDisplayable();
                    // Return number of displayable frames.

                void            removeAllDisplayable();
                    // Mark all frames as not displayable.

        protected:
            // Release object
            void Release(void);

            H264DecoderFrame *m_pHead;                          // (H264DecoderFrame *) pointer to first frame in list
            H264DecoderFrame *m_pTail;                          // (H264DecoderFrame *) pointer to last frame in list
            H264DecoderFrame *m_pCurrent;                       // (H264DecoderFrame *) pointer to current frame
            H264DecoderFrame *m_pFree;                          // (H264DecoderFrame *) pointer to single linked list of free frames
        };

        // Slice data struct used by  decoder for maintaining slice-level
        // data for a picture.
        struct H264SliceData {
            Ipp32s        DistScaleFactor[MAX_NUM_REF_FRAMES];
            Ipp32s        DistScaleFactorMV[MAX_NUM_REF_FRAMES];
            Ipp32s        DistScaleFactorAFF[2][2][2][MAX_NUM_REF_FRAMES]; // [curmb field],[ref1field],[ref0field]
            Ipp32s        DistScaleFactorMVAFF[2][2][2][MAX_NUM_REF_FRAMES]; // [curmb field],[ref1field],[ref0field]
//            Ipp32u        num_ref_idx_l0_active;            // num list 0 ref pics used to decode the slice
//            Ipp32u        num_ref_idx_l1_active;            // num list 0 ref pics used to decode the slice
//            Ipp8u        pic_parameter_set_id;            // of pic param set used for this slice
//            Ipp8u        disable_deblocking_filter_idc;        // deblock filter control, 0=filter all edges
//            Ipp8s        slice_alpha_c0_offset;                // deblock filter c0, alpha table offset
//            Ipp8s        slice_beta_offset;                    // deblock filter beta table offset
//           Ipp8u        luma_log2_weight_denom;            // luma weighting denominator
//            Ipp8u        chroma_log2_weight_denom;        // chroma weighting denominator
//            Ipp8s        chroma_qp_index_offset;            // offset to add to QP for chroma
//            EnumSliceCodType slice_type;
        };    // H264SliceData


        Ipp8u                         *m_pParsedData;
        Ipp8u                         *m_pParsedDataNew;
            // This points to a huge, monolithic buffer that contains data
            // derived from parsing the current frame.
            // Logically this information belongs in the H264DecoderFrame class.
            // However, algorithmically, we only need to keep around this
            // information for the most recent reference frame.
            // Thus, as a space saving optimization, we
            // allocate this information in the Decoder class rather than
            // in the H264DecoderFrame class.
        Ipp32s                          m_parsedDataLength;
        sDimensions                     m_paddedParsedDataSize;
        H264DecoderLocalMacroblockDescriptor m_mbinfo; //Local MB Data
        H264DecoderMBAddr *next_mb_tables[3];//0 linear scan 1 mbaff linear scan 2 - bitstream defined scan

        Ipp32s                          mb_width,mb_height;
        Ipp32s                          m_CurMB_QP;
#ifdef USE_SEI
        Ipp8u                           m_FrameProcessingStage;
        bool                            m_bHasSEI;
        Ipp8u                           m_SEITargetSPS;
#endif
        H264DecoderCurrentMacroblockDescriptor m_cur_mb;

    // forward declaration of internal types
    typedef void (H264VideoDecoder::*DeblockingFunction)(Ipp32u nMBAddr);
protected:

        H264SeqParamSet   m_SeqParamSet[MAX_NUM_SEQ_PARAM_SETS];// Sequence parameter sets read from the bitstream.
        H264SEIPayLoad    m_SEIPayLoads[MAX_NUM_SEQ_PARAM_SETS];

        H264PicParamSet   m_PicParamSet[MAX_NUM_PIC_PARAM_SETS];// Picture parameter sets read from the bitstream.
        struct H264DecoderStaticCoeffs
        {
            Ipp16s m_CoeffsBuffer[16 * 51 + DEFAULT_ALIGN_VALUE]; // (Ipp16s []) array of blocks to decode
            Ipp16s *Coeffs()
            {
                return align_pointer<Ipp16s *> (m_CoeffsBuffer, DEFAULT_ALIGN_VALUE);
            }
        } m_pCoeffBlocksBufStatic;
        struct H264DecoderStaticCoeffsExt
        {
            Ipp32s m_CoeffsBuffer[16 * 51 + DEFAULT_ALIGN_VALUE]; // (Ipp16s []) array of blocks to decode
            Ipp32s *Coeffs()
            {
                return align_pointer<Ipp32s *> (m_CoeffsBuffer, DEFAULT_ALIGN_VALUE);
            }
        } m_pCoeffBlocksBufStaticExt;

		AdaptiveMarkingInfo         m_FrameLevel_AdaptiveMarkingInfo;

        // At least one sequence parameter set and one picture parameter
        // set must have been read before a picture can be decoded.
        bool           m_bSeqParamSetRead;
        bool           m_bPicParamSetRead;
        //bool           sp_for_switch_flag;
        bool           m_NALRefIDC_kinoma_one_entry;//[2];

        // Keep track of which parameter set is in use.
        Ipp32s         m_CurrentSeqParamSet;
        Ipp32s         m_CurrentPicParamSet;
        bool           m_bDPBSizeChanged;
        bool           m_bSeqParamSetChanged;
        bool           m_WaitForDR;
        Ipp8s          GetReferenceField(Ipp8s *pFields,Ipp8s RefIndex)
        {
            if (RefIndex<0)
            {
                return -1;
            }
            else
            {
                VM_ASSERT(pFields[RefIndex]>=0);
                return pFields[RefIndex];
            }
        }
        Ipp64f         m_initialTR;
        Ipp32s         m_dpbSize;

        // This is the value at which the TR Wraps around for this
        // particular sequence.
        Ipp32s         m_MaxLongTermFrameIdx;


        // Forward and backward scaling ratios used in B frame direct mode
        // motion vector scaling.

        // Indicates whether we are delaying the display of frames
        // due to the fact that there might be B frames in the video
        // sequence.  Initially, latency is disabled.  However, if a
        // B frame has been seen or if the appropriate user interface
        // is used, then latency is enabled.

        // We allocate this object only once (in our constructor),
        // and use it to parse all of our bitstreams.
        bool           m_bNeedToCheckMBSliceEdges;
        H264Bitstream  *m_pFrameLevel_BitStream_xxx;
		//***
		unsigned char  *m_src;
		long		   m_src_size;


        // Declare space for our YUV reference frames and B frames.
        // We keep a doubly-linked list of reference frames (i.e.,
        // non B-frames).  Currently the list contains at most two decoded
        // reference frames.  An additional single buffer is maintained for
        // decoding B frames, and this buffer is not linked into the reference
        // frame list (although its previous() and future() pointers will
        // point into the reference frame list).
            // At Start_Sequence, all (2) of our reference frame buffers are
            // available, and are on this list.
        H264DecoderFrameList  m_H264DecoderFramesList;

            // This is the buffer that we decode B frames into.

        H264DecoderFrame     *m_pCurrentFrame;
        H264DecoderFrame     *m_pCurrentFrame_shadow;//***kinoma added
            // This points to either m_BFrame, or one of the frames in our
            // reference frame list.  It is the frame that we are currently
            // parsing in Decode().  It's previous() and future() methods
            // can be used to access the frames it is predicted from.
            // After a successful call to Decode(), this remains a valid
            // pointer, so that the application can use custom messages to
            // extract bitstream information for the just-decoded picture.
        H264DecoderFrame *m_pLastDecodedFrame;                  // (H264DecoderFrame *) last decoded frame (it is defferent from m_pCurrentFrame in fram-parallelization mode)

        H264DecoderFrame     *m_pDisplayFrame;
            // This points to either m_BFrame, or one of the frames in our
            // reference frame list.  It is the frame that we are going to
            // return from our Decode() invocation (or that we have returned
            // from our most recent Decode() invocation, if we're outside
            // the context of Decode()).  Due to B-frame latency, this may
            // not be the frame we are actually decoding.  Due to post
            // processing, the YUV data that we eventually display might
            // actually reside in one of our post processing buffers.
/*
        Ipp32u                *m_pMBIntraTypes;*/
        // Buffers (within m_pParsedFrameData) used to store list 0 and
        // list 1 reference index for each 4x4 subblock of a frame.
            // The differential coding of intra-coding types uses the intra
            // type of the above and left subblocks as predictors.
#define NUM_INTRA_TYPE_ELEMENTS 16
#define NUM_MVFLAG_ELEMENTS 16

            // Buffer (within m_pParsedData) used to store the number of
            // non-zero coefficients in a 4x4 block. Contains only the
            // data for the bottom blocks of one row of macroblocks,
            // 4 luma, 4 chroma.

            // Buffer used to store the number of non-zero coefficients in
            // a 4x4 block, containing the data for the right column of
            // 8 blocks of one macroblock (4 luma, 4 chroma).
        H264SliceData                 m_CurSliceInfo;
            // Array of H264SliceData structs used to store slice-level info about
            // each slice of the picture being decoded. Indexes are assigned
            // as new slices are encountered in the picture and are stored in
            // MBInfo for each MB to map MBs to slices. Allocated for worst
            // case number of slices (number of MBs).
        //***bnie: Ipp8u                      m_field_index_kinoma_always_zero;
        Ipp8u                      m_broken_buffer;
        Ipp32s                     m_broken_buffer_start_mb;
        Ipp16s                     m_broken_buffer_start_slice;
        Ipp32s                     m_NumFramesInL0List;
        Ipp32s                     m_NumFramesInL1List;
        Ipp32s                     m_NumFramesInLTList;

        Ipp8u                      m_NumShortEntriesInList;
        Ipp8u                      m_NumLongEntriesInList;
        Ipp8u                      m_FrameNumGapPresented;/* DEBUG : skipping was turned OFF, require to reimplement
        bool                       m_SkipThisPic;
        bool                       m_PreviousPicSkipped;*/

        Ipp8u                         *m_pMBMap;
		
		//***
		int						   m_approx;
		//***int						   m_maff;

private:

    H264SliceStore_ *m_pSliceStore;                              // (H264SliceStore *) pointer to slice manager
    H264SliceDecoder **m_pSliceDecoder;                         // (H264SliceDecoder *) pointer to array of slice decoders
    Ipp32s m_iSliceDecoderNum;                                  // (Ipp32u) count of slice decoders
        /////////////////////
        //                 //
        // Private methods //
        //                 //
        /////////////////////
        Status     AllocateParsedData( const sDimensions&, bool exactSizeRequested);
            // Reallocate m_pParsedData, if necessary, and initialize all the
            // various pointers that point into it.  This gets called
            // during Start_Sequence, and at each Decode invocation, based
            // on the incoming image dimensions.
            // If exactSizeRequested is false, then any existing
            // allocated buffer, even if way too big, will be reused without
            // being deallocated.  Othwerwise, any existing allocated buffer
            // will be first deallocated if is not exactly the size needed.

        void        DeallocateParsedData();

		
        // Parse frame
        //***bnie: Status ParseFrame(MediaData *pSource);
        // Decode frame
        Status DecodeFrame(MediaData3_V51* in);
        // Decode headers (sequence & picture)
        Status DecodeHeaders(MediaData3_V51 *pSource);
     
        // Prepare buffers & convert frame to output buffer
        Status OutputFrame(Ipp32u nStage);

        // Convert frame to output buffer
        Status ConvertFrame(MediaData_V51 *dst);
        // Get oldest frame to display
        H264DecoderFrame *GetFrameToDisplay(Ipp32u nStage);

        // Obtain free frame from queue
        H264DecoderFrame *GetFreeFrame(void);
        // Set current frame being decoded
        void SetCurrentFrame(H264DecoderFrame *pFrame);
        // Prepare decode buffer(s)
        Status PrepareDecodeBuffers(/*RefPicListReorderInfo *, RefPicListReorderInfo * */);
        //***bnie Status     ProcessTailNALUnits();
        //***bnie bool       DetectNewAU();
        //***bnie bool       IsNewAU(H264SliceHeader *fs,H264SliceHeader *ss,Ipp8u pic_order_cnt_type);
            // prepareDecodeBuffers is called at the beginning of Decode(),
            // to set up m_pCurrentFrame, m_pDisplayFrame and m_pMBInfo for
            // the decoding of the incoming frame. If we have to display a frame prior to
            // decoding, the display is handled here and m_pDisplayframe
            // will be left NULL.


        // Reconstruct into m_pCurrentFrame.
        Status      UpdateRefPicList(
                           H264SliceHeader SHdr,
                           RefPicListReorderInfo *pReorderInfo_L0,
                           RefPicListReorderInfo *pReorderInfo_L1,
                           Ipp32u uSliceNum);
            // Called after decoding a slice header to update the reference
            // picture lists using information in the slice header.
        void            InitPSliceRefPicList(
                            Ipp32s NumL0RefActive,
                            H264DecoderFrame **pRefPicList);

        void            InitBSliceRefPicLists(
                            Ipp32s NumL0RefActive,
                            Ipp32s NumL1RefActive,
                            H264DecoderFrame **pRefPicList0,
                            H264DecoderFrame **pRefPicList1);

        void            ReOrderRefPicList(
                            H264DecoderFrame **pRefPicList,
                            RefPicListReorderInfo *pReorderInfo,
                            Ipp32s uMaxPicNum,
                            Ipp32s uNumRefActive);


        void                ProcessFrameNumGap();

        void            InitDistScaleFactor(
                            Ipp32s NumL0RefActive,
                            H264DecoderFrame **pRefPicList0,
                            H264DecoderFrame **pRefPicList1);
            // Called after decoding a slice header to initialize
            // m_CurSliceInfo.DistScaleFactor array.

        Status              UpdateRefPicMarking();
        void                DecodePictureOrderCount();
        Status              SetDPBSize();
		//***
        Status              ForceDPBSize(long dpbSize);
            // After decoding a frame, updates marking of reference pictures
            // in decoded frame buffer.

        Status              SetMBMap();
            // Initialize MB ordering for the picture using slice groups as
            // defined in the picture parameter set.


        void                ResetGMBData();

        Status      AdjustIndex             (
                                             Ipp8u ref_mb_is_bottom,
                                             Ipp8s ref_mb_is_field,
                                             Ipp8s& RefIdx);
        void        ComputeDirectSpatialRefIdx(
                                            Ipp8s *pRefIndexL0,    // neighbor RefIdx relative to this, forward result here
                                            Ipp8s *pRefIndexL1        // neighbor RefIdx relative to this, backward result here
                                            );

        static const Ipp8u ICBPTAB[6];

public:

           H264VideoDecoder();

           ~H264VideoDecoder();

//////////////////////
// interface methods
/////////////////////
        Status      Init                    (BaseCodecParams_V51 *init);

        // Decode & get decoded frame
		Status GetFrame(MediaData_V51* in, MediaData_V51* out) {return 0;};
        Status GetFrame(MediaData3_V51* in);
        
        //***
        Status GetYUV(int display_order, YUVPlannar *yuv);
		Status GetDimension( long *width16, long *height16, long *left, long *top, long *right, long *bottom );
		void   SetApprox(int level ) { m_approx = level; }

		//***WWD
		//***bnie: int  GetMaff() {return m_maff; }

        Status      Close                    ();

        Status    Reset();

        Status GetInfo(BaseCodecParams_V51* info);
        Ipp32s              m_SkipFlag;
        Ipp32s              m_SkipCycle;
        Ipp32s              m_ModSkipCycle;

        // reset skip frame counter
        Status    ResetSkipCount()
        {
            if (IS_DECODE_ONLY_INTRA_REF_SLICES(m_NeedToSkip))
            {
                m_WaitForDR = true;//set true to return to normal frame processing
            }
            m_NeedToSkip = 0;
            m_SkipFlag = 0;
            return UMC_OK;
        };
        // increment skip frame counter
        Ipp32s m_VideoDecodingSpeed;
        virtual Status    ChangeVideoDecodingSpeed(int& num)
        {
            m_VideoDecodingSpeed+=num;
            if (m_VideoDecodingSpeed<0) m_VideoDecodingSpeed=0;
            if (m_VideoDecodingSpeed>6) m_VideoDecodingSpeed=6;
            num=m_VideoDecodingSpeed;
            if (m_VideoDecodingSpeed>5)
            {
                m_SkipCycle=1;
                m_ModSkipCycle=1;
                m_PermanentTurnOffDeblocking = 1;
            }
            else if (m_VideoDecodingSpeed>4)
            {
                m_SkipCycle=3;
                m_ModSkipCycle=2;
                m_PermanentTurnOffDeblocking = 1;
            }
            else if (m_VideoDecodingSpeed>3)
            {
                m_SkipCycle=2;
                m_ModSkipCycle=2;
                m_PermanentTurnOffDeblocking = 1;
            }
            else if (m_VideoDecodingSpeed>2)
            {
                m_SkipCycle=3;
                m_ModSkipCycle=3;
                m_PermanentTurnOffDeblocking = 1;
            }
            else if (m_VideoDecodingSpeed>1)
            {
                m_SkipCycle=4;
                m_ModSkipCycle=4;
                m_PermanentTurnOffDeblocking = 1;
            }
            else if (m_VideoDecodingSpeed==1)
            {
                m_PermanentTurnOffDeblocking = 1;
            }
            else
            {
                m_PermanentTurnOffDeblocking = 0;
            }

            return UMC_OK;
        }
        Status    SkipVideoFrame(int num)
        {
            INCREASE_SKIP_REPEAT_COUNTER(m_SkipRepeat);
            m_AskedSkipped+=num;
            //***bnie:vm_debug_trace(-1,VM_STRING("Skip Frame %d %d %d\n"),num,m_AskedSkipped,m_ReallySkipped);
            if (m_SkipRepeat>MAX_SKIP_REPEAT)
            {
                m_PermanentTurnOffDeblocking=1;
                m_SkipRepeat=MAX_SKIP_REPEAT;
            }
            m_NeedToSkip += num;
            if (m_NeedToSkip>SKIP_THRESHOLD4)
            {
                m_SkipCycle=1;
                m_ModSkipCycle=1;
            }
            else if (m_NeedToSkip>SKIP_THRESHOLD3)
            {
                m_SkipCycle=3;
                m_ModSkipCycle=2;
            }
            else if (m_NeedToSkip>SKIP_THRESHOLD2)
            {
                m_SkipCycle=2;
                m_ModSkipCycle=2;
            }
            else if (m_NeedToSkip>SKIP_THRESHOLD1)
            {
                m_SkipCycle=3;
                m_ModSkipCycle=3;
            }
            else
            {
                m_SkipCycle=4;
                m_ModSkipCycle=4;
            }
            //limit skipValue to SKIP_NONREF_FRAMES_MODE
            //if (m_NeedToSkip>=SKIP_NONREF_FRAMES_MODE) m_NeedToSkip=SKIP_NONREF_FRAMES_MODE-1;
            return UMC_OK;
        };
        // get skip frame counter statistic
        vm_var32 GetSkipedFrame()
        {
            return m_ReallySkipped;
        };

protected:

    unsigned long                         ulResized;
    bool                                  m_bTwoPictures;        // (bool) two picture output

    unsigned long                         m_lFlags;
    //***double                                m_local_frame_time;
    //***double                                m_local_delta_frame_time;
    bool                                  m_bIsDecodingStarted;

    int                                   m_NeedToSkip;
    int                                   m_SkipRepeat;
    int                                   m_PermanentTurnOffDeblocking;
    int                                   m_ReallySkipped;
    int                                   m_AskedSkipped;
    int                                   m_getframe_calls;
////
    //***MediaData                             m_swp_bfr;
    //***bnie bool                                  m_bDataNotSwapped;
    Ipp32u                                m_FrameNum;
    Ipp32s                                m_FrameNumGap;
    Ipp32u                                m_PicOrderCnt;
    Ipp32u                                m_PicOrderCntMsb;
    Ipp32u                                m_PicOrderCntLsb;
    Ipp32s                                m_FrameNumOffset;
    Ipp32u                                m_TopFieldPOC_kinoma_question;
    Ipp32u                                m_BottomFieldPOC_kinoma_question;

    Ipp32s                                SwapBuffer(unsigned long *src, unsigned long *dst,  long len);
    Status                                InitSwapBuffer();
    void                                  FreeSwapBuffer();

    //***bnie: H264SliceHeader m_CurSliceHeader;
    H264SliceHeader *m_FirstSliceHeader;
    //***bnie: H264SliceHeader m_TempSliceHeader;

    //***PredWeightTable m_pPredWeight_L0_whoecares1[MAX_NUM_REF_FRAMES];
    //***PredWeightTable m_pPredWeight_L1_whoecares2[MAX_NUM_REF_FRAMES];

    //
    // Threading tools
    //

    // Initialize conversion threading tools
    //*** void InitializeConversionThreading();
    // Release conversion threading tools
    //*** void ReleaseConversionThreading();
    // Convert frame asynchronously
    //*** Status ConvertFrameAsync(MediaData *dst, Ipp32u nStage);
    // Working routine for second conversion thread
    //*** static
    //*** unsigned int ConvertFrameAsyncSecondThread(void *p);

    //*** vm_event            m_hStartConversion;    // (vm_event) start conversion event
    //*** vm_event            m_hStopConversion;     // (vm_event) stop conversion event
    //*** vm_thread           m_hConversionThread;   // (vm_thread) handle to async conversion thread
    bool                m_bQuit;               // (bool) quit flag

//*** #define SWAP_BUFFER_SIZE 100*1024;            //default swap buffer size
                                              //increase this value in case of it is too small
//***     unsigned int        m_uiSwpBuffSize;      //in case of input data in byte order
                                              //we need temporary buffer for sawpping

public:


    //***TimingInfo m_clsTimingInfo;
};


#pragma pack()

// Declare function to swapping memory
void SwapMemoryAndRemovePreventingBytes(void *pDst, size_t &nDstSize, void *pSrc, size_t nSrcSize);
//***

int SwapInQTMediaSample(int spspps_only, int naluLengthSize, unsigned char *src, int size, void *m );
int Delete_MediaDataEx(void *m_void );

#define ALLOK 0
#define PREDICTION_FROM_TOP 1
#define PREDICTION_FROM_BOTTOM 2

inline
Ipp8s SelectPredictionMethod(Ipp32s MBYoffset,Ipp32s mvy,Ipp32s sbheight,Ipp32s height)
{
    Ipp32s padded_y = (mvy&3)>0?3:0;
    mvy>>=2;
    if (mvy-padded_y+MBYoffset<0)
    {
        return PREDICTION_FROM_TOP;
    }
    if (mvy+padded_y+MBYoffset+sbheight>=height)
    {
        return PREDICTION_FROM_BOTTOM;
    }

    return ALLOK;
}

Status get_sps_pps(MediaData3_V51 *pSource, H264SeqParamSet *sps, H264PicParamSet *pps );


} // end namespace UMC



#endif // __UMC_H264_DEC_H__
