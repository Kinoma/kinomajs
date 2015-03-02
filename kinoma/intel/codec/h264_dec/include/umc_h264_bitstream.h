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
#ifndef __UMC_H264_BITSTREAM_H_
#define __UMC_H264_BITSTREAM_H_

//***
#include "kinoma_ipp_lib.h"

#include <string.h>
#include "ippdefs.h"
#include "umc_video_decoder.h"
#include "umc_h264_dec_internal_cabac.h"
#include "umc_h264_dec_init_tables_cabac.h"
#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_dec_tables.h"
#include "vm_debug.h"
#include "ippvc.h"

using namespace UMC_H264_DECODER;
namespace UMC
{
#ifdef STORE_CABAC_BITS
extern FILE *cabac_bits;
extern Ipp32s sym_cnt;
#endif

#define _h264Get12Bits(current_data, offset, data)                      \
{                                                                       \
    Ipp32u x;                                                           \
                                                                        \
    VM_ASSERT(offset >= 0 && offset <= 31);                             \
                                                                        \
/*  removeSCEBP(current_data, offset);*/                                \
                                                                        \
    offset -= 12;                                                       \
                                                                        \
    if(offset < 0)                                                      \
    {                                                                   \
        offset &= 31;                                                   \
                                                                        \
        x = current_data[1] >> (offset);                                \
        x >>= 1;                                                        \
        x += current_data[0] << (31 - offset);                          \
        current_data++;                                                 \
    }                                                                   \
    else                                                                \
    {                                                                   \
        x = current_data[0] >> (offset + 1);                            \
    }                                                                   \
                                                                        \
    VM_ASSERT(offset >= 0 && offset <= 31);                             \
                                                                        \
    (data) = x & 0x00000fff;                                            \
}

#define _h264GetBits(current_data, offset, nbits, data)                    \
{                                                                       \
    Ipp32u x;                                                           \
                                                                        \
    VM_ASSERT((nbits) > 0 && (nbits) <= 32);                               \
    VM_ASSERT(offset >= 0 && offset <= 31);                                \
                                                                        \
/*  removeSCEBP(current_data, offset);*/                                \
                                                                        \
    offset -= (nbits);                                                  \
                                                                        \
    if(offset >= 0)                                                     \
    {                                                                   \
        x = current_data[0] >> (offset + 1);                            \
    }                                                                   \
    else                                                                \
    {                                                                   \
        offset += 32;                                                   \
                                                                        \
        x = current_data[1] >> (offset);                                \
        x >>= 1;                                                        \
        x += current_data[0] << (31 - offset);                          \
        current_data++;                                                 \
    }                                                                   \
                                                                        \
    VM_ASSERT(offset >= 0 && offset <= 31);                                \
                                                                        \
    (data) = x & (((Ipp32u)0x01 << (nbits)) - 1);                       \
}
#define ippiGetBits1( current_data, offset, data)   _h264GetBits(current_data, offset,  1, data);
#define ippiGetBits8( current_data, offset, data)   _h264GetBits(current_data, offset,  8, data);
#define ippiGetBits16( current_data, offset, data)  _h264GetBits(current_data, offset,  16, data);
#define ippiGetNBits( current_data, offset, nbits, data) _h264GetBits(current_data, offset, nbits, data);
#define ippiUngetNBits(current_data, offset, nbits)                     \
{                                                                       \
    VM_ASSERT(offset >= 0 && offset <= 31);                                \
                                                                        \
    offset += (nbits);                                                  \
    if(offset > 31)                                                     \
    {                                                                   \
        offset -= 32;                                                   \
        current_data--;                                                 \
    }                                                                   \
                                                                        \
    VM_ASSERT(offset >= 0 && offset <= 31);                                \
}
#define ippiUngetBits32(current_data, offset)                           \
    VM_ASSERT(offset >= 0 && offset <= 31);                                \
    current_data--;                                                     \

#define ippiAlignBSPointerRight(current_data, offset)                   \
{                                                                       \
    if((offset & 0x07) != 0x07)                                         \
    {                                                                   \
        offset = (offset | 0x07) - 8;                                   \
        if(offset == -1)                                                \
        {                                                               \
            offset = 31;                                                \
            current_data++;                                             \
        }                                                               \
    }                                                                   \
}
#define ippiNextBits(current_data, offset, nbits, data)             \
{                                                                       \
    Ipp32s bp;                                                          \
    Ipp32u x;                                                           \
                                                                        \
    VM_ASSERT((nbits) >= 0 && (nbits) <= 32);                              \
    VM_ASSERT(offset >= 0 && offset <= 31);                                \
                                                                        \
    bp = offset - (nbits);                                              \
                                                                        \
    if(bp < 0)                                                          \
    {                                                                   \
        bp += 32;                                                       \
        x = current_data[1] >> bp;                                      \
        x >>= 1;                                                        \
        x += current_data[0] << (31 - bp);                              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        x = current_data[0] >> bp;                                      \
        x >>= 1;                                                        \
    }                                                                   \
                                                                        \
    (data) = x & (((Ipp32u)0x01 << (nbits)) - 1);                       \
}


#define CABAC_Load16Bit()                               \
{                                                       \
    if (16 >= m_ValOffset)                              \
    {                                                   \
        Ipp32s lCount = 2;                                \
        do                                              \
        {                                               \
            Ipp32u lTemp;                               \
            ippiGetBits8(m_pbs, m_bitOffset, lTemp)     \
            if ((2 == m_null_count) && (3 == lTemp))    \
            {                                           \
                ippiGetBits8(m_pbs, m_bitOffset, lTemp) \
                m_null_count = 0;                       \
            };                                          \
            m_BufValue = (m_BufValue << 8) | lTemp;     \
            if(0 == lTemp)                              \
                m_null_count++;                         \
            else                                        \
                m_null_count = 0;                       \
        } while (--lCount);                             \
        m_ValOffset += 16;                              \
    }                                                   \
}

// NAL unit definitions
#define NAL_STORAGE_IDC_BITS    0x60
#define NAL_UNITTYPE_BITS       0x1f

#define NUM_BLOCK_TYPES        8
#define NUM_MB_TYPE_CTX       11
#define NUM_BLOCK_TYPE_CTX     9
#define NUM_MVD_CTX           10
#define NUM_REF_NO_CTX         6
#define NUM_DELTA_QAUNT_CTX    4
#define NUM_MB_AFF_CTX         4


#define NUM_INTRA_PRED_CTX             2
#define NUM_CR_INTRA_PRED_CTX          4
#define NUM_CBP_CTX                    4
#define NUM_BLOCK_CBP_CTX              4
#define NUM_SIGN_COEFF_FLAG_CTX       15
#define NUM_LAST_SIGN_COEFF_FLAG_CTX  15
#define NUM_COEF_ONE_LEVEL_CTX         5
#define NUM_COEF_ABS_LEVEL_CTX         5

typedef struct CABAC_CONTEXT
{
    Ipp32u pStateIdx;                       // (unsigned Ipp16s Ipp32s) probability state index
    Ipp32u valMPS;                          // (unsigned Ipp16s Ipp32s) value of most probable symbol

} CABAC_CONTEXT;

class H264Bitstream
{
    DYNAMIC_CAST_DECL_BASE(H264Bitstream)
public:
        H264Bitstream       (Ipp8u* const   pb,  const Ipp32u   maxsize);

        H264Bitstream       ();

        ~H264Bitstream      ();

protected:
        Ipp32u* m_pbs;      // m_pbs points to the current position of the buffer.
        Ipp32s m_bitOffset; // Indicates the bit position (0 to 31) in the byte pointed by m_pbs.

#ifdef __INTEL_IPP__		//***bnie 2/10/2009 not used for other implementations
        IppVCHuffmanSpec_32s*   m_tblCoeffToken[5];
        IppVCHuffmanSpec_32s*   m_tblRunBefore [16];
        IppVCHuffmanSpec_32s*   m_tblTotalZeros[16];

        IppVCHuffmanSpec_32s*   m_tblTotalZerosCR[8];//*** changed from 4  --bnie 8/19/2008
        IppVCHuffmanSpec_32s*   m_tblTotalZerosCR422[8];
#endif

        Ipp32u*  m_pbsBase;  // m_pbsBase points to the first byte of the buffer.
        
		//***kinoma enhancement   --bnie 6/12/2008
		Ipp32u*  m_pbsBoundary;   // m_pbsBase points to the last 4 bytes of the buffer.
		Ipp32s   m_pbsBoundaryOffset;   // m_pbsBase points to the last 4 bytes of the buffer.

        Ipp32u   m_maxBsSize; // Maximum buffer size in bytes.

        Ipp32u*  m_pbsRBSPBase;  // Points to beginning of previous
                                 // "Raw Byte Sequence Payload"
private:
        Status InitTables();
        Status FreeTables();

public:
	
    //***bnie: Status    AdvanceToNextSCP();
    //***bnie: Status    More_RBSP_Data();

    inline
    void AlignPointerRight()
    {
        ippiAlignBSPointerRight(m_pbs,m_bitOffset);
    }
#ifndef DROP_CABAC
    // Initialize CABAC decoding engine
    void InitializeDecodingEngine_CABAC(void);
    void TerminateDecode_CABAC(void);

    // Initialize CABAC context(s) in intra slices
    void InitializeContextVariablesIntra_CABAC(Ipp32s SliceQPy);

    // Initialize CABAC context(s) in inter slices
    void InitializeContextVariablesInter_CABAC(Ipp32s SliceQPy,
                                               Ipp32s cabac_init_idc);
    // Calculate residual block
    void ResidualBlock4x4_CABAC(Ipp32s ctxBlockCat,
                                const Ipp32u* ctxBase,
                                Ipp32s coef_ctr,
                                const Ipp32s* single_scan,
                                Ipp16s*pPosCoefbuf);

    void ResidualChromaDCBlockMH_CABAC(
                                const Ipp32u* ctxBase,
                                Ipp16s*pPosCoefbuf);
    void ResidualChromaDCBlockH2_CABAC(
                                const Ipp32u* ctxBase,
                                Ipp16s*pPosCoefbuf);
    void ResidualChromaDCBlockH4_CABAC(
                                const Ipp32u* ctxBase,
                                const Ipp32s* single_scan,
                                Ipp16s*pPosCoefbuf);


    void ResidualBlock8x8_CABAC(bool field_decoding_flag,
                               const Ipp32s* single_scan,
                               Ipp16s*pPosCoefbuf);


    inline Ipp32u DecodeSingleBinOnes_CABAC(Ipp32u ctxIdx,
                                           Ipp32s &binIdx);

    // Decode block coefficient value mimus one
    inline Ipp32s  DecodeCoeffAbsLevelMinus1_CABAC(Ipp32u ctxIdxOffset,
                                         Ipp32u &numDecodAbsLevelEq1,
                                         Ipp32u &numDecodAbsLevelGt1);
    // Decode signed coefficient value
    inline Ipp32s  DecodeSignedLevel_CABAC(Ipp32u ctxIdxOffset,
                                 Ipp32u &numDecodAbsLevelEq1,
                                 Ipp32u &numDecodAbsLevelGt1,
                                 Ipp32u max_value);

    // Decode single bin from stream
    inline Ipp32u DecodeSingleBin_CABAC(Ipp32u ctxIdx);

    // Decode single bin using bypass decoding
    inline Ipp32u DecodeBypass_CABAC();

    inline Ipp32s DecodeBypassSign_CABAC(Ipp32s val);

    // Decode multiple bins using bypass decoding until ==1
    inline Ipp32u DecodeBypassOnes_CABAC();

    // Decode end symbol
    inline Ipp32u DecodeSymbolEnd_CABAC();

protected:
    CABAC_CONTEXT context_array[460];// array of cabac context(s)
    Ipp32u m_lcodIRange;       // arithmetic decoding engine variable
    Ipp32u m_lcodIOffset;      // arithmetic decoding engine variable
    Ipp32u m_validCABACbits;   // arithmetic decoding engine extension
#endif

public:
    //***void            Reset( Ipp8u* const pb, const Ipp32u maxsize);
    void            Reset( Ipp8u const *pb, Ipp32s const bit_offset, Ipp32s const maxsize);

    //***Ipp32s            GetSCP();

    Status          GetNALUnitType( NAL_Unit_Type &uNALUnitType,
                                    Ipp8u &uNALStorageIDC);

    Status          GetSequenceParamSet(H264SeqParamSet *sps);

    void            GetScalingList4x4(H264ScalingList4x4 *scl,Ipp8u *def,Ipp8u *scl_type);
    void            GetScalingList8x8(H264ScalingList8x8 *scl,Ipp8u *def,Ipp8u *scl_type);

    void inline FillFlatScalingList4x4(H264ScalingList4x4 *scl)
    {
        for (Ipp32s i=0;i<16;i++) scl->ScalingListCoeffs[i]=16;
    }
    void  inline FillFlatScalingList8x8(H264ScalingList8x8 *scl)
    {
        for (Ipp32s i=0;i<64;i++) scl->ScalingListCoeffs[i]=16;
    }

    void  inline FillScalingList4x4(H264ScalingList4x4 *scl_dst,Ipp8u *coefs_src)
    {
        for (Ipp32s i=0;i<16;i++) scl_dst->ScalingListCoeffs[i]=coefs_src[i];
    }

    void  inline FillScalingList8x8(H264ScalingList8x8 *scl_dst,Ipp8u *coefs_src)
    {
        for (Ipp32s i=0;i<64;i++) scl_dst->ScalingListCoeffs[i]=coefs_src[i];
    }

    Status          GetPictureParamSetPart1(H264PicParamSet *pps);
    Status          GetPictureParamSetPart2(H264PicParamSet  *pps,const H264SeqParamSet *sps);
    Status          GetPictureParamSet( H264PicParamSet *pps, Ipp8u pic_order_cnt_type);

    Status          GetPictureDelimiter( Ipp32u& PicCodType);

    Status          ReadFillerData();
    void            RollbackCurrentNALU();

    Status          GetSliceHeaderPart1(H264SliceHeader *pSliceHeader);

    Status          GetSliceHeaderPart2(H264SliceHeader *hdr,       // slice header read goes here
                                        PredWeightTable *pPredWeight_L0, // L0 weight table goes here
                                        PredWeightTable *pPredWeight_L1, // L1 weight table goes here
                                        RefPicListReorderInfo *pReorderInfo_L0,
                                        RefPicListReorderInfo *pReorderInfo_L1,
                                        AdaptiveMarkingInfo *pAdaptiveMarkingInfo,
                                        const H264PicParamSet *pps,
                                        bool bIsIDRSlice,
                                        const H264SeqParamSet *sps,
                                            Ipp8u NALRef_idc,          // from slice header NAL unit
                                        bool bIsSearch);        // called during next picture search?

#ifndef DROP_SEI
//sei related functions
public:
        Ipp8u         ParseSEI(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);

private:
        Ipp8u          sei_message(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          sei_payload(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          buffering_period(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          pic_timing(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          pan_scan_rect(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          filler_payload(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          user_data_registered_itu_t_t35(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          user_data_unregistered(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          recovery_point(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          dec_ref_pic_marking_repetition(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          spare_pic(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          scene_info(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          sub_seq_info(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          sub_seq_layer_characteristics(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          sub_seq_characteristics(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          full_frame_freeze(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          full_frame_freeze_release(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          full_frame_snapshot(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          progressive_refinement_segment_start(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          progressive_refinement_segment_end(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          motion_constrained_slice_group_set(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          reserved_sei_message(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
        Ipp8u          unparsed_sei_message(H264SeqParamSet *sps,Ipp8u current_sps,H264SEIPayLoad *spl);
#endif

public:
        Status  GetCAVLCInfoLuma  ( Ipp32u uVLCSelect,      // N, obtained from num coeffs of above/left blocks
                                    Ipp16s uMaxNumCoeff,
                                    Ipp16s &uNumCoeff,
                                    Ipp16s  *&pPosCoefbuf,
                                    Ipp8s hp_4x4_block_index);

        Status  GetCAVLCInfoChroma0  (Ipp16s &sNumCoeff,
                                    Ipp16s  *&pPosCoefbuf);

        Status  GetCAVLCInfoChroma2  (Ipp16s &sNumCoeff,
                                    Ipp16s  *&pPosCoefbuf);
        Status  GetCAVLCInfoChroma4  (Ipp16s &sNumCoeff,
                                    Ipp16s  *&pPosCoefbuf,
                                    Ipp8u field_flag);

        void    GetState    (Ipp32u** pbs, Ipp32u* bitOffset);
        void    SetState    (Ipp32u* pbs, Ipp32u bitOffset);
        Ipp32s  GetVLCElement(bool bIsSigned);  // Read one VLC signed or unsigned value from bitstream
		//***       
		Ipp32s  GetVLCElement_signed(void);  // Read one VLC signed or unsigned value from bitstream
        Ipp32s  GetVLCElement_unsigned(void);  // Read one VLC signed or unsigned value from bitstream

        inline Ipp32u  GetBits     (const Ipp32u nbits);   // Reads bits from buffer.

        inline Ipp32u  Get1Bit     ();                     // Reads one bit from the buffer.

        inline Ipp32u  Get12Bits();

        bool    SearchBits  ( const Ipp32u  nbits,// Searches for a code with known number of bits.
                              const Ipp32u  code,
                              const Ipp32u  lookahead);
        bool    NextBit     ();

        //***inline void   DiscardZeroPadding();
        // Set current decoding position
        void SetDecodedBytes(size_t);

        inline size_t
        BytesDecoded()
        {
            return static_cast<size_t>((Ipp8u*)m_pbs - (Ipp8u*)m_pbsBase) +
                    ((31 - m_bitOffset) >> 3);
        }
        inline size_t
            BytesLeft() { return((Ipp32s)m_maxBsSize - (Ipp32s) BytesDecoded()); }

        inline size_t
        BytesDecodedRoundOff()
        {
            return static_cast<size_t>((Ipp8u*)m_pbs - (Ipp8u*)m_pbsBase);
        }
public:
        Ipp32s totalDecodedBytes;
        Ipp32s GetTotalDecodedBytes() { return(totalDecodedBytes + (Ipp32s)BytesDecoded()); }
		
		//***kinoma enhancement  --bnie 6/12/2008
        Ipp32s ReachBoundary(); 
};

// ---------------------------------------------------------------------------
//      H264Bitstream::GetBits()
//      Reads bits from buffer.  Supports up to 25 bits.
//      nbits   : number of bits to be read
// ---------------------------------------------------------------------------
inline
Ipp32u H264Bitstream::GetBits(const Ipp32u nbits)
{
    Ipp32u w,n = nbits;

    ippiGetNBits(m_pbs, m_bitOffset, n, w);
    return(w);

} // H264Bitstream::GetBits()

inline
Ipp32u H264Bitstream::Get12Bits()
{
    Ipp32u w;

    _h264Get12Bits(m_pbs, m_bitOffset, w);
    return(w);

} // H264Bitstream::GetBits()

inline
Ipp32u H264Bitstream::Get1Bit()
{
    Ipp32u w;

    ippiGetBits1(m_pbs, m_bitOffset, w);
    return(w);

} // H264Bitstream::Get1Bit()

// ---------------------------------------------------------------------------
//      NextBit()
//      Check next 1 bit.  Bitstream state,
//      pointer and bit offset, will be updated if code equal to 1.
// ---------------------------------------------------------------------------

inline
bool H264Bitstream::NextBit()
{
    Ipp32s bp;
    Ipp32u w;

    bp = m_bitOffset - 1;

    if(bp < 0)
    {
        w = m_pbs[0] & 1;
        if(w)
        {
            m_pbs++;
            m_bitOffset = 31;
            return true;
        }
    }
    else
    {
        w = m_pbs[0] >> m_bitOffset;
        w = w & 1;
        if(w)
        {
            m_bitOffset = bp;
            return true;
        }
    }
    return false;

} // H264Bitstream::SearchBits()

#define CABAC_LMB 20

inline
Ipp32u Get12Bits(Ipp32u** m_pbs, Ipp32s* m_bitOffset)
{
    Ipp32u w;

    _h264Get12Bits((*m_pbs), (*m_bitOffset), w);
    return(w);

}
inline
Ipp32u decodeBin_c(Ipp32u &pStateIdx, Ipp32u &valMPS,
                   Ipp32u &codIOffset, Ipp32u &codIRange,
                   Ipp32u &validCABACbits,
                   Ipp32u** m_pbs, Ipp32s* m_bitOffset)
{
    // See subclause 9.3.3.2.1 of H.264 standard

    Ipp32u codIRangeLPS;
    Ipp32u binVal;

    codIRangeLPS = (rangeTabLPS20[pStateIdx]
                    [((codIRange >> (6+CABAC_LMB)))]);

    codIRange -= codIRangeLPS;
    binVal     = valMPS;
    pStateIdx  =  transIdxMPS[pStateIdx];

    if (codIOffset >= codIRange)
    {
        binVal      ^= 1;
        codIOffset -= codIRange;
        codIRange   = codIRangeLPS;

        valMPS ^=  pStateIdx == 1 ? 1 :0;
        pStateIdx =  transIdxLPS[pStateIdx-1];

    }

    // Renormalization process
    // See subclause 9.3.3.2.2 of H.264
    //if (codIRange < (0x100<<(CABAC_LMB)))
    {
        unsigned char numBits = NumBitsToGetTbl[codIRange>>CABAC_LMB];
        codIRange <<= numBits;
        codIOffset <<= numBits;
#if (CABAC_LMB > 0)
        validCABACbits -= numBits;
        if (validCABACbits < 8)
        {
            // add 8 more bits.
            Ipp32u bits = Get12Bits(m_pbs, m_bitOffset);//GetBits(CABAC_LMB-8);
            validCABACbits += CABAC_LMB-8;
            bits <<= (CABAC_LMB-validCABACbits);
            codIOffset |= bits;
        }
#else
        codIOffset |= GetBits(NumBitsToGet);
#endif
    }

    return binVal;
}

#ifndef DROP_CABAC
inline
Ipp32u H264Bitstream::DecodeSingleBin_CABAC(Ipp32u ctxIdx)
{
 /*   return decodeBin_c(context_array[ctxIdx].pStateIdx,
                       context_array[ctxIdx].valMPS,
                       m_lcodIOffset,
                       m_lcodIRange,
                       m_validCABACbits,
                       &m_pbs, &m_bitOffset);
    // See subclause 9.3.3.2.1 of H.264 standard
*/
    //Ipp32u &pStateIdx = context_array[ctxIdx].pStateIdx;
    //Ipp32u &valMPS = context_array[ctxIdx].valMPS;

    Ipp32u codIOffset = m_lcodIOffset;
    Ipp32u codIRange = m_lcodIRange;
    Ipp32u codIRangeLPS;
    Ipp32u binVal;
#ifdef STORE_CABAC_BITS
    Ipp32u preStateIdx = context_array[ctxIdx].pStateIdx;
    Ipp32u prevalMPS = context_array[ctxIdx].valMPS;
#endif

    codIRangeLPS = (rangeTabLPS20[context_array[ctxIdx].pStateIdx]
                    [((codIRange >> (6+CABAC_LMB)))]);

    codIRange -= codIRangeLPS;
    binVal     = context_array[ctxIdx].valMPS;
    context_array[ctxIdx].pStateIdx =
        transIdxMPS[context_array[ctxIdx].pStateIdx];

    if (codIOffset >= codIRange)
    {
        binVal      ^= 1;
        codIOffset -= codIRange;
        codIRange   = codIRangeLPS;

        context_array[ctxIdx].valMPS ^=
            context_array[ctxIdx].pStateIdx == 1 ? 1 :0;
        context_array[ctxIdx].pStateIdx =
            transIdxLPS[context_array[ctxIdx].pStateIdx-1];

    }

    // Renormalization process
    // See subclause 9.3.3.2.2 of H.264
    //if (codIRange < (0x100<<(CABAC_LMB)))
    {
        unsigned char numBits = NumBitsToGetTbl[codIRange>>CABAC_LMB];
        codIRange <<= numBits;
        codIOffset <<= numBits;
#if (CABAC_LMB > 0)
        m_validCABACbits -= numBits;
        if (m_validCABACbits < 8)
        {
            // add 8 more bits.
            Ipp32u bits = Get12Bits();//GetBits(CABAC_LMB-8);
            m_validCABACbits += CABAC_LMB-8;
            bits <<= (CABAC_LMB-m_validCABACbits);
            codIOffset |= bits;
        }
#else
        codIOffset |= GetBits(NumBitsToGet);
#endif
    }


#ifdef STORE_CABAC_BITS
    sym_cnt++;
    if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
    if(cabac_bits)
#ifdef CABAC_DECORER_COMP
        fprintf(cabac_bits,"sb %d %d %d %d %d\n",
        ctxIdx,
        codIRange>>CABAC_LMB,
        codIOffset>>CABAC_LMB,
        binVal,sym_cnt);
#else
        fprintf(cabac_bits,"sb %d %d %d %d %d %d %d\n",ctxIdx,preStateIdx,prevalMPS,context_array[ctxIdx].pStateIdx,context_array[ctxIdx].valMPS,binVal,sym_cnt);
#endif
#endif
    m_lcodIOffset = codIOffset;
    m_lcodIRange = codIRange;

    return binVal;

} //Ipp32s H264Bitstream::DecodeSingleBin_CABAC(Ipp32s ctxIdx)

inline
Ipp32u H264Bitstream::DecodeSymbolEnd_CABAC(void)
{
    Ipp32u binVal = 1;
    Ipp32u codIOffset = m_lcodIOffset;
    Ipp32u codIRange = m_lcodIRange;

    // See subclause 9.3.3.2.4 of H.264 standard
    if (codIOffset < (codIRange - (2<<CABAC_LMB)))
    {
        codIRange -= (2<<CABAC_LMB);

        // Renormalization process
        // See subclause 9.3.3.2.2 of H.264
        if (codIRange < (0x100<<(CABAC_LMB)))
        {
            codIRange <<= 1;
            codIOffset <<= 1;
#if (CABAC_LMB > 0)
            m_validCABACbits -= 1;
#else
            codIOffset |= GetBits(1);
#endif
        }
        binVal = 0;
        m_lcodIOffset = codIOffset;
        m_lcodIRange = codIRange;

    }

#ifdef STORE_CABAC_BITS
    sym_cnt++;

    if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
    if(cabac_bits)
#ifdef CABAC_DECORER_COMP
        fprintf(cabac_bits,"fsb %d %d %d %d\n",m_lcodIRange>>CABAC_LMB,
                                               m_lcodIOffset>>CABAC_LMB,
                                               binVal,sym_cnt);
#else
    fprintf(cabac_bits,"fsb %d %d\n",binVal,sym_cnt);
#endif
#endif
    return binVal;

} //Ipp32s H264Bitstream::DecodeSymbolEnd_CABAC(void)

inline
Ipp32u H264Bitstream::DecodeBypass_CABAC(void)
{
    // See subclause 9.3.3.2.3 of H.264 standard
    Ipp32u binVal;// = 0;
#if (CABAC_LMB > 0)
    m_lcodIOffset = (m_lcodIOffset << 1);
    m_validCABACbits--;
    if (m_validCABACbits < 8)
    {
        // add 8 more bits.
        Ipp32u bits = Get12Bits();//GetBits(CABAC_LMB-8);
        m_validCABACbits += CABAC_LMB-8;
        bits <<= (CABAC_LMB-m_validCABACbits);
        m_lcodIOffset |= bits;
    }
#else
    m_lcodIOffset = (m_lcodIOffset << 1) | Get1Bit();
#endif
/*
    Ipp32s mask = ((Ipp32s)(m_lcodIRange)-1-(Ipp32s)(m_lcodIOffset))>>31;
    // conditionally negate level
    binVal = mask&1;
    // conditionally subtract range from offset
    m_lcodIOffset -= m_lcodIRange & mask;
*/
    if(m_lcodIOffset >= m_lcodIRange)
    {
        binVal = 1;
        m_lcodIOffset -= m_lcodIRange;
   }
    else
    {
        binVal = 0;
    }
#ifdef STORE_CABAC_BITS
    sym_cnt++;
    if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
    if(cabac_bits)
#ifdef CABAC_DECORER_COMP
        fprintf(cabac_bits,"bp %d %d %d %d\n",m_lcodIRange>>CABAC_LMB,
                                              m_lcodIOffset>>CABAC_LMB,
                                              binVal,sym_cnt);
#else
    fprintf(cabac_bits,"bp %d %d\n",binVal,sym_cnt);
#endif
#endif
    return binVal;

} //Ipp32s H264Bitstream::DecodeBypass_CABAC(void)

inline
Ipp32s H264Bitstream::DecodeBypassSign_CABAC(Ipp32s val)
{
    // See subclause 9.3.3.2.3 of H.264 standard
    Ipp32s binVal;// = 0;

#if (CABAC_LMB > 0)
    m_lcodIOffset = (m_lcodIOffset << 1);
    m_validCABACbits--;
    if (m_validCABACbits < 8)
    {
        // add 8 more bits.
        Ipp32u bits = Get12Bits();//GetBits(CABAC_LMB-8);
        m_validCABACbits += CABAC_LMB-8;
        bits <<= (CABAC_LMB-m_validCABACbits);
        m_lcodIOffset |= bits;
    }
#else
    m_lcodIOffset = (m_lcodIOffset << 1) | Get1Bit();
#endif
/*
    Ipp32s mask = ((Ipp32s)(m_lcodIRange)-1-(Ipp32s)(m_lcodIOffset))>>31;
    // conditionally negate level
    binVal = mask&1;
    // conditionally subtract range from offset
    m_lcodIOffset -= m_lcodIRange & mask;
*/
    if(m_lcodIOffset >= m_lcodIRange)
    {
        binVal = -val;
        m_lcodIOffset -= m_lcodIRange;
   }
    else
    {
        binVal = val;
    }
#ifdef STORE_CABAC_BITS
    sym_cnt++;
    if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
    if(cabac_bits)
#ifdef CABAC_DECORER_COMP
        fprintf(cabac_bits,"bp %d %d %d %d\n",m_lcodIRange>>CABAC_LMB,
                                              m_lcodIOffset>>CABAC_LMB,
                                              binVal<0,sym_cnt);
#else
    fprintf(cabac_bits,"bp %d %d\n",binVal<0,sym_cnt);
#endif
#endif
    return binVal;

} //Ipp32s H264Bitstream::DecodeBypassSign_CABAC()



#endif	//DROP_CABAC

#if 0	//***bnie: not used
inline
void removeSCEBP(Ipp32u* &current_data, Ipp32s &offset)
{
   Ipp32s num1[] = {2, 2, -6, -6};
   Ipp32s num2[] = {1, 1, 1, -7};

   if((offset & 7) == 7)
    {
        Ipp32s             a = offset>>3;
        unsigned char* tmp = ((unsigned char*)current_data) + a;
        if(tmp[0] == 3)
        {
            if(!tmp[num1[a]] && !tmp[num2[a]])
            {
                offset -= 8;
                current_data += (offset&0x20)>>5;
                offset&=0x1f;

            }

        }
    }

}

#endif

inline
Ipp32s  H264Bitstream::GetVLCElement(bool bIsSigned)
{
    Ipp16s sval = 0;
	
    ippiDecodeExpGolombOne_H264_1u16s_x(&m_pbs, &m_bitOffset, &sval, bIsSigned);
    return sval;
}

//***
inline
Ipp32s  H264Bitstream::GetVLCElement_signed(void)
{
	//***
    return ippiDecodeExpGolombOne_H264_1u16s_signed_x(&m_pbs, &m_bitOffset);
}

inline
Ipp32s  H264Bitstream::GetVLCElement_unsigned(void)
{
	//***
    return ippiDecodeExpGolombOne_H264_1u16s_unsigned_x(&m_pbs, &m_bitOffset);
}


// ---------------------------------------------------------------------------
// GetCAVLCInfoChroma()
//
// Calls CAVLC bitstream decoding functions to obtain nonzero coefficients
// and related information, returning in passed buffers and passed-by-reference
// parameters.
// Bitstream pointer and offset are updated by called functions and are
// updated on return.
// ---------------------------------------------------------------------------
//temporal declaration
IPPAPI(IppStatus, ippiDecodeCAVLCChromaDcCoeffs422_H264_1u16s, (Ipp32u **ppBitStream,
       Ipp32s *pOffset,
       Ipp16s *pNumCoeff,
       Ipp16s **ppDstCoeffs,
       const Ipp32s *pTblCoeffToken,
       const Ipp32s **ppTblTotalZerosCR,
       const Ipp32s **ppTblRunBefore))

inline
Status H264Bitstream::GetCAVLCInfoChroma0  (Ipp16s &sNumCoeff,
                                           Ipp16s  *&pPosCoefbuf
                                           )
{
    if(ippStsNoErr < ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_x(&m_pbs,
        &m_bitOffset,
        &sNumCoeff,
        &pPosCoefbuf,
#ifdef __INTEL_IPP__
        m_tblCoeffToken[3],
        (const Ipp32s **) m_tblTotalZerosCR,
        (const Ipp32s **) m_tblRunBefore))
#else
		NULL, NULL, NULL))
#endif
    {
        return UMC_BAD_STREAM;
    }
    return UMC_OK;
}


inline
Status H264Bitstream::GetCAVLCInfoChroma2  (Ipp16s &sNumCoeff,
                                           Ipp16s  *&pPosCoefbuf
                                           )
{
    if(ippStsNoErr < ippiDecodeCAVLCChromaDcCoeffs422_H264_1u16s(&m_pbs,
        &m_bitOffset,
        &sNumCoeff,
        &pPosCoefbuf,
#ifdef __INTEL_IPP__
        m_tblCoeffToken[4],
        (const Ipp32s **) m_tblTotalZerosCR422,
        (const Ipp32s **) m_tblRunBefore))
#else
		NULL, NULL, NULL))
#endif
    {
        return UMC_BAD_STREAM;
    }
    return UMC_OK;
}

inline
Status H264Bitstream::GetCAVLCInfoChroma4  (Ipp16s &sNumCoeff,
                                           Ipp16s  *&pPosCoefbuf,
                                           Ipp8u field_flag
                                           )
{
    if(ippStsNoErr < ippiDecodeCAVLCCoeffs_H264_1u16s_x(&m_pbs,
        &m_bitOffset,
        &sNumCoeff,
        &pPosCoefbuf,
        0,
        16,
#ifdef __INTEL_IPP__
        (const Ipp32s **) m_tblCoeffToken,
        (const Ipp32s **) m_tblTotalZeros,
        (const Ipp32s **) m_tblRunBefore,
#else
		NULL, NULL, NULL,
#endif        
        (Ipp32s*)mp_scan4x4[field_flag]))

    {
        return UMC_BAD_STREAM;
    }
    return UMC_OK;
}


// ---------------------------------------------------------------------------
// C_GetCAVLCInfoLuma()
//
// Calls CAVLC bitstream decoding functions to obtain nonzero coefficients
// and related information, returning in passed buffers and passed-by-reference
// parameters.
// Bitstream pointer and offset are updated by called functions and are
// updated on return.
// ---------------------------------------------------------------------------
inline
Status H264Bitstream::GetCAVLCInfoLuma(Ipp32u uVLCSelect,       // N, obtained from num coeffs of above/left blocks
                                        Ipp16s uMaxNumCoeff,
                                        Ipp16s &sNumCoeff,
                                        Ipp16s  *&pPosCoefbuf,// buffer to return up to 16
                                        Ipp8s hp_4x4_block_index)

{
    Status umcRes = UMC_OK;

    Ipp32s *scan_matr = NULL;
    if (hp_4x4_block_index<0)
    {
        scan_matr = (Ipp32s*)mp_scan4x4[0];//***bniebnie
    }
    if(ippStsNoErr < ippiDecodeCAVLCCoeffs_H264_1u16s_x(&m_pbs,
                                                      &m_bitOffset,
                                                      &sNumCoeff,
                                                      &pPosCoefbuf,
                                                      uVLCSelect,
                                                      uMaxNumCoeff,
#ifdef __INTEL_IPP__
                                                      (const Ipp32s **) m_tblCoeffToken,
                                                      (const Ipp32s **) m_tblTotalZeros,
                                                      (const Ipp32s **) m_tblRunBefore,
#else
													  NULL, NULL, NULL,
#endif
                                                      scan_matr))

    {
        umcRes = UMC_BAD_STREAM;
    }
    return umcRes;
}

#if 0
inline
void H264Bitstream::DiscardZeroPadding()
{
    Ipp32s rest_bytes = (Ipp32s)(m_maxBsSize - BytesDecoded());
    Ipp32s zero_byte = 0xff;

    if(rest_bytes > 4)
    {
        if(!GetSCP())
        {
            zero_byte = GetBits(8);

            while(rest_bytes > 0 && zero_byte == 0)
            {
                if(!GetSCP())
                    zero_byte = GetBits(8);
                else
                {
                    ippiUngetNBits(m_pbs,m_bitOffset,24);
                    break;
                }

            }
            if(zero_byte != 0)
            {
                ippiUngetNBits(m_pbs,m_bitOffset,8);
            }
        }
        else
        {
            ippiUngetNBits(m_pbs,m_bitOffset,24);
        }
    }

}
#endif

} // end namespace UMC

#endif // __UMC_H264_BITSTREAM_H_
