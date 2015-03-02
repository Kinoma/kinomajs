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
//***
#include "kinoma_ipp_lib.h"

#include "umc_h264_bitstream.h"
#include "vm_debug.h"
#include "vm_time.h"

#include "umc_h264_dec_coeff_token_map.h"
#include "umc_h264_dec_total_zero.h"
#include "umc_h264_dec_run_before.h"
#include "umc_h264_dec_ipplevel.h"

namespace UMC
{

// ---------------------------------------------------------------------------
//      H264Bitstream::GetState()
//        Obtains current position of the buffer pointer.  This is only
//        used to maintain the bitstream state when calling an assembly
//        routine that accesses the bitstream.
//        *pbs        : to be assigned with the new pointer value
//        *bitOffset    : to be assigned with the new bit offset
// ---------------------------------------------------------------------------
    static const Ipp32u GetBitsMask[25] =
    {
        0x00000000, 0x00000001, 0x00000003, 0x00000007,
            0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
            0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
            0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
            0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
            0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
            0x00ffffff
    };

void H264Bitstream::GetState(Ipp32u** pbs,Ipp32u* bitOffset)
{
    *pbs       = m_pbs;
    *bitOffset = m_bitOffset;

} // H264Bitstream::GetState()
void H264Bitstream::SetState(Ipp32u* pbs,Ipp32u bitOffset)
{
    m_pbs = pbs;
    m_bitOffset = bitOffset;

} // H264Bitstream::GetState()


//  H264Bitstream::H264Bitstream()
//        H.264 bitstream constructor used in the decoder
// ---------------------------------------------------------------------------

H264Bitstream::H264Bitstream(Ipp8u * const pb,
                             const Ipp32u maxsize)
{
    Status sts;
    m_pbsBase    = (Ipp32u*)pb;
	
	//***kinoma enhancement   --bnie 6/13/2008
    m_pbsBoundary = m_pbsBase + (maxsize>>2);
	m_pbsBoundaryOffset = 31 - ((maxsize&0x03)<<3);

    m_pbs        = (Ipp32u*)pb;
    m_bitOffset    = 31;
    m_maxBsSize    = maxsize;
    m_pbsRBSPBase = m_pbsBase;

    sts = InitTables();
    VM_ASSERT(sts == UMC_OK);

    totalDecodedBytes = 0;

} // H264Bitstream::H264Bitstream(Ipp8u * const pb,


//***kinoma enhancement  --bnie 8/3/2008
Ipp32s H264Bitstream::ReachBoundary()
{ 
	Ipp32u *this_m_pbs		= m_pbs;
	Ipp32s this_m_bitOffset = m_bitOffset;

	if( this_m_bitOffset < 0 )
	{
		this_m_pbs++;
		this_m_bitOffset += 32;
	}

	if( this_m_pbs > m_pbsBoundary )
		return 1;
	else if( this_m_pbs == m_pbsBoundary  && this_m_bitOffset < m_pbsBoundaryOffset )
		return 1;

	return 0; 
}


// ---------------------------------------------------------------------------
//  H264Bitstream::H264Bitstream()
//        default constructor used in the decoder
// ---------------------------------------------------------------------------
H264Bitstream::H264Bitstream(void)
{
    Status  sts;
    m_pbsRBSPBase = m_pbsBase = 0;
    m_pbs       = 0;
    m_pbsBase   = 0;
	
	//***kinoma enhancement   --bnie 6/13/2008
    m_pbsBoundary = 0;
	m_pbsBoundaryOffset = 0;

    m_bitOffset = 0;
    m_maxBsSize    = 0;
    totalDecodedBytes = 0;

    sts = InitTables();
    VM_ASSERT(sts == UMC_OK);

} // H264Bitstream::H264Bitstream(void)

// ---------------------------------------------------------------------------
//  H264Bitstream::~H264Bitstream()
// ---------------------------------------------------------------------------

H264Bitstream::~H264Bitstream()
{
    FreeTables();
} // H264Bitstream::~H264Bitstream()

Status H264Bitstream::FreeTables()
{
#ifdef __INTEL_IPP__
    Ipp32s i;
    ////////////////////////////////////////////////////////////
    //Number Coeffs and Trailing Ones map tables free//////////
    ////////////////////////////////////////////////////////////

    for (i = 0; i <= 4; i++ )
    {
        if(m_tblCoeffToken[i])
        {
            ippiHuffmanTableFree_32s_x(m_tblCoeffToken[i]);
            m_tblCoeffToken[i] = NULL;
        }
    }
    ////////////////////////////////////////////////////////////
    /////////////////////TotalZeros tables free/////////////////
    ////////////////////////////////////////////////////////////

    for(i = 1; i <= 15; i++)
    {
        if(m_tblTotalZeros[i])
        {
            ippiHuffmanTableFree_32s_x(m_tblTotalZeros[i]);
            m_tblTotalZeros[i] = NULL;
        }
    }

    for(i = 1; i <= 3; i++)
    {
        if(m_tblTotalZerosCR[i])
        {
            ippiHuffmanTableFree_32s_x(m_tblTotalZerosCR[i]);
            m_tblTotalZerosCR[i] = NULL;
        }
    }
    for(i = 1; i <= 7; i++)
    {
        if(m_tblTotalZerosCR422[i])
        {
            ippiHuffmanTableFree_32s_x(m_tblTotalZerosCR422[i]);
            m_tblTotalZerosCR[i] = NULL;
        }
    }

    ///////////////////////////
    ///Run Befores free////////
    ///////////////////////////
    for(i = 1; i <= 7; i++)
    {
        if(m_tblRunBefore[i])
        {
            ippiHuffmanTableFree_32s_x(m_tblRunBefore[i]);
            m_tblRunBefore[i] = NULL;
        }
    }
    m_tblRunBefore[8]  = NULL;
    m_tblRunBefore[9]  = NULL;
    m_tblRunBefore[10] = NULL;
    m_tblRunBefore[11] = NULL;
    m_tblRunBefore[12] = NULL;
    m_tblRunBefore[13] = NULL;
    m_tblRunBefore[14] = NULL;
    m_tblRunBefore[15] = NULL;
#endif

    return UMC_OK;
}

Status H264Bitstream::InitTables()
{
#ifdef __INTEL_IPP__
    IppStatus ippSts;

    ////////////////////////////////////////////////////////////
    //Number Coeffs and Trailing Ones map tables alloc/////////
    ////////////////////////////////////////////////////////////

    ippSts = ippiHuffmanRunLevelTableInitAlloc_32s_x(coeff_token_map_02, &m_tblCoeffToken[0]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanRunLevelTableInitAlloc_32s_x(coeff_token_map_24, &m_tblCoeffToken[1]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanRunLevelTableInitAlloc_32s_x(coeff_token_map_48, &m_tblCoeffToken[2]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanRunLevelTableInitAlloc_32s_x(coeff_token_map_cr, &m_tblCoeffToken[3]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;
    ippSts = ippiHuffmanRunLevelTableInitAlloc_32s_x(coeff_token_map_cr2, &m_tblCoeffToken[4]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ////////////////////////////////////////////////////////////
    /////////////////////TotalZeros tables alloc///////////////
    ////////////////////////////////////////////////////////////
    m_tblTotalZerosCR[0]  = NULL;
    m_tblTotalZerosCR422[0]  = NULL;
    m_tblTotalZeros[0]  = NULL;


    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr1, &m_tblTotalZerosCR[1]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr2, &m_tblTotalZerosCR[2]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr3, &m_tblTotalZerosCR[3]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_1, &m_tblTotalZerosCR422[1]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_2, &m_tblTotalZerosCR422[2]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_3, &m_tblTotalZerosCR422[3]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_4, &m_tblTotalZerosCR422[4]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_5, &m_tblTotalZerosCR422[5]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_6, &m_tblTotalZerosCR422[6]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_cr422_7, &m_tblTotalZerosCR422[7]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;


    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_1, &m_tblTotalZeros[1]);


    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_2, &m_tblTotalZeros[2]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_3, &m_tblTotalZeros[3]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_4, &m_tblTotalZeros[4]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_5, &m_tblTotalZeros[5]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_6, &m_tblTotalZeros[6]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_7, &m_tblTotalZeros[7]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_8, &m_tblTotalZeros[8]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_9, &m_tblTotalZeros[9]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_10, &m_tblTotalZeros[10]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_11, &m_tblTotalZeros[11]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_12, &m_tblTotalZeros[12]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_13, &m_tblTotalZeros[13]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_14, &m_tblTotalZeros[14]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(total_zeros_map_15, &m_tblTotalZeros[15]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ///////////////////////////
    ///Run Befores alloc///////
    ///////////////////////////
    m_tblRunBefore[0]  = NULL;


    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_1, &m_tblRunBefore[1]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_2, &m_tblRunBefore[2]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_3, &m_tblRunBefore[3]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_4, &m_tblRunBefore[4]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_5, &m_tblRunBefore[5]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_6, &m_tblRunBefore[6]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    ippSts = ippiHuffmanTableInitAlloc_32s_x(run_before_map_6p, &m_tblRunBefore[7]);
    if(ippSts!=ippStsNoErr)
        return UMC_ALLOC;

    m_tblRunBefore[8]  = m_tblRunBefore[7];
    m_tblRunBefore[9]  = m_tblRunBefore[7];
    m_tblRunBefore[10] = m_tblRunBefore[7];
    m_tblRunBefore[11] = m_tblRunBefore[7];
    m_tblRunBefore[12] = m_tblRunBefore[7];
    m_tblRunBefore[13] = m_tblRunBefore[7];
    m_tblRunBefore[14] = m_tblRunBefore[7];
    m_tblRunBefore[15] = m_tblRunBefore[7];
#endif

    return UMC_OK;

} // H264Bitstream::H264Bitstream()



// ---------------------------------------------------------------------------
//  H264Bitstream::Reset()
//        reset bitstream; used in the decoder
// ---------------------------------------------------------------------------
#if 0
void H264Bitstream::Reset(Ipp8u * const pb, const Ipp32u maxsize)
{
    totalDecodedBytes += (Ipp32s) BytesDecoded();
    m_pbs       = (Ipp32u*)pb;
    m_pbsBase   = (Ipp32u*)pb;
	
	//***kinoma enhancement   --bnie 6/13/2008
    m_pbsBoundary = m_pbsBase + (maxsize>>2);
	m_pbsBoundaryOffset = 31 - ((maxsize&0x03)<<3);
    
    m_bitOffset = 31;
    m_maxBsSize    = maxsize;

    m_pbsRBSPBase = m_pbsBase;

} // void H264Bitstream::Reset(Ipp8u * const pb, const Ipp32u maxsize)
#endif

void H264Bitstream::Reset(Ipp8u const * pb, Ipp32s const offset, const Ipp32s maxsize)
{
    totalDecodedBytes += (Ipp32s) BytesDecoded();
    m_pbs       = (Ipp32u*)pb;
    m_pbsBase   = (Ipp32u*)pb;

	//***kinoma enhancement   --bnie 6/13/2008
    m_pbsBoundary = m_pbsBase + (maxsize>>2);
	m_pbsBoundaryOffset = 31 - ((maxsize&0x03)<<3);

    m_bitOffset = offset;
    m_maxBsSize    = maxsize;

    m_pbsRBSPBase = m_pbsBase;

} // void H264Bitstream::Reset(Ipp8u * const pb, Ipp32s offset, const Ipp32u maxsize)


void H264Bitstream::RollbackCurrentNALU()
{
    ippiUngetBits32(m_pbs,m_bitOffset);
}

#if 0
// ---------------------------------------------------------------------------
//  H264Bitstream::GetSCP()
//  Determine if next bistream symbol is a start code. If not,
//  do not change bitstream position and return false. If yes, advance
//  bitstream position to first symbol following SCP and return true.
//
//  A start code is:
//  * next 2 bytes are zero
//  * next non-zero byte is '01'
//  * may include extra stuffing zero bytes
// ---------------------------------------------------------------------------
Ipp32s H264Bitstream::GetSCP()
{
    Ipp32s code, code1,tmp;
    Ipp32u* ptr_state = m_pbs;
    Ipp32s  bit_state = m_bitOffset;
    VM_ASSERT(m_bitOffset >= 0 && m_bitOffset <= 31);
    tmp = (m_bitOffset+1)%8;
    if(tmp)
    {
        ippiGetNBits(m_pbs, m_bitOffset, tmp ,code);
        if ((code << (8 - tmp)) & 0x7f)    // most sig bit could be rbsp stop bit
        {
            m_pbs = ptr_state;
            m_bitOffset = bit_state;
            return 0;    // not rbsp if following non-zero bits
        }
		else
			return -1;		// ///*** WWD_BUG92 : indicate stop bit  
    }
    else
    {
        Ipp32s remaining_bytes = (Ipp32s)m_maxBsSize - (Ipp32s) BytesDecoded();
        if (remaining_bytes<1)
            return -1; //signilizes end of buffer
        ippiNextBits(m_pbs, m_bitOffset,8, code);
        if (code == 0x80)
        {
            // skip past trailing RBSP stop bit
            ippiGetBits8(m_pbs, m_bitOffset, code);
        }
    }
    Ipp32s remaining_bytes = (Ipp32s)BytesLeft();
    if (remaining_bytes<1)
        return -1; //signilizes end of buffer
    //ippiNextBits(m_pbs, m_bitOffset,8, code);

    ippiGetBits8(m_pbs, m_bitOffset, code);
    if(code != 0) {
        m_pbs = ptr_state;
        m_bitOffset = bit_state;
        return 0;
    }
    if(remaining_bytes<2) {
        return(-1);
    }
    ippiGetBits8(m_pbs, m_bitOffset, code1);
    if (code1 != 0)
    {
        m_pbs = ptr_state;
        m_bitOffset = bit_state;
        return 0;
    }
    ippiGetBits8(m_pbs, m_bitOffset, code);
    Ipp32s max_search_length = (Ipp32s)BytesLeft();

    while (code == 0 && max_search_length-->0)
    {
        ippiGetBits8(m_pbs, m_bitOffset, code);

    }
    if (max_search_length<1)
        return -1;
    if (code != 1)
    {
        m_pbs = ptr_state;
        m_bitOffset = bit_state;
        return 0;
    }

    return 1;

} // H264Bitstream::GetSCP()

Status H264Bitstream::AdvanceToNextSCP()
{
    // Search bitstream for next start code:
    // 3 bytes:  0 0 1

    Ipp32s max_search_length = (Ipp32s)(m_maxBsSize - (Ipp32u)(((Ipp8u *) m_pbs) - ((Ipp8u *) m_pbsBase)));
    Ipp32u t1,t2,t3;
    Ipp32s p;

    if((m_bitOffset+1)%8)
    {
        ippiAlignBSPointerRight(m_pbs, m_bitOffset);
    }

    ippiGetBits8(m_pbs, m_bitOffset, t1);
    ippiGetBits8(m_pbs, m_bitOffset, t2);
    ippiGetBits8(m_pbs, m_bitOffset, t3);

    for (p = 0; p < max_search_length - 2; p++)
    {
        if (t1==0 && t2==0 && t3==1)
        {
            ippiUngetNBits(m_pbs,m_bitOffset,24);
            return UMC_OK;
        }
        t1=t2;
        t2=t3;
        ippiGetBits8(m_pbs, m_bitOffset, t3);

    }

    return  UMC_BAD_STREAM;

} // Status H264Bitstream::AdvanceToNextSCP()


Status H264Bitstream::More_RBSP_Data()
{
#if 0
    Ipp32u code;Ipp32u bits_to_show;
    Ipp32u* ptr_state = m_pbs;
    Ipp32s  bit_state = m_bitOffset;
    Ipp32s remaining_bytes;
    if (AdvanceToNextSCP()==UMC_OK)
        remaining_bytes = ((Ipp32s)m_pbs) - ((Ipp32s)ptr_state)+((bit_state/8)-(m_bitOffset/8))-(((31-bit_state)/8)-((31-m_bitOffset)/8));
    else
        remaining_bytes = (((Ipp32s)m_pbsBase)+((Ipp32s)m_maxBsSize)) - ((Ipp32s)ptr_state)-((31-bit_state)/8);
    m_pbs = ptr_state;
    m_bitOffset = bit_state;
    if (remaining_bytes>1) return UMC_OK;
    bits_to_show=m_bitOffset%8;
    ippiNextBits(m_pbs, m_bitOffset,bits_to_show, code)
    if (code!=1<<(bits_to_show-1)) return UMC_OK;
    return UMC_END_OF_STREAM;
#else
    Ipp32u* ptr_state = m_pbs;
    Ipp32s  bit_state = m_bitOffset;
    Ipp32s r=GetSCP();
    m_pbs = ptr_state;
    m_bitOffset = bit_state;
    if (r==0) return UMC_OK;
    return UMC_END_OF_STREAM;
#endif
}
#endif
// ---------------------------------------------------------------------------
//  H264Bitstream::GetNALUnitType()
//    Bitstream position is expected to be at the start of a NAL unit.
//    Read and return NAL unit type and NAL storage idc.
// ---------------------------------------------------------------------------
#ifdef STORE_NUTS
FILE *fnuts;
Ipp32s fnuts_num;
#endif

Status H264Bitstream::GetNALUnitType( NAL_Unit_Type &uNALUnitType,Ipp8u &uNALStorageIDC)
{
    Ipp32u code;
    ippiGetBits8(m_pbs, m_bitOffset, code);

    uNALStorageIDC = (Ipp8u)(code & NAL_STORAGE_IDC_BITS)>>5;
    uNALUnitType = (NAL_Unit_Type)(code & NAL_UNITTYPE_BITS);
#ifdef STORE_NUTS
    if (fnuts==NULL) fnuts=fopen(__NUTS_FILE__,"w+t");
    if (fnuts)
    {
        fprintf(fnuts,"%d %d\n",uNALUnitType,++fnuts_num);
        fflush(fnuts);
    }
#endif
    return UMC_OK;
}    // GetNALUnitType


void H264Bitstream::GetScalingList4x4(H264ScalingList4x4 *scl,Ipp8u *def,Ipp8u *scl_type)
{
    Ipp32u lastScale = 8;
    Ipp32u nextScale = 8;
    bool DefaultMatrix = false;
    Ipp32s j;

    for (j = 0; j < 16; j++ )
    {
        if (nextScale != 0)
        {
            Ipp32s delta_scale  = GetVLCElement_signed();
            nextScale = ( lastScale + delta_scale + 256 ) & 0xff;
            DefaultMatrix = ( j == 0 && nextScale == 0 );
        }
        scl->ScalingListCoeffs[ mp_scan4x4[0][j] ] = ( nextScale == 0 ) ? (Ipp8u)lastScale : (Ipp8u)nextScale;
        lastScale = scl->ScalingListCoeffs[ mp_scan4x4[0][j] ];
    }
    if (!DefaultMatrix)
    {
        *scl_type=SCLREDEFINED;
        return;
    }
    *scl_type= SCLDEFAULT;
    FillScalingList4x4(scl,def);
    return;
}

#ifndef DROP_HIGH_PROFILE
void H264Bitstream::GetScalingList8x8(H264ScalingList8x8 *scl,Ipp8u *def,Ipp8u *scl_type)
{
    Ipp32u lastScale = 8;
    Ipp32u nextScale = 8;
    bool DefaultMatrix=false;
    Ipp32s j;

    for (j = 0; j < 64; j++ )
    {
        if (nextScale != 0)
        {
            Ipp32s delta_scale  = GetVLCElement_signed();
            nextScale = ( lastScale + delta_scale + 256 ) & 0xff;
            DefaultMatrix = ( j == 0 && nextScale == 0 );
        }
        scl->ScalingListCoeffs[ hp_scan8x8[0][j] ] = ( nextScale == 0 ) ? (Ipp8u)lastScale : (Ipp8u)nextScale;
        lastScale = scl->ScalingListCoeffs[ hp_scan8x8[0][j] ];
    }
    if (!DefaultMatrix)
    {
        *scl_type=SCLREDEFINED;
        return;
    }
    *scl_type= SCLDEFAULT;
    FillScalingList8x8(scl,def);
    return;

}
#endif

// ---------------------------------------------------------------------------
//  H264Bitstream::GetSequenceParamSet()
//    Read sequence parameter set data from bitstream.
// ---------------------------------------------------------------------------
Status H264Bitstream::GetSequenceParamSet(H264SeqParamSet *sps)
{
    // Not all members of the seq param set structure are contained in all
    // seq param sets. So start by init all to zero.
    Status ps = UMC_OK;
    ippsZero_8u_x((Ipp8u*)sps, sizeof (H264SeqParamSet));

    // skip NALU 8 bits
    GetBits(8);

    // profile
    // TBD: add rejection of unsupported profile
    sps->profile_idc = (Ipp8u)GetBits(8);
    sps->constrained_set0_flag = (Ipp8u)Get1Bit();
    sps->constrained_set1_flag = (Ipp8u)Get1Bit();
    sps->constrained_set2_flag = (Ipp8u)Get1Bit();
    sps->constrained_set3_flag = (Ipp8u)Get1Bit();

    // skip 4 zero bits
    GetBits(4);

    sps->level_idc = (Ipp8u)GetBits(8);
    // id
    sps->seq_parameter_set_id = (Ipp8u)GetVLCElement_unsigned();
    if (sps->seq_parameter_set_id > MAX_NUM_SEQ_PARAM_SETS-1)
    {
        ps = UMC_BAD_FORMAT;
    }

    if((sps->profile_idc==100) || (sps->profile_idc==110) ||(sps->profile_idc==122) ||(sps->profile_idc==144))
#ifndef DROP_HIGH_PROFILE    
	{
        sps->chroma_format_idc = (Ipp8u) GetVLCElement_unsigned();
        if (sps->chroma_format_idc==3)
        {
            sps->residual_colour_transform_flag = (Ipp8u) Get1Bit();
        }
        sps->bit_depth_luma = (Ipp8u) GetVLCElement_unsigned()+8;
        sps->bit_depth_chroma = (Ipp8u) GetVLCElement_unsigned()+8;
        VM_ASSERT(sps->bit_depth_luma == 8);
        VM_ASSERT(sps->bit_depth_chroma == 8);
        VM_ASSERT(sps->residual_colour_transform_flag == 1);
        if (sps->bit_depth_luma != 8 || sps->bit_depth_chroma != 8 || sps->residual_colour_transform_flag == 1)
        {
            ps = UMC_UNSUPPORTED;
        }
        sps->qpprime_y_zero_transform_bypass_flag = (Ipp8u)Get1Bit();
        sps->seq_scaling_matrix_present_flag = (Ipp8u)Get1Bit();
        if(sps->seq_scaling_matrix_present_flag)
        {
            // 0
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[0],(Ipp8u*)default_intra_scaling_list4x4,&sps->type_of_scaling_list_used[0]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[0],(Ipp8u*) default_intra_scaling_list4x4);
                sps->type_of_scaling_list_used[0] = SCLDEFAULT;
            }
            // 1
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[1],(Ipp8u*) default_intra_scaling_list4x4,&sps->type_of_scaling_list_used[1]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[1],(Ipp8u*) sps->ScalingLists4x4[0].ScalingListCoeffs);
                sps->type_of_scaling_list_used[1] = SCLDEFAULT;
            }
            // 2
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[2],(Ipp8u*) default_intra_scaling_list4x4,&sps->type_of_scaling_list_used[2]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[2],(Ipp8u*) sps->ScalingLists4x4[1].ScalingListCoeffs);
                sps->type_of_scaling_list_used[2] = SCLDEFAULT;
            }
            // 3
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[3],(Ipp8u*)default_inter_scaling_list4x4,&sps->type_of_scaling_list_used[3]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[3],(Ipp8u*) default_inter_scaling_list4x4);
                sps->type_of_scaling_list_used[3] = SCLDEFAULT;
            }
            // 4
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[4],(Ipp8u*) default_inter_scaling_list4x4,&sps->type_of_scaling_list_used[4]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[4],(Ipp8u*) sps->ScalingLists4x4[3].ScalingListCoeffs);
                sps->type_of_scaling_list_used[4] = SCLDEFAULT;
            }
            // 5
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[5],(Ipp8u*) default_inter_scaling_list4x4,&sps->type_of_scaling_list_used[5]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[5],(Ipp8u*) sps->ScalingLists4x4[4].ScalingListCoeffs);
                sps->type_of_scaling_list_used[5] = SCLDEFAULT;
            }



            // 0
            if(Get1Bit())
            {
                GetScalingList8x8(&sps->ScalingLists8x8[0],(Ipp8u*)default_intra_scaling_list8x8,&sps->type_of_scaling_list_used[6]);
            }
            else
            {
                FillScalingList8x8(&sps->ScalingLists8x8[0],(Ipp8u*) default_intra_scaling_list8x8);
                sps->type_of_scaling_list_used[6] = SCLDEFAULT;
            }
            // 1
            if(Get1Bit())
            {
                GetScalingList8x8(&sps->ScalingLists8x8[1],(Ipp8u*) default_inter_scaling_list8x8,&sps->type_of_scaling_list_used[7]);
            }
            else
            {
                FillScalingList8x8(&sps->ScalingLists8x8[1],(Ipp8u*) default_inter_scaling_list8x8);
                sps->type_of_scaling_list_used[7] = SCLDEFAULT;
            }

        }
        else
        {
            Ipp32s i;

            for (i = 0; i < 6; i += 1)
            {
                FillFlatScalingList4x4(&sps->ScalingLists4x4[i]);
            }
            for (i = 0; i < 2; i += 1)
            {
                FillFlatScalingList8x8(&sps->ScalingLists8x8[i]);
            }

        }
    }
#else
	return UMC_UNSUPPORTED;
#endif
    else
    {
        sps->chroma_format_idc = 1;
        sps->bit_depth_luma = 8;
        sps->bit_depth_chroma = 8;
    }

    // log2 max frame num (bitstream contains value - 4)
    sps->log2_max_frame_num = (Ipp8u)GetVLCElement_unsigned() + 4;

    // pic order cnt type (0..2)
    sps->pic_order_cnt_type = (Ipp8u)GetVLCElement_unsigned();
    if (sps->pic_order_cnt_type > 2)
    {
        ps =  UMC_BAD_FORMAT;
    }

    if (sps->pic_order_cnt_type == 0)
    {
        // log2 max pic order count lsb (bitstream contains value - 4)
        sps->log2_max_pic_order_cnt_lsb = (Ipp8u)GetVLCElement_unsigned() + 4;
        sps->MaxPicOrderCntLsb = (1 << sps->log2_max_pic_order_cnt_lsb);
    }
    else if (sps->pic_order_cnt_type == 1)
    {
        sps->delta_pic_order_always_zero_flag = (Ipp8u)Get1Bit();
        sps->offset_for_non_ref_pic = GetVLCElement_signed();
        sps->offset_for_top_to_bottom_field = GetVLCElement_signed();
        sps->num_ref_frames_in_pic_order_cnt_cycle = GetVLCElement_unsigned();

        // alloc memory for stored frame offsets
        Ipp32s len = MAX(1, sps->num_ref_frames_in_pic_order_cnt_cycle*sizeof(Ipp32u));

        sps->poffset_for_ref_frame = (Ipp32s *)ippsMalloc_8u_x(len);

        if (sps->poffset_for_ref_frame == NULL)
            ps =  UMC_FAILED_TO_ALLOCATE_BUFFER;

        // get offsets
       if( sps->poffset_for_ref_frame != NULL )	//***add check  --bnie  8/19/2008
       for (Ipp32u i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
			sps->poffset_for_ref_frame[i] = GetVLCElement_signed();
        }
    }    // pic order count type 1

    // num ref frames
    sps->num_ref_frames = GetVLCElement_unsigned();
//    if (sps->num_ref_frames > 2)
//    {
//        return UMC_UNSUPPORTED;
//    }
    sps->gaps_in_frame_num_value_allowed_flag = (Ipp8u)Get1Bit();


    // picture width in MBs (bitstream contains value - 1)
    sps->frame_width_in_mbs = GetVLCElement_unsigned() + 1;

    // picture height in MBs (bitstream contains value - 1)
    sps->frame_height_in_mbs = GetVLCElement_unsigned() + 1;

    sps->frame_mbs_only_flag_kinoma_always_one = (Ipp8u)Get1Bit();
    if (sps->frame_mbs_only_flag_kinoma_always_one != 1)
    {
        return UMC_UNSUPPORTED;
        //***sps->mb_adaptive_frame_field_flag = (Ipp8u)Get1Bit();
    }
    sps->frame_height_in_mbs  = (2-sps->frame_mbs_only_flag_kinoma_always_one)*sps->frame_height_in_mbs;
    sps->direct_8x8_inference_flag = (Ipp8u)Get1Bit();
    if (sps->frame_mbs_only_flag_kinoma_always_one==0)
    {
        //VM_ASSERT(sps->direct_8x8_inference_flag);
        sps->direct_8x8_inference_flag = 1;
    }
    sps->frame_cropping_flag = (Ipp8u)Get1Bit();

    if (sps->frame_cropping_flag)
    {
        sps->frame_cropping_rect_left_offset      = GetVLCElement_unsigned();
        sps->frame_cropping_rect_right_offset     = GetVLCElement_unsigned();
        sps->frame_cropping_rect_top_offset       = GetVLCElement_unsigned();
        sps->frame_cropping_rect_bottom_offset    = GetVLCElement_unsigned();
    }
#ifndef DROP_VUI
    sps->vui_parameters_present_flag = (Ipp8u)Get1Bit();
    if (sps->vui_parameters_present_flag)
    {
        if (ps==UMC_OK) ps = GetVUIParam(sps);
    }
#else
    sps->vui_parameters_present_flag = 0;
#endif

    return ps;
}    // GetSequenceParamSet

Status H264Bitstream::GetPictureParamSetPart1(
    H264PicParamSet *pps
)
{
    // Not all members of the pic param set structure are contained in all
    // pic param sets. So start by init all to zero.
    memset(pps, 0, sizeof (H264PicParamSet));

    // skip NALU 8 bits
    GetBits(8);

    // id
    pps->pic_parameter_set_id = (Ipp16u)GetVLCElement_unsigned();
    if (pps->pic_parameter_set_id > MAX_NUM_PIC_PARAM_SETS-1)
    {
        return UMC_BAD_FORMAT;
    }

    // seq param set referred to by this pic param set
    pps->seq_parameter_set_id = (Ipp8u)GetVLCElement_unsigned();
    if (pps->seq_parameter_set_id > MAX_NUM_SEQ_PARAM_SETS-1)
    {
        return UMC_BAD_FORMAT;
    }

    return UMC_OK;
}    // GetPictureParamSetPart1

// Number of bits required to code slice group ID, index is num_slice_groups - 2
static const Ipp8u SGIdBits[7] = {1,2,2,3,3,3,3};

// ---------------------------------------------------------------------------
//  CH263pBs::GetPictureParamSet()
//    Read picture parameter set data from bitstream.
// ---------------------------------------------------------------------------
Status H264Bitstream::GetPictureParamSetPart2(
    H264PicParamSet  *pps,
    const H264SeqParamSet *sps
)
{
    pps->entropy_coding_mode = (Ipp8u)Get1Bit();

#ifdef DROP_CABAC
    if( pps->entropy_coding_mode == 1 )
		return UMC_UNSUPPORTED;
#endif

    pps->pic_order_present_flag = (Ipp8u)Get1Bit();

    // number of slice groups, bitstream has value - 1
    pps->num_slice_groups = GetVLCElement_unsigned() + 1;
    if (pps->num_slice_groups != 1)
#ifndef DROP_SLICE_GROUP
    {
        Ipp32u slice_group;
        Ipp32u PicSizeInMapUnits;    // for range checks

        PicSizeInMapUnits = sps->frame_width_in_mbs * sps->frame_height_in_mbs;
            // TBD: needs adjust for fields

        if (pps->num_slice_groups > MAX_NUM_SLICE_GROUPS)
        {
            return UMC_BAD_FORMAT;
        }

        pps->SliceGroupInfo.slice_group_map_type = (Ipp8u)GetVLCElement_unsigned();

        // Get additional, map type dependent slice group data
        switch (pps->SliceGroupInfo.slice_group_map_type)
        {
        case 0:
            for (slice_group=0; slice_group<pps->num_slice_groups; slice_group++)
            {
                // run length, bitstream has value - 1
                pps->SliceGroupInfo.run_length[slice_group] = GetVLCElement_unsigned() + 1;

                if (pps->SliceGroupInfo.run_length[slice_group] > PicSizeInMapUnits)
                {
                    return UMC_BAD_FORMAT;
                }
            }
            break;
        case 1:
            // no additional info
            break;
        case 2:
            for (slice_group=0; slice_group<(Ipp32u)(pps->num_slice_groups-1); slice_group++)
            {
                pps->SliceGroupInfo.t1.top_left[slice_group] = GetVLCElement_unsigned();
                pps->SliceGroupInfo.t1.bottom_right[slice_group] = GetVLCElement_unsigned();

                // check for legal values
                if (pps->SliceGroupInfo.t1.top_left[slice_group] >
                    pps->SliceGroupInfo.t1.bottom_right[slice_group])
                {
                    return UMC_BAD_FORMAT;
                }
                if (pps->SliceGroupInfo.t1.bottom_right[slice_group] >= PicSizeInMapUnits)
                {
                    return UMC_BAD_FORMAT;
                }
                if ((pps->SliceGroupInfo.t1.top_left[slice_group] %
                    sps->frame_width_in_mbs) >
                    (pps->SliceGroupInfo.t1.bottom_right[slice_group] %
                    sps->frame_width_in_mbs))
                {
                    return UMC_BAD_FORMAT;
                }
            }
            break;
        case 3:
        case 4:
        case 5:
            // For map types 3..5, number of slice groups must be 2
            if (pps->num_slice_groups != 2)
            {
                return UMC_BAD_FORMAT;
            }
            pps->SliceGroupInfo.t2.slice_group_change_direction_flag = (Ipp8u)Get1Bit();
            pps->SliceGroupInfo.t2.slice_group_change_rate = GetVLCElement_unsigned() + 1;
            if (pps->SliceGroupInfo.t2.slice_group_change_rate > PicSizeInMapUnits)
            {
                return UMC_BAD_FORMAT;
            }
            break;
        case 6:
            // mapping of slice group to map unit (macroblock if not fields) is
            // per map unit, read from bitstream
            {
                Ipp32u map_unit;
                Ipp32u num_bits;    // number of bits used to code each slice group id

                // number of map units, bitstream has value - 1
                pps->SliceGroupInfo.t3.pic_size_in_map_units = GetVLCElement_unsigned() + 1;
                if (pps->SliceGroupInfo.t3.pic_size_in_map_units != PicSizeInMapUnits)
                {
                    return UMC_BAD_FORMAT;
                }

                Ipp32s len = MAX(1, pps->SliceGroupInfo.t3.pic_size_in_map_units);

                pps->SliceGroupInfo.t3.pSliceGroupIDMap = ippsMalloc_8u_x(len);

                if (pps->SliceGroupInfo.t3.pSliceGroupIDMap == NULL)
                    return UMC_ALLOC;

                // num_bits is Ceil(log2(num_groups))
                num_bits = SGIdBits[pps->num_slice_groups - 2];

                for (map_unit = 0;
                     map_unit < pps->SliceGroupInfo.t3.pic_size_in_map_units;
                     map_unit++)
                {
                    pps->SliceGroupInfo.t3.pSliceGroupIDMap[map_unit] = (Ipp8u)GetBits(num_bits);
                    if (pps->SliceGroupInfo.t3.pSliceGroupIDMap[map_unit] >
                        pps->num_slice_groups - 1)
                    {
                        return UMC_BAD_FORMAT;
                    }
                }
            }
            break;
        default:
            return UMC_BAD_FORMAT;

        }    // switch
    }    // slice group info
#else
    return UMC_UNSUPPORTED;
#endif

    // number of list 0 ref pics used to decode picture, bitstream has value - 1
    pps->num_ref_idx_l0_active = GetVLCElement_unsigned() + 1;

    // number of list 1 ref pics used to decode picture, bitstream has value - 1
    pps->num_ref_idx_l1_active = GetVLCElement_unsigned() + 1;

    // weighted pediction
    pps->weighted_pred_flag = (Ipp8u)Get1Bit();
    pps->weighted_bipred_idc = (Ipp8u)GetBits(2);

    // default slice QP, bitstream has value - 26
    pps->pic_init_qp = (Ipp8u)(GetVLCElement_signed() + 26);
    if (pps->pic_init_qp > QP_MAX)
    {
        //return UMC_BAD_FORMAT;
    }

    // default SP/SI slice QP, bitstream has value - 26
    pps->pic_init_qs = (Ipp8u)(GetVLCElement_signed() + 26);
    if (pps->pic_init_qs > QP_MAX)
    {
        //return UMC_BAD_FORMAT;
    }

    pps->chroma_qp_index_offset[0] = (Ipp8s)GetVLCElement_signed();
    if ((pps->chroma_qp_index_offset[0] < -12) || (pps->chroma_qp_index_offset[0] > 12))
    {
        //return UMC_BAD_FORMAT;
    }

    pps->deblocking_filter_variables_present_flag = (Ipp8u)Get1Bit();
    pps->constrained_intra_pred_flag = (Ipp8u)Get1Bit();
    pps->redundant_pic_cnt_present_flag = (Ipp8u)Get1Bit();

#ifndef DROP_HIGH_PROFILE
	if(More_RBSP_Data()==UMC_OK)
    {
        pps->transform_8x8_mode_flag= (Ipp8u) Get1Bit();
        if(sps->seq_scaling_matrix_present_flag)
        {
            //fall-back set rule B
            if(Get1Bit())
            {
                // 0
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[0],(Ipp8u*)default_intra_scaling_list4x4,&pps->type_of_scaling_list_used[0]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[0],(Ipp8u*) sps->ScalingLists4x4[0].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[0] = SCLDEFAULT;
                }
                // 1
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[1],(Ipp8u*) default_intra_scaling_list4x4,&pps->type_of_scaling_list_used[1]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[1],(Ipp8u*) pps->ScalingLists4x4[0].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[1] = SCLDEFAULT;
                }
                // 2
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[2],(Ipp8u*) default_intra_scaling_list4x4,&pps->type_of_scaling_list_used[2]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[2],(Ipp8u*) pps->ScalingLists4x4[1].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[2] = SCLDEFAULT;
                }
                // 3
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[3],(Ipp8u*) default_inter_scaling_list4x4,&pps->type_of_scaling_list_used[3]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[3],(Ipp8u*) sps->ScalingLists4x4[3].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[3] = SCLDEFAULT;
                }
                // 4
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[4],(Ipp8u*) default_inter_scaling_list4x4,&pps->type_of_scaling_list_used[4]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[4],(Ipp8u*) pps->ScalingLists4x4[3].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[4] = SCLDEFAULT;
                }
                // 5
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[5],(Ipp8u*) default_inter_scaling_list4x4,&pps->type_of_scaling_list_used[5]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[5],(Ipp8u*) pps->ScalingLists4x4[4].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[5] = SCLDEFAULT;
                }



                // 0
                if(Get1Bit())
                {
                    GetScalingList8x8(&pps->ScalingLists8x8[0],(Ipp8u*)default_intra_scaling_list8x8,&pps->type_of_scaling_list_used[6]);
                }
                else
                {
                    FillScalingList8x8(&pps->ScalingLists8x8[0],(Ipp8u*) sps->ScalingLists8x8[0].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[6] = SCLDEFAULT;
                }
                // 1
                if(Get1Bit())
                {
                    GetScalingList8x8(&pps->ScalingLists8x8[1],(Ipp8u*) default_inter_scaling_list8x8,&pps->type_of_scaling_list_used[7]);
                }
                else
                {
                    FillScalingList8x8(&pps->ScalingLists8x8[1],(Ipp8u*) sps->ScalingLists8x8[1].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[7] = SCLDEFAULT;
                }

            }
            else
            {
                Ipp32s i;
                for(i=0; i<6; i++)
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[i],(Ipp8u *)sps->ScalingLists4x4[i].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
                }
                for(i=0; i<2; i++)
                {
                    FillScalingList8x8(&pps->ScalingLists8x8[i],(Ipp8u *)sps->ScalingLists8x8[i].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
                }
            }
        }
        else
        {
            //fall-back set rule A
            if(Get1Bit())
            {
                // 0
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[0],(Ipp8u*)default_intra_scaling_list4x4,&pps->type_of_scaling_list_used[0]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[0],(Ipp8u*) default_intra_scaling_list4x4);
                    pps->type_of_scaling_list_used[0] = SCLDEFAULT;
                }
                // 1
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[1],(Ipp8u*) default_intra_scaling_list4x4,&pps->type_of_scaling_list_used[1]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[1],(Ipp8u*) pps->ScalingLists4x4[0].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[1] = SCLDEFAULT;
                }
                // 2
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[2],(Ipp8u*) default_intra_scaling_list4x4,&pps->type_of_scaling_list_used[2]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[2],(Ipp8u*) pps->ScalingLists4x4[1].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[2] = SCLDEFAULT;
                }
                // 3
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[3],(Ipp8u*)default_inter_scaling_list4x4,&pps->type_of_scaling_list_used[3]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[3],(Ipp8u*) default_inter_scaling_list4x4);
                    pps->type_of_scaling_list_used[3] = SCLDEFAULT;
                }
                // 4
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[4],(Ipp8u*) default_inter_scaling_list4x4,&pps->type_of_scaling_list_used[4]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[4],(Ipp8u*) pps->ScalingLists4x4[3].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[4] = SCLDEFAULT;
                }
                // 5
                if(Get1Bit())
                {
                    GetScalingList4x4(&pps->ScalingLists4x4[5],(Ipp8u*) default_inter_scaling_list4x4,&pps->type_of_scaling_list_used[5]);
                }
                else
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[5],(Ipp8u*) pps->ScalingLists4x4[4].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[5] = SCLDEFAULT;
                }



                // 0
                if(Get1Bit())
                {
                    GetScalingList8x8(&pps->ScalingLists8x8[0],(Ipp8u*)default_intra_scaling_list8x8,&pps->type_of_scaling_list_used[6]);
                }
                else
                {
                    FillScalingList8x8(&pps->ScalingLists8x8[0],(Ipp8u*) default_intra_scaling_list8x8);
                    pps->type_of_scaling_list_used[6] = SCLDEFAULT;
                }
                // 1
                if(Get1Bit())
                {
                    GetScalingList8x8(&pps->ScalingLists8x8[1],(Ipp8u*) default_inter_scaling_list8x8,&pps->type_of_scaling_list_used[7]);
                }
                else
                {
                    FillScalingList8x8(&pps->ScalingLists8x8[1],(Ipp8u*) default_inter_scaling_list8x8);
                    pps->type_of_scaling_list_used[7] = SCLDEFAULT;
                }

            }
            else
            {
                Ipp32s i;
                for(i=0; i<6; i++)
                {
                    FillScalingList4x4(&pps->ScalingLists4x4[i],(Ipp8u *)sps->ScalingLists4x4[i].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
                }
                for(i=0; i<2; i++)
                {
                    FillScalingList8x8(&pps->ScalingLists8x8[i],(Ipp8u *)sps->ScalingLists8x8[i].ScalingListCoeffs);
                    pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
                }
            }

        }
        pps->chroma_qp_index_offset[1] = (Ipp8s)GetVLCElement_signed();
    }
    else
#endif
    {
        pps->chroma_qp_index_offset[1] = pps->chroma_qp_index_offset[0];
        Ipp32s i;
        for(i=0; i<6; i++)
        {
            FillScalingList4x4(&pps->ScalingLists4x4[i],(Ipp8u *)sps->ScalingLists4x4[i].ScalingListCoeffs);
            pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
        }
        for(i=0; i<2; i++)
        {
            FillScalingList8x8(&pps->ScalingLists8x8[i],(Ipp8u *)sps->ScalingLists8x8[i].ScalingListCoeffs);
            pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
        }
    }
    // calculate level scale matrices

    //start DC first
    //to do: reduce th anumber of matrices (in fact 1 is enough)
    Ipp32s i,j,k;/*
    for (i=0;i<6;i++)
        for (j=0;j<52;j++)
        {
            Ipp32u level_scale = pps->ScalingLists4x4[i].ScalingListCoeffs[0]*pre_norm_adjust4x4[j%6][0];
                pps->m_DCLevelScale[i].LevelScaleCoeffs[j] = level_scale;

        }*/
    // now process other 4x4 matrices
    for (i=0;i<6;i++)
        for (j=0;j<52;j++)
            for (k=0;k<16;k++)
            {

                Ipp32u level_scale = pps->ScalingLists4x4[i].ScalingListCoeffs[k]*pre_norm_adjust4x4[j%6][pre_norm_adjust_index4x4[k]];
                    pps->m_LevelScale4x4[i].LevelScaleCoeffs[j][k] = (Ipp16s) level_scale;

            }

#ifndef DROP_HIGH_PROFILE
    // process remaining 8x8  matrices
    for (i=0;i<2;i++)
        for (j=0;j<52;j++)
            for (k=0;k<64;k++)
            {

                Ipp32u level_scale = pps->ScalingLists8x8[i].ScalingListCoeffs[k]*pre_norm_adjust8x8[j%6][pre_norm_adjust_index8x8[k]];
                    pps->m_LevelScale8x8[i].LevelScaleCoeffs[j][k] = (Ipp16s) level_scale;

            }
#endif

    return UMC_OK;
}    // GetPictureParamSet
// ---------------------------------------------------------------------------
//  H264Bitstream::GetPictureDelimiter()
//    Read optional picture delimiter from bitstream.
// ---------------------------------------------------------------------------
Status H264Bitstream::GetPictureDelimiter(Ipp32u &PicCodType)
{
    PicCodType = GetBits(3);
    return UMC_OK;
}    // GetPictureDelimiter

// ---------------------------------------------------------------------------
//  H264Bitstream::ReadFillerData()
//    Filler data RBSP, read and discard all bytes == 0xff
// ---------------------------------------------------------------------------
Status H264Bitstream::ReadFillerData()
{
    while (SearchBits(8, 0xff, 0));
    return UMC_OK;
}    // SkipFillerData


// ---------------------------------------------------------------------------
//  H264Bitstream::GetSliceHeaderPart1()
//    Read H.264 first part of slice header
//
//  Reading the rest of the header requires info in the picture and sequence
//  parameter sets referred to by this slice header.
//
//    Do not print debug messages when IsSearch is true. In that case the function
//    is being used to find the next compressed frame, errors may occur and should
//    not be reported.
//
// ---------------------------------------------------------------------------
Status H264Bitstream::GetSliceHeaderPart1(H264SliceHeader *pSliceHeader)
{
    Ipp32u val;

    pSliceHeader->first_mb_in_slice = GetVLCElement_unsigned();
    if (0 > pSliceHeader->first_mb_in_slice)
        return UMC_BAD_FORMAT;

    // slice type
    val = GetVLCElement_unsigned();
    if (val > S_INTRASLICE)
    {
        if (val > S_INTRASLICE+S_INTRASLICE+1)
        {
            return UMC_BAD_FORMAT;
        }
        else
        {
            // Slice type is specifying type of not only this but all remaining
            // slices in the picture. Since slice type is always present, this bit
            // of info is not used in our implementation. Adjust (just shift range)
            // and return type without this extra info.
            val -= (S_INTRASLICE + 1);
        }
    }
    pSliceHeader->slice_type = (EnumSliceCodType)val;

    pSliceHeader->pic_parameter_set_id = (Ipp16u)GetVLCElement_unsigned();
    if (pSliceHeader->pic_parameter_set_id > MAX_NUM_PIC_PARAM_SETS-1)
    {
        return UMC_BAD_FORMAT;
    }

    return UMC_OK;

} // Status H264Bitstream::GetSliceHeaderPart1(H264SliceHeader *pSliceHeader)

// ---------------------------------------------------------------------------
//  H264Bitstream::GetSliceHeaderPart2()
//    Read H.264 second part of slice header
//
//    Do not print debug messages when IsSearch is true. In that case the function
//    is being used to find the next compressed frame, errors may occur and should
//    not be reported.
// ---------------------------------------------------------------------------

Status H264Bitstream::GetSliceHeaderPart2
(
    H264SliceHeader			*hdr,				// slice header read goes here
    PredWeightTable			*pPredWeight_L0,	// L0 weight table goes here
    PredWeightTable			*pPredWeight_L1,	// L1 weight table goes here
    RefPicListReorderInfo	*pReorderInfo_L0,
    RefPicListReorderInfo	*pReorderInfo_L1,
    AdaptiveMarkingInfo		*pAdaptiveMarkingInfo,
    const H264PicParamSet	*pps,
    bool					bIsIDRSlice,
    const H264SeqParamSet	*sps,
    Ipp8u					NALRef_idc,         // from slice header NAL unit
    bool					bIsSearch           // called during next picture search?
)
{
    Ipp8u ref_pic_list_reordering_flag_l0 = 0;
    Ipp8u ref_pic_list_reordering_flag_l1 = 0;

    hdr->frame_num = GetBits(sps->log2_max_frame_num);
    hdr->idr_flag = (Ipp8u) bIsIDRSlice;
    hdr->nal_ref_idc = NALRef_idc;

	//***bnie:
    hdr->bottom_field_flag_kinoma_always_zero = 0;
    if (sps->frame_mbs_only_flag_kinoma_always_one == 0)
#ifndef DROP_FIELD
    {
        hdr->field_pic_flag_unimplemented = (Ipp8u)Get1Bit();
		//***bnie
        hdr->MbaffFrameFlag_unimplemented = !hdr->field_pic_flag_unimplemented && sps->mb_adaptive_frame_field_flag_unimplemented;
        if (hdr->field_pic_flag_unimplemented != 0)
        {
            hdr->bottom_field_flag_unimplemented = (Ipp8u)Get1Bit();
            if (!bIsSearch)
            {
            }
            //return UMC_UNSUPPORTED;
        }
    }
#else
		return UMC_UNSUPPORTED;
#endif
    //correct frst_mb_in_slice in order to handle MBAFF
#ifndef DROP_MBAFF
    if (hdr->MbaffFrameFlag && hdr->first_mb_in_slice)//zero is allways zero ;)
    {
        Ipp32s width_in_mbs = sps->frame_width_in_mbs;
        Ipp32s mb_y = (hdr->first_mb_in_slice*2)/(2*width_in_mbs);
        Ipp32s mb_x = (hdr->first_mb_in_slice*2)%(2*width_in_mbs);
        Ipp32s bottom_mb = mb_x&1;
        mb_x>>=1;//real mb_x
        hdr->first_mb_in_slice = (mb_y*2+bottom_mb)*width_in_mbs+mb_x;
    }
#endif

    if (bIsIDRSlice)
        hdr->idr_pic_id = GetVLCElement_unsigned();

    if (sps->pic_order_cnt_type == 0)
    {
        hdr->pic_order_cnt_lsb = GetBits(sps->log2_max_pic_order_cnt_lsb);
        if (pps->pic_order_present_flag && (!hdr->field_pic_flag_kinoma_always_zero))
            hdr->delta_pic_order_cnt_bottom = GetVLCElement_signed();
    }

    if ((sps->pic_order_cnt_type == 1) && (sps->delta_pic_order_always_zero_flag == 0))
    {
        hdr->delta_pic_order_cnt[0] = GetVLCElement_signed();
        if (pps->pic_order_present_flag && (!hdr->field_pic_flag_kinoma_always_zero))
            hdr->delta_pic_order_cnt[1] = GetVLCElement_signed();
    }

    if (pps->redundant_pic_cnt_present_flag)
    {
        // redundant pic count
        hdr->redundant_pic_cnt = GetVLCElement_unsigned();
    }

    if (BPREDSLICE == hdr->slice_type)
    {
        // direct mode prediction method
        hdr->direct_spatial_mv_pred_flag = (Ipp8u)Get1Bit();
    }

    if (PREDSLICE == hdr->slice_type ||
        S_PREDSLICE == hdr->slice_type ||
        BPREDSLICE == hdr->slice_type)
    {
        hdr->num_ref_idx_active_override_flag = (Ipp8u)Get1Bit();
        if (hdr->num_ref_idx_active_override_flag != 0)
        // ref idx active l0 and l1
        {
            hdr->num_ref_idx_l0_active = GetVLCElement_unsigned() + 1;
            if (BPREDSLICE == hdr->slice_type)
                hdr->num_ref_idx_l1_active = GetVLCElement_unsigned() + 1;
        }
        else
        {
            // no overide, use num active from pic param set
            hdr->num_ref_idx_l0_active = pps->num_ref_idx_l0_active;
            if (BPREDSLICE == hdr->slice_type)
                hdr->num_ref_idx_l1_active = pps->num_ref_idx_l1_active;
            else
                hdr->num_ref_idx_l1_active = 0;
        }
    }    // ref idx override

    if (hdr->slice_type != INTRASLICE && hdr->slice_type != S_INTRASLICE)
    {
        Ipp32u reordering_of_pic_nums_idc;
        Ipp32u reorder_idx;

        // Reference picture list reordering
        ref_pic_list_reordering_flag_l0 = (Ipp8u)Get1Bit();
        if (ref_pic_list_reordering_flag_l0)
        {
            bool bOk = true;

            reorder_idx = 0;
            reordering_of_pic_nums_idc = 0;

            // Get reorder idc,pic_num pairs until idc==3
            while (bOk)
            {
                reordering_of_pic_nums_idc = (Ipp8u)GetVLCElement_unsigned();
                if (reordering_of_pic_nums_idc == 3)
                    break;

                pReorderInfo_L0->reordering_of_pic_nums_idc[reorder_idx] =
                                            (Ipp8u)reordering_of_pic_nums_idc;
                pReorderInfo_L0->reorder_value[reorder_idx]  =
                                                    GetVLCElement_unsigned();
                if (reordering_of_pic_nums_idc < 2)
                    // abs_diff_pic_num is coded minus 1
                    pReorderInfo_L0->reorder_value[reorder_idx]++;
                reorder_idx++;
                if (reorder_idx >= MAX_NUM_REF_FRAMES)
                {
                    return UMC_BAD_FORMAT;
                }
            }    // while
            pReorderInfo_L0->num_entries = reorder_idx;
        }    // L0 reordering info
        else
            pReorderInfo_L0->num_entries = 0;
        if (BPREDSLICE == hdr->slice_type)
        {
            ref_pic_list_reordering_flag_l1 = (Ipp8u)Get1Bit();
            if (ref_pic_list_reordering_flag_l1)
            {
                bool bOk = true;

                // Get reorder idc,pic_num pairs until idc==3
                reorder_idx = 0;
                reordering_of_pic_nums_idc = 0;
                while (bOk)
                {
                    reordering_of_pic_nums_idc = (Ipp8u)GetVLCElement_unsigned();
                    if (reordering_of_pic_nums_idc == 3)
                        break;

                    pReorderInfo_L1->reordering_of_pic_nums_idc[reorder_idx] =
                                                (Ipp8u)reordering_of_pic_nums_idc;
                    pReorderInfo_L1->reorder_value[reorder_idx]  =
                                                        GetVLCElement_unsigned();
                    if (reordering_of_pic_nums_idc < 2)
                        // abs_diff_pic_num is coded minus 1
                        pReorderInfo_L1->reorder_value[reorder_idx]++;
                    reorder_idx++;
                    if (reorder_idx >= MAX_NUM_REF_FRAMES)
                    {
                        if (!bIsSearch)
                        {
                        }
                        return UMC_BAD_FORMAT;
                    }
                }    // while
                pReorderInfo_L1->num_entries = reorder_idx;
            }    // L1 reordering info
            else
                pReorderInfo_L1->num_entries = 0;

        }    // B slice
    }    // reordering info

    // prediction weight table
    if ( (pps->weighted_pred_flag &&
          ((PREDSLICE == hdr->slice_type) || (S_PREDSLICE == hdr->slice_type))) ||
         ((pps->weighted_bipred_idc == 1) && (BPREDSLICE == hdr->slice_type)))
    {
        Ipp32u refindex;

        hdr->luma_log2_weight_denom = (Ipp8u)GetVLCElement_unsigned();
        hdr->chroma_log2_weight_denom = (Ipp8u)GetVLCElement_unsigned();
        for (refindex=0; refindex<hdr->num_ref_idx_l0_active; refindex++)
        {
            pPredWeight_L0[refindex].luma_weight_flag = (Ipp8u)Get1Bit();
            if (pPredWeight_L0[refindex].luma_weight_flag)
            {
                pPredWeight_L0[refindex].luma_weight = (Ipp8s)GetVLCElement_signed();
                pPredWeight_L0[refindex].luma_offset = (Ipp8s)GetVLCElement_signed();
            }
            else
            {
                pPredWeight_L0[refindex].luma_weight = 1<<hdr->luma_log2_weight_denom;
                pPredWeight_L0[refindex].luma_offset = 0;
            }
            pPredWeight_L0[refindex].chroma_weight_flag = (Ipp8u)Get1Bit();
            if (pPredWeight_L0[refindex].chroma_weight_flag)
            {
                pPredWeight_L0[refindex].chroma_weight[0] = (Ipp8s)GetVLCElement_signed();
                pPredWeight_L0[refindex].chroma_offset[0] = (Ipp8s)GetVLCElement_signed();
                pPredWeight_L0[refindex].chroma_weight[1] = (Ipp8s)GetVLCElement_signed();
                pPredWeight_L0[refindex].chroma_offset[1] = (Ipp8s)GetVLCElement_signed();
            }
            else
            {
                pPredWeight_L0[refindex].chroma_weight[0] = 1<<hdr->chroma_log2_weight_denom;
                pPredWeight_L0[refindex].chroma_weight[1] = 1<<hdr->chroma_log2_weight_denom;
                pPredWeight_L0[refindex].chroma_offset[0] = 0;
                pPredWeight_L0[refindex].chroma_offset[1] = 0;
            }
        }

        if (BPREDSLICE == hdr->slice_type)
        {
            for (refindex=0; refindex<hdr->num_ref_idx_l1_active; refindex++)
            {
                pPredWeight_L1[refindex].luma_weight_flag = (Ipp8u)Get1Bit();
                if (pPredWeight_L1[refindex].luma_weight_flag)
                {
                    pPredWeight_L1[refindex].luma_weight = (Ipp8s)GetVLCElement_signed();
                    pPredWeight_L1[refindex].luma_offset = (Ipp8s)GetVLCElement_signed();
                }
                else
                {
                    pPredWeight_L1[refindex].luma_weight = 1<<hdr->luma_log2_weight_denom;
                    pPredWeight_L1[refindex].luma_offset = 0;
                }
                pPredWeight_L1[refindex].chroma_weight_flag = (Ipp8u)Get1Bit();
                if (pPredWeight_L1[refindex].chroma_weight_flag)
                {
                    pPredWeight_L1[refindex].chroma_weight[0] = (Ipp8s)GetVLCElement_signed();
                    pPredWeight_L1[refindex].chroma_offset[0] = (Ipp8s)GetVLCElement_signed();
                    pPredWeight_L1[refindex].chroma_weight[1] = (Ipp8s)GetVLCElement_signed();
                    pPredWeight_L1[refindex].chroma_offset[1] = (Ipp8s)GetVLCElement_signed();
                }
                else
                {
                    pPredWeight_L1[refindex].chroma_weight[0] = 1<<hdr->chroma_log2_weight_denom;
                    pPredWeight_L1[refindex].chroma_weight[1] = 1<<hdr->chroma_log2_weight_denom;
                    pPredWeight_L1[refindex].chroma_offset[0] = 0;
                    pPredWeight_L1[refindex].chroma_offset[1] = 0;
                }
            }
        }    // B slice
    }    // prediction weight table
    else
    {
        hdr->luma_log2_weight_denom = 0;
        hdr->chroma_log2_weight_denom = 0;
    }

    // dec_ref_pic_marking
    pAdaptiveMarkingInfo->num_entries = 0;

    if (NALRef_idc)
    {
        if (bIsIDRSlice)
        {
            hdr->no_output_of_prior_pics_flag = (Ipp8u)Get1Bit();
            hdr->long_term_reference_flag = (Ipp8u)Get1Bit();
        }
        else
        {
            Ipp32u num_entries = 0;

            hdr->adaptive_ref_pic_marking_mode_flag = (Ipp8u)Get1Bit();
#ifndef DROP_ADAPTIVE_REF_MARKING
            Ipp32u memory_management_control_operation;
            while (hdr->adaptive_ref_pic_marking_mode_flag != 0)
            {
                memory_management_control_operation = (Ipp8u)GetVLCElement_unsigned();
                if (memory_management_control_operation == 0)
                    break;

                pAdaptiveMarkingInfo->mmco[num_entries] =
                    (Ipp8u)memory_management_control_operation;
                if (memory_management_control_operation != 5)
                     pAdaptiveMarkingInfo->value[num_entries*2] =
                        GetVLCElement_unsigned();
                // Only mmco 3 requires 2 values
                if (memory_management_control_operation == 3)
                     pAdaptiveMarkingInfo->value[num_entries*2+1] =
                        GetVLCElement_unsigned();
                num_entries++;
                if (num_entries >= MAX_NUM_REF_FRAMES)
                {
                    return UMC_BAD_FORMAT;
                }
            }    // while
#else
			if( hdr->adaptive_ref_pic_marking_mode_flag)
				return UMC_BAD_FORMAT;
#endif
            pAdaptiveMarkingInfo->num_entries = num_entries;
        }
    }    // def_ref_pic_marking

#ifndef DROP_CABAC
    if (pps->entropy_coding_mode == 1  &&    // CABAC
        (hdr->slice_type != INTRASLICE && hdr->slice_type != S_INTRASLICE))
        hdr->cabac_init_idc = (Ipp8u)GetVLCElement_unsigned();
    else
#else
	if( pps->entropy_coding_mode==1 )
		return UMC_UNSUPPORTED;
#endif
        hdr->cabac_init_idc = 0;

    hdr->slice_qp_delta = (Ipp8s)GetVLCElement_signed();

    if (S_PREDSLICE == hdr->slice_type ||
        S_INTRASLICE == hdr->slice_type)
    {
        if (S_PREDSLICE == hdr->slice_type)
            hdr->sp_for_switch_flag = (Ipp8u)Get1Bit();
        hdr->slice_qs_delta = (Ipp8s)GetVLCElement_signed();
    }

    if (pps->deblocking_filter_variables_present_flag != 0)
    {
        // deblock filter flag and offsets
        hdr->disable_deblocking_filter_idc = (Ipp8u)GetVLCElement_unsigned();
        if (hdr->disable_deblocking_filter_idc != 1)
        {
            hdr->slice_alpha_c0_offset = (Ipp8s)(GetVLCElement_signed()<<1);
            hdr->slice_beta_offset = (Ipp8s)(GetVLCElement_signed()<<1);
            if (hdr->slice_alpha_c0_offset < -12 || hdr->slice_alpha_c0_offset > 12)
            {
                return UMC_BAD_FORMAT;
            }
            if (hdr->slice_beta_offset < -12 || hdr->slice_beta_offset > 12)
            {
                return UMC_BAD_FORMAT;
            }
        }
        else
        {
            // set filter offsets to max values to disable filter
            hdr->slice_alpha_c0_offset = (Ipp8s)(0 - QP_MAX);
            hdr->slice_beta_offset = (Ipp8s)(0 - QP_MAX);
        }
    }

    //hdr->disable_deblocking_filter_idc = 1;
#ifndef DROP_SLICE_GROUP
    if ((pps->num_slice_groups > 1) &&
        (pps->SliceGroupInfo.slice_group_map_type >= 3) &&
        (pps->SliceGroupInfo.slice_group_map_type <= 5))
    {
        Ipp32u num_bits;    // number of bits used to code slice_group_change_cycle
        Ipp32u val;
        Ipp32u pic_size_in_map_units;
        Ipp32u max_slice_group_change_cycle=0;

        // num_bits is Ceil(log2(picsizeinmapunits/slicegroupchangerate + 1))
        pic_size_in_map_units = sps->frame_width_in_mbs * sps->frame_height_in_mbs;
            // TBD: change above to support fields

        max_slice_group_change_cycle = pic_size_in_map_units /
                        pps->SliceGroupInfo.t2.slice_group_change_rate;
        if (pic_size_in_map_units %
                        pps->SliceGroupInfo.t2.slice_group_change_rate)
            max_slice_group_change_cycle++;

        val = max_slice_group_change_cycle;// + 1;
        num_bits = 0;
        while (val)
        {
            num_bits++;
            val >>= 1;
        }
        hdr->slice_group_change_cycle = GetBits(num_bits);
        if (hdr->slice_group_change_cycle > max_slice_group_change_cycle)
        {
            //return UMC_BAD_FORMAT; don't see any reasons for that
        }
    }
#endif

    return UMC_OK;
} // CH263pbs::GetSliceHeaderPart2()


// ---------------------------------------------------------------------------
//        H264Bitstream::SearchBits()
//        Searches for a code with known number of bits.  Bitstream state,
//        pointer and bit offset, will be updated if code found.
//        nbits        : number of bits in the code
//        code        : code to search for
//        lookahead    : maximum number of bits to parse for the code
// ---------------------------------------------------------------------------

bool H264Bitstream::SearchBits(const Ipp32u    nbits,const Ipp32u code,const Ipp32u lookahead)
{
    Ipp32u    w;
    Ipp32u n = nbits;
    Ipp32u*    pbs;
    Ipp32s    offset;

    pbs        = m_pbs;
    offset    = m_bitOffset;

    ippiGetNBits(m_pbs, m_bitOffset, n, w)

    for (n = 0; w != code && n < lookahead; n ++)
    {
        w = ((w << 1) & GetBitsMask[nbits]) | Get1Bit();
    }

    if (w == code)
        return(true);
    else
    {
        m_pbs        = pbs;
        m_bitOffset = offset;
        return(false);
    }

} // H264Bitstream::SearchBits()

//const Ipp32u CtxIdxIncLuma[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
//const Ipp32u CtxIdxIncChromaDC420[4]={0,1,2,2};
//const Ipp32u CtxIdxIncChromaDC422[8]={0,0,1,1,2,2,2,2};
//const Ipp32u CtxIdxIncChromaDC444[16]={0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2};
#ifndef DROP_CABAC
void H264Bitstream::ResidualBlock4x4_CABAC(Ipp32s ctxBlockCat,
                                           const Ipp32u *ctxIdxBase,
                                           Ipp32s coef_ctr,
                                           const Ipp32s *single_scan,
                                           Ipp16s *pPosCoefbuf)
{
    // See subclause 7.3.5.3.2 of H.264 standard
    Ipp32s maxNumCoeffminus1;
    Ipp32u ctxIdxOffset, ctxIdxOffsetLast;
    Ipp32u numDecodAbsLevelEq1 = 0, numDecodAbsLevelGt1 = 0;
    Ipp16s coeffRuns[18];

    // See table 9-32 of H.264 standard
#ifdef __ICL
    __assume_aligned(pPosCoefbuf, 8);
#endif
    switch (ctxBlockCat)
    {
    case BLOCK_LUMA_DC_LEVELS:
    case BLOCK_LUMA_LEVELS:
        memset(pPosCoefbuf, 0, sizeof(Ipp16s) * 16);
        maxNumCoeffminus1 = 15;
        break;
    case BLOCK_LUMA_AC_LEVELS:
    case BLOCK_CHROMA_AC_LEVELS:
        memset(pPosCoefbuf, 0, sizeof(Ipp16s) * 16);
        maxNumCoeffminus1 = 14;
        break;
    default:
        return;
    }

    ctxIdxOffset = ctxIdxBase[SIGNIFICANT_COEFF_FLAG] +
                   ctxIdxBlockCatOffset[SIGNIFICANT_COEFF_FLAG][ctxBlockCat];
    ctxIdxOffsetLast = ctxIdxBase[LAST_SIGNIFICANT_COEFF_FLAG] +
                       ctxIdxBlockCatOffset[LAST_SIGNIFICANT_COEFF_FLAG][ctxBlockCat];

    Ipp32u ncoefs = 0;
    Ipp32s i = 0;
    for (; i < maxNumCoeffminus1; i += 1)
    {
        // get significant_coeff_flag
        if (DecodeSingleBin_CABAC(ctxIdxOffset + i))
        {
            // store position of non-zero coeff
            coeffRuns[ncoefs] = (Ipp16s) i;
            // Intel compiler should use memory form of increment
            ncoefs += 1;
            // get last_significant_coeff_flag
            if (DecodeSingleBin_CABAC(ctxIdxOffsetLast + i))
                break;
        }
    }

    if (i == maxNumCoeffminus1)
    {
        coeffRuns[ncoefs] = (Ipp16s) i;
        ncoefs += 1;
    }

    // calculate last coefficient in block
    ctxIdxOffset = ctxIdxBase[COEFF_ABS_LEVEL_MINUS1] +
                   ctxIdxBlockCatOffset[COEFF_ABS_LEVEL_MINUS1][ctxBlockCat];

    for (; ncoefs > 0; ncoefs -= 1)
    {
        Ipp32s level = DecodeSignedLevel_CABAC(ctxIdxOffset,
                                               numDecodAbsLevelEq1,
                                               numDecodAbsLevelGt1,
                                               9);

        // store coeff position and level to coeff buffer
        Ipp32u pos = coeffRuns[ncoefs - 1] + coef_ctr + 1;

        if(single_scan)
            pos = single_scan[pos];

        pPosCoefbuf[pos] = (Ipp16s) level;
    }

} // void H264Bitstream::ResidualBlock4x4_CABAC(Ipp32s ctxBlockCat,



void H264Bitstream::ResidualChromaDCBlockMH_CABAC(const Ipp32u *ctxIdxBase,
                                                  Ipp16s *pPosCoefbuf)
{
    // See subclause 7.3.5.3.2 of H.264 standard
    Ipp32s coef_ctr=-1;
    Ipp32s maxNumCoeffminus1;
    Ipp32u ctxIdxOffset, ctxIdxOffsetLast;
    Ipp32u numDecodAbsLevelEq1 = 0, numDecodAbsLevelGt1 = 0;
    Ipp16s coeffRuns[18];

    // See table 9-32 of H.264 standard
#ifdef __ICL
    __assume_aligned(pPosCoefbuf, 8);
#endif
    memset(pPosCoefbuf, 0, sizeof(Ipp16s) * 4);
    maxNumCoeffminus1 = 3;

    ctxIdxOffset = ctxIdxBase[SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[SIGNIFICANT_COEFF_FLAG][BLOCK_CHROMA_DC420_LEVELS];
    ctxIdxOffsetLast = ctxIdxBase[LAST_SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[LAST_SIGNIFICANT_COEFF_FLAG][BLOCK_CHROMA_DC420_LEVELS];

    Ipp32u ncoefs = 0;
    Ipp32s i = 0;
    for (; i < maxNumCoeffminus1; i++)
    {
        // get significant_coeff_flag
        if (DecodeSingleBin_CABAC(ctxIdxOffset+i))
        {
            // store position of non-zero coeff
            coeffRuns[ncoefs] = (Ipp16s) i;
            // Intel compiler should use memory form of increment
            ncoefs ++;
            // get last_significant_coeff_flag
            if (DecodeSingleBin_CABAC(ctxIdxOffsetLast+i)) break;
        }
    }

    if (i == maxNumCoeffminus1)
    {
        coeffRuns[ncoefs] = (Ipp16s) i;
        ncoefs ++;
    }

    // calculate last coefficient in block
    ctxIdxOffset = ctxIdxBase[COEFF_ABS_LEVEL_MINUS1] +
        ctxIdxBlockCatOffset[COEFF_ABS_LEVEL_MINUS1][BLOCK_CHROMA_DC420_LEVELS];

    for (; ncoefs > 0; ncoefs--)
    {
        Ipp32s level = DecodeSignedLevel_CABAC(ctxIdxOffset,
            numDecodAbsLevelEq1,
            numDecodAbsLevelGt1,8);

        // store coeff position and level to coeff buffer
        Ipp32u pos = coeffRuns[ncoefs - 1] + coef_ctr + 1;

        pPosCoefbuf[pos] = (Ipp16s) level;
    }

} // void H264Bitstream::ResidualChromaDCBlockMH_CABAC(const Ipp32u *ctxIdxBase,

void H264Bitstream::ResidualChromaDCBlockH2_CABAC(const Ipp32u *ctxIdxBase,
                                                  Ipp16s *pPosCoefbuf)
{
    // See subclause 7.3.5.3.2 of H.264 standard
    Ipp32s coef_ctr=-1;
    Ipp32s maxNumCoeffminus1;
    Ipp32u ctxIdxOffset, ctxIdxOffsetLast;
    Ipp32u numDecodAbsLevelEq1 = 0, numDecodAbsLevelGt1 = 0;
    Ipp16s coeffRuns[18];

    // See table 9-32 of H.264 standard
#ifdef __ICL
    __assume_aligned(pPosCoefbuf, 8);
#endif
    memset(pPosCoefbuf, 0, sizeof(Ipp16s) * 8);
    maxNumCoeffminus1 = 7;

    ctxIdxOffset = ctxIdxBase[SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[SIGNIFICANT_COEFF_FLAG][BLOCK_CHROMA_DC422_LEVELS];
    ctxIdxOffsetLast = ctxIdxBase[LAST_SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[LAST_SIGNIFICANT_COEFF_FLAG][BLOCK_CHROMA_DC422_LEVELS];

    Ipp32u ncoefs = 0;
    Ipp32s i = 0;
    for (; i < maxNumCoeffminus1; i++)
    {
        // get significant_coeff_flag
        if (DecodeSingleBin_CABAC(ctxIdxOffset+(i>>1)))
        {
            // store position of non-zero coeff
            coeffRuns[ncoefs] = (Ipp16s) i;
            // Intel compiler should use memory form of increment
            ncoefs ++;
            // get last_significant_coeff_flag
            if (DecodeSingleBin_CABAC(ctxIdxOffsetLast+(i>>1))) break;
        }
    }

    if (i == maxNumCoeffminus1)
    {
        coeffRuns[ncoefs] = (Ipp16s) i;
        ncoefs ++;
    }

    // calculate last coefficient in block
    ctxIdxOffset = ctxIdxBase[COEFF_ABS_LEVEL_MINUS1] +
        ctxIdxBlockCatOffset[COEFF_ABS_LEVEL_MINUS1][BLOCK_CHROMA_DC422_LEVELS];

    for (; ncoefs > 0; ncoefs--)
    {
        Ipp32s level = DecodeSignedLevel_CABAC(ctxIdxOffset,
            numDecodAbsLevelEq1,
            numDecodAbsLevelGt1,8);

        // store coeff position and level to coeff buffer
        Ipp32u pos = coeffRuns[ncoefs - 1] + coef_ctr + 1;

        pPosCoefbuf[pos] = (Ipp16s) level;
    }

} // void H264Bitstream::ResidualChromaDCBlockH2_CABAC(const Ipp32u *ctxIdxBase,

void H264Bitstream::ResidualChromaDCBlockH4_CABAC(const Ipp32u *ctxIdxBase,
                                                  const Ipp32s *single_scan,
                                                  Ipp16s *pPosCoefbuf)
{
    // See subclause 7.3.5.3.2 of H.264 standard
    Ipp32s coef_ctr=-1;
    Ipp32s maxNumCoeffminus1;
    Ipp32u ctxIdxOffset, ctxIdxOffsetLast;
    Ipp32u numDecodAbsLevelEq1 = 0, numDecodAbsLevelGt1 = 0;
    Ipp16s coeffRuns[18];

    // See table 9-32 of H.264 standard
#ifdef __ICL
    __assume_aligned(pPosCoefbuf, 8);
#endif
    memset(pPosCoefbuf, 0, sizeof(Ipp16s) * 16);
    maxNumCoeffminus1 = 15;

    ctxIdxOffset = ctxIdxBase[SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[SIGNIFICANT_COEFF_FLAG][BLOCK_CHROMA_DC444_LEVELS];
    ctxIdxOffsetLast = ctxIdxBase[LAST_SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[LAST_SIGNIFICANT_COEFF_FLAG][BLOCK_CHROMA_DC444_LEVELS];

    Ipp32u ncoefs = 0;
    Ipp32s i = 0;
    for (; i < maxNumCoeffminus1; i++)
    {
        // get significant_coeff_flag
        if (DecodeSingleBin_CABAC(ctxIdxOffset+(i>>2)))
        {
            // store position of non-zero coeff
            coeffRuns[ncoefs] = (Ipp16s) i;
            // Intel compiler should use memory form of increment
            ncoefs ++;
            // get last_significant_coeff_flag
            if (DecodeSingleBin_CABAC(ctxIdxOffsetLast+(i>>2))) break;
        }
    }

    if (i == maxNumCoeffminus1)
    {
        coeffRuns[ncoefs] = (Ipp16s) i;
        ncoefs ++;
    }

    // calculate last coefficient in block
    ctxIdxOffset = ctxIdxBase[COEFF_ABS_LEVEL_MINUS1] +
        ctxIdxBlockCatOffset[COEFF_ABS_LEVEL_MINUS1][BLOCK_CHROMA_DC444_LEVELS];

    for (; ncoefs > 0; ncoefs--)
    {
        Ipp32s level = DecodeSignedLevel_CABAC(ctxIdxOffset,
            numDecodAbsLevelEq1,
            numDecodAbsLevelGt1,8);

        // store coeff position and level to coeff buffer
        Ipp32u pos = coeffRuns[ncoefs - 1] + coef_ctr + 1;
        if(single_scan)
            pos = single_scan[pos];

        pPosCoefbuf[pos] = (Ipp16s) level;
    }

} // void H264Bitstream::ResidualChromaDCBlockH4_CABAC(const Ipp32u *ctxIdxBase,

void H264Bitstream::ResidualBlock8x8_CABAC(bool field_decoding_flag,
                                           const Ipp32s *single_scan,
                                           Ipp16s *pPosCoefbuf)
{
    // See subclause 7.3.5.3.2 of H.264 standard
    Ipp32u ctxIdxOffset, ctxIdxInc, ctxIdxOffsetLast;
    Ipp32u numDecodAbsLevelEq1 = 0, numDecodAbsLevelGt1 = 0;
    const Ipp32u *ctxIdxBase;
    const Ipp32s* pHPFF = hp_CtxIdxInc_sig_coeff_flag[field_decoding_flag];

    Ipp32s maxNumCoeffminus1 = 63;
    Ipp32u ncoefs = 0;
    Ipp32s i = 0;
    Ipp16s coeffRuns[65];

#ifdef __ICL
    __assume_aligned(pPosCoefbuf, 8);
#endif
    memset(pPosCoefbuf, 0, sizeof(Ipp16s) * 64);

    // See table 9-32 of H.264 standard
    if (field_decoding_flag)
        ctxIdxBase = ctxIdxOffset8x8FieldCoded;
    else
        ctxIdxBase = ctxIdxOffset8x8FrameCoded;

    ctxIdxOffset = ctxIdxBase[SIGNIFICANT_COEFF_FLAG] +
        ctxIdxBlockCatOffset[SIGNIFICANT_COEFF_FLAG][BLOCK_LUMA8X8_LEVELS];
    ctxIdxOffsetLast = ctxIdxBase[LAST_SIGNIFICANT_COEFF_FLAG] +
                    ctxIdxBlockCatOffset[LAST_SIGNIFICANT_COEFF_FLAG][BLOCK_LUMA8X8_LEVELS];

    for (; i < maxNumCoeffminus1; i++)
    {
        ctxIdxInc = pHPFF[i];
        // get significant_coeff_flag
        if (DecodeSingleBin_CABAC(ctxIdxOffset+ctxIdxInc))
        {
            // store position of non-zero coeff
            coeffRuns[ncoefs] = (Ipp16s) i;
            // Intel compiler should use memory form of increment
            ncoefs ++;
            ctxIdxInc = hp_CtxIdxInc_last_sig_coeff_flag[i];
            // get last_significant_coeff_flag
            if (DecodeSingleBin_CABAC(ctxIdxOffsetLast+ctxIdxInc)) break;
        }
    }

    if (i == maxNumCoeffminus1)
    {
        coeffRuns[ncoefs] = (Ipp16s) i;
        ncoefs ++;
    }

    // calculate last coefficient in block
    ctxIdxOffset = ctxIdxBase[COEFF_ABS_LEVEL_MINUS1] +
        ctxIdxBlockCatOffset[COEFF_ABS_LEVEL_MINUS1][BLOCK_LUMA8X8_LEVELS];

    for (; ncoefs > 0; ncoefs--)
    {
        Ipp32s level = DecodeSignedLevel_CABAC(ctxIdxOffset,
            numDecodAbsLevelEq1,
            numDecodAbsLevelGt1,9);

        // store coeff position and level to coeff buffer
        Ipp32u pos = coeffRuns[ncoefs - 1];
        pos = single_scan[pos];

        pPosCoefbuf[pos] = (Ipp16s) level;
    }


} // void H264Bitstream::ResidualBlock8x8_CABAC(bool field_decoding_flag,


inline
Ipp32u H264Bitstream::DecodeSingleBinOnes_CABAC(Ipp32u ctxIdx,
                                                Ipp32s &binIdx)
{
    // See subclause 9.3.3.2.1 of H.264 standard

    Ipp32u pStateIdx = context_array[ctxIdx].pStateIdx;
    Ipp32u valMPS = context_array[ctxIdx].valMPS;
    Ipp32u codIOffset = m_lcodIOffset;
    Ipp32u codIRange = m_lcodIRange;
    Ipp32u validBits = m_validCABACbits;
    Ipp32u codIRangeLPS;
    Ipp32u binVal;

#ifdef STORE_CABAC_BITS
    Ipp16s preStateIdx = context_array[ctxIdx].pStateIdx;
    bool prevalMPS = context_array[ctxIdx].valMPS;
#endif
    do
    {
        binIdx++;
        codIRangeLPS = (rangeTabLPS20[pStateIdx]
                        [((codIRange >> (6+CABAC_LMB)))]);
        codIRange -= codIRangeLPS;
        binVal = valMPS;

        pStateIdx = transIdxMPS[pStateIdx];

        if (codIOffset >= codIRange)
        {
            binVal      ^= 1;
            codIOffset -= codIRange;
            codIRange   = codIRangeLPS;

            valMPS ^= pStateIdx == 1 ? 1 :0;
            pStateIdx = transIdxLPS[pStateIdx-1];

        }

        // Renormalization process
        // See subclause 9.3.3.2.2 of H.264
        //if (codIRange < (0x100<<(CABAC_LMB)))
        {
            unsigned char numBits = NumBitsToGetTbl[codIRange>>CABAC_LMB];
            codIRange <<= numBits;
            codIOffset <<= numBits;
#if (CABAC_LMB > 0)
            validBits -= numBits;
            if (validBits < 8)
            {
                // add 8 more bits.
                Ipp32u bits = Get12Bits();//GetBits(CABAC_LMB-8);
                validBits += CABAC_LMB-8;
                bits <<= (CABAC_LMB-validBits);
                codIOffset |= bits;
            }
#else
            codIOffset |= GetBits(numBits);
#endif
        }

#ifdef STORE_CABAC_BITS
        sym_cnt++;
        if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
        if(cabac_bits)
#ifdef CABAC_DECORER_COMP
            fprintf(cabac_bits,"sb %d %d %d %d %d\n",ctxIdx,
            codIRange>>CABAC_LMB,
            codIOffset>>CABAC_LMB,
            binVal,sym_cnt);
#else
            fprintf(cabac_bits,"sb %d %d %d %d %d %d %d\n",ctxIdx,preStateIdx,prevalMPS,pStateIdx,valMPS,binVal,sym_cnt);
            preStateIdx = pStateIdx;
            prevalMPS = valMPS;
#endif
#endif
    }while (binVal && (binIdx < 14));

    context_array[ctxIdx].pStateIdx = pStateIdx;
    context_array[ctxIdx].valMPS = valMPS;
    m_lcodIOffset = codIOffset;
    m_lcodIRange = codIRange;
    m_validCABACbits = validBits;

    return binVal;

} // Ipp32u H264Bitstream::DecodeSingleBinOnes_CABAC(Ipp32u ctxIdx,

inline
Ipp32u H264Bitstream::DecodeBypassOnes_CABAC(void)
{
    // See subclause 9.3.3.2.3 of H.264 standard
    Ipp32u binVal;// = 0;
    Ipp32u binCount = 0;
    Ipp32u codIOffset = m_lcodIOffset;
    Ipp32u codIRange = m_lcodIRange;

    do
    {
#if (CABAC_LMB > 0)
        codIOffset = (codIOffset << 1);
        m_validCABACbits--;
        if (m_validCABACbits < 8)
        {
            // add 8 more bits.
            Ipp32u bits = Get12Bits();//GetBits(CABAC_LMB-8);
            m_validCABACbits += CABAC_LMB-8;
            bits <<= (CABAC_LMB-m_validCABACbits);
            codIOffset |= bits;
        }
#else
        codIOffset = (codIOffset << 1) | Get1Bit();
#endif

        Ipp32s mask = ((Ipp32s)(codIRange)-1-(Ipp32s)(codIOffset))>>31;
        // conditionally negate level
        binVal = mask&1;
        binCount += binVal;
        // conditionally subtract range from offset
        codIOffset -= codIRange & mask;

#ifdef STORE_CABAC_BITS
        sym_cnt++;
        if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
        if(cabac_bits)
#ifdef CABAC_DECORER_COMP
            fprintf(cabac_bits,"bp %d %d %d %d\n",
            codIRange>>CABAC_LMB,
            codIOffset>>CABAC_LMB,
            binVal,sym_cnt);
#else
        fprintf(cabac_bits,"bp %d %d\n",binVal,sym_cnt);
#endif
#endif
    } while(binVal);

    m_lcodIOffset = codIOffset;
    m_lcodIRange = codIRange;

    return binCount;

} // Ipp32u H264Bitstream::DecodeBypassOnes_CABAC(void)

inline
Ipp32s H264Bitstream::DecodeCoeffAbsLevelMinus1_CABAC(Ipp32u ctxIdxOffset,
                                                   Ipp32u&numDecodAbsLevelEq1,
                                                   Ipp32u&numDecodAbsLevelGt1)
{
    // See subclause 9.3.2.3 of H.264
    Ipp32u ctxIdxInc;
    Ipp32s binVal, binIdx;

    // PREFIX BIN(S) STRING DECODING
    // decoding first bin of prefix bin string
    binIdx = 1;
    ctxIdxInc = 0;
    if(numDecodAbsLevelGt1 == 0)
        ctxIdxInc = (min(4, 1 + numDecodAbsLevelEq1));

    if (0 == DecodeSingleBin_CABAC(ctxIdxOffset + ctxIdxInc))
    {
        numDecodAbsLevelEq1 ++;
    }
    else
    {

        // decoding next bin(s) of prefix bin string
        // we use Truncated Unary binarization with cMax = uCoff;
        //ctxIdxInc = 5 + min(4, numDecodAbsLevelGt1);
        ctxIdxInc = 5 + numDecodAbsLevelGt1;
        ctxIdxInc = ctxIdxInc >= 9 ? 9 : ctxIdxInc;

        do
        {
            binIdx++;
            binVal = DecodeSingleBin_CABAC(ctxIdxOffset + ctxIdxInc);

        } while (binVal && (binIdx < 14));

        // SUFFIX BIN(S) STRING DECODING

        // See subclause 9.1 of H.264 standard
        // we use Exp-Golomb code of 0-th order
        if (binVal)
        {
            Ipp32s leadingZeroBits;
            Ipp32s codeNum;

            // counting leading 1' before 0
            leadingZeroBits = DecodeBypassOnes_CABAC();

            // create codeNum
            codeNum = 1;
            while (leadingZeroBits--)
                codeNum = (codeNum << 1) | DecodeBypass_CABAC();

            // update syntax element
            binIdx += codeNum;

        }

        numDecodAbsLevelGt1 ++;
    }

    return binIdx;

} //Ipp32s H264Bitstream::DecodeCoeffAbsLevelMinus1_CABAC(Ipp32s ctxIdxOffset, Ipp32s &numDecodAbsLevelEq1, Ipp32s &numDecodAbsLevelGt1)


inline
Ipp32s H264Bitstream::DecodeSignedLevel_CABAC(Ipp32u ctxIdxOffset,
                                           Ipp32u&numDecodAbsLevelEq1,
                                           Ipp32u&numDecodAbsLevelGt1,
                                           Ipp32u max_value)
{
    // See subclause 9.3.2.3 of H.264
    Ipp32u ctxIdxInc;
    Ipp32s binVal, binIdx;

    // PREFIX BIN(S) STRING DECODING
    // decoding first bin of prefix bin string
    binIdx = 1;
    ctxIdxInc = 0;
    if(numDecodAbsLevelGt1 == 0)
        ctxIdxInc = (min(4, 1 + numDecodAbsLevelEq1));

    if (0 == DecodeSingleBin_CABAC(ctxIdxOffset + ctxIdxInc))
    {
        numDecodAbsLevelEq1 ++;
    }
    else
    {
        // decoding next bin(s) of prefix bin string
        // we use Truncated Unary binarization with cMax = uCoff;
        //ctxIdxInc = 5 + min(4, numDecodAbsLevelGt1);
        ctxIdxInc = 5 + numDecodAbsLevelGt1;
        ctxIdxInc = ctxIdxInc >= max_value? max_value : ctxIdxInc;

        //do
        //{
        //printf("%d %d ",
        //        m_lcodIRange>>CABAC_LMB,
        //        m_lcodIOffset>>CABAC_LMB
        //        );
        //binIdx++;
        //binVal = DecodeSingleBin_CABAC(ctxIdxOffset + ctxIdxInc);

        //printf("%d %d %d\n",
        //        m_lcodIRange>>CABAC_LMB,
        //        m_lcodIOffset>>CABAC_LMB,
        //        binVal
        //        );
        //} while (binVal && (binIdx < 14));

        binVal = DecodeSingleBinOnes_CABAC(ctxIdxOffset + ctxIdxInc, binIdx);
        // SUFFIX BIN(S) STRING DECODING

        // See subclause 9.1 of H.264 standard
        // we use Exp-Golomb code of 0-th order
        if (binVal)
        {
            Ipp32s leadingZeroBits;
            Ipp32s codeNum;

            // counting leading 1' before 0
            leadingZeroBits = DecodeBypassOnes_CABAC();

            // create codeNum
            codeNum = 1;
            while (leadingZeroBits--)
                codeNum = (codeNum << 1) | DecodeBypass_CABAC();

            // update syntax element
            binIdx += codeNum;

        }

        numDecodAbsLevelGt1 ++;

    }
    {
        // See subclause 9.3.3.2.3 of H.264 standard
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

        Ipp32s mask = ((Ipp32s)(m_lcodIRange)-1-(Ipp32s)(m_lcodIOffset))>>31;
        // conditionally negate level
        binIdx = (binIdx ^ mask) - mask;
        // conditionally subtract range from offset
        m_lcodIOffset -= m_lcodIRange & mask;

#ifdef STORE_CABAC_BITS
        sym_cnt++;
        if(cabac_bits==NULL) cabac_bits=fopen(__CABAC_FILE__,"w+t");
        if(cabac_bits)
#ifdef CABAC_DECORER_COMP
            fprintf(cabac_bits,"bp %d %d %d %d\n",
            m_lcodIRange>>CABAC_LMB,
            m_lcodIOffset>>CABAC_LMB,
            -mask,sym_cnt);
#else
        fprintf(cabac_bits,"bp %d %d\n",-mask,sym_cnt);
#endif
#endif
    }
    return binIdx;

} //Ipp32s H264Bitstream::DecodeSignedLevel_CABAC(Ipp32s ctxIdxOffset, Ipp32s &numDecodAbsLevelEq1, Ipp32s &numDecodAbsLevelGt1)

#endif

#ifndef DROP_VUI
Status H264Bitstream::GetVUIParam(H264SeqParamSet *sps)
{
    Status ps=UMC_OK;
    sps->aspect_ratio_info_present_flag = (Ipp8u) Get1Bit();
    if( sps->aspect_ratio_info_present_flag ) {
        sps->aspect_ratio_idc = (Ipp8u) GetBits(8);
        if( sps->aspect_ratio_idc  ==  255) {
            sps->sar_width = (Ipp16u) GetBits(16);
            sps->sar_height = (Ipp16u) GetBits(16);
        }
    }
    sps->overscan_info_present_flag = (Ipp8u) Get1Bit();
    if( sps->overscan_info_present_flag )
        sps->overscan_appropriate_flag = (Ipp8u) Get1Bit();
    sps->video_signal_type_present_flag = (Ipp8u) Get1Bit();
    if( sps->video_signal_type_present_flag ) {
        sps->video_format = (Ipp8u) GetBits(3);
        sps->video_full_range_flag = (Ipp8u) Get1Bit();
        sps->colour_description_present_flag = (Ipp8u) Get1Bit();
        if( sps->colour_description_present_flag ) {
            sps->colour_primaries = (Ipp8u) GetBits(8);
            sps->transfer_characteristics = (Ipp8u) GetBits(8);
            sps->matrix_coefficients = (Ipp8u) GetBits(8);
        }
    }
    sps->chroma_loc_info_present_flag = (Ipp8u) Get1Bit();
    if( sps->chroma_loc_info_present_flag ) {
        sps->chroma_sample_loc_type_top_field = (Ipp8u) GetVLCElement_unsigned();
        sps->chroma_sample_loc_type_bottom_field = (Ipp8u) GetVLCElement_unsigned();
    }
    sps->timing_info_present_flag = (Ipp8u) Get1Bit();
    if( sps->timing_info_present_flag ) {
        sps->num_units_in_tick = GetBits(32);
        sps->time_scale = GetBits(32);
        sps->fixed_frame_rate_flag = (Ipp8u) Get1Bit();
    }
    sps->nal_hrd_parameters_present_flag = (Ipp8u) Get1Bit();
    if( sps->nal_hrd_parameters_present_flag )
        ps=GetHRDParam(sps);
    sps->vcl_hrd_parameters_present_flag = (Ipp8u) Get1Bit();
    if( sps->vcl_hrd_parameters_present_flag )
        ps=GetHRDParam(sps);
    if( sps->nal_hrd_parameters_present_flag  ||  sps->vcl_hrd_parameters_present_flag )
        sps->low_delay_hrd_flag = (Ipp8u) Get1Bit();
    sps->pic_struct_present_flag  = (Ipp8u) Get1Bit();
    sps->bitstream_restriction_flag = (Ipp8u) Get1Bit();
    if( sps->bitstream_restriction_flag ) {
        sps->motion_vectors_over_pic_boundaries_flag = (Ipp8u) Get1Bit();
        sps->max_bytes_per_pic_denom = (Ipp8u) GetVLCElement_unsigned();
        sps->max_bits_per_mb_denom = (Ipp8u) GetVLCElement_unsigned();
        sps->log2_max_mv_length_horizontal = (Ipp8u) GetVLCElement_unsigned();
        sps->log2_max_mv_length_vertical = (Ipp8u) GetVLCElement_unsigned();
        sps->num_reorder_frames = (Ipp8u) GetVLCElement_unsigned();
        sps->max_dec_frame_buffering = (Ipp8u) GetVLCElement_unsigned();
    }
    return ps;
}
Status H264Bitstream::GetHRDParam(H264SeqParamSet *sps)
{
    Status ps=UMC_OK;
    sps->cpb_cnt = (Ipp8u) GetVLCElement_unsigned()+1;
    sps->bit_rate_scale = (Ipp8u) GetBits(4);
    sps->cpb_size_scale = (Ipp8u) GetBits(4);
    for( Ipp32s idx= 0; idx < sps->cpb_cnt; idx++ ) {
        sps->bit_rate_value[ idx ] = (Ipp32u) (GetVLCElement_unsigned()+1);
        sps->cpb_size_value[ idx ] = (Ipp8u) ((GetVLCElement_unsigned()+1)&0xff);
        sps->cbr_flag[ idx ] = (Ipp8u) Get1Bit();
    }
    sps->initial_cpb_removal_delay_length = (Ipp8u) GetBits(5)+1;
    sps->cpb_removal_delay_length = (Ipp8u) GetBits(5)+1;
    sps->dpb_output_delay_length = (Ipp8u) GetBits(5)+1;
    sps->time_offset_length = (Ipp8u) GetBits(5);
    return ps;

}

#endif
void H264Bitstream::SetDecodedBytes(size_t nBytes)
{
    m_pbs = m_pbsBase + (nBytes / 4);
    m_bitOffset = 31 - (nBytes % 4) * 8;

} // void H264Bitstream::SetDecodedBytes(size_t nBytes)
#define h264GetBits(current_data, offset, nbits, data) \
{ \
    Ipp32u x; \
    /*removeSCEBP(current_data, offset);*/ \
    offset -= (nbits); \
    if (offset >= 0) \
{ \
    x = current_data[0] >> (offset + 1); \
} \
    else \
{ \
    offset += 32; \
    x = current_data[1] >> (offset); \
    x >>= 1; \
    x += current_data[0] << (31 - offset); \
    current_data++; \
} \
    (data) = x & ((((Ipp32u) 0x01) << (nbits)) - 1); \
}

#define h264GetBits1(current_data, offset, data) h264GetBits(current_data, offset,  1, data);
#define h264GetBits8(current_data, offset, data) h264GetBits(current_data, offset,  8, data);
#define h264GetNBits(current_data, offset, nbits, data) h264GetBits(current_data, offset, nbits, data);

#define h264UngetNBits(current_data, offset, nbits) \
{ \
    offset += (nbits); \
    if (offset > 31) \
{ \
    offset -= 32; \
    current_data--; \
} \
}
static const Ipp8u vlc_inc[] = {0,3,6,12,24,48};

static Ipp32s bitsToGetTbl16s[7][16] = /*[level][numZeros]*/
{
    /*         0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15        */
    /*0*/    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 12, },
    /*1*/    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 12, },
    /*2*/    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 12, },
    /*3*/    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 12, },
    /*4*/    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 12, },
    /*5*/    {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 12, },
    /*6*/    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 12  }
};
static Ipp32s addOffsetTbl16s[7][16] = /*[level][numZeros]*/
{
    /*         0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15    */
    /*0*/    {1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  8,  16,},
    /*1*/    {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,},
    /*2*/    {1,  3,  5,  7,  9,  11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31,},
    /*3*/    {1,  5,  9,  13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61,},
    /*4*/    {1,  9,  17, 25, 33, 41, 49, 57, 65, 73, 81, 89, 97, 105,113,121,},
    /*5*/    {1,  17, 33, 49, 65, 81, 97, 113,129,145,161,177,193,209,225,241,},
    /*6*/    {1,  33, 65, 97, 129,161,193,225,257,289,321,353,385,417,449,481,}
};
static Ipp32s sadd[7]={15,0,0,0,0,0,0};

static void _GetBlockCoeffs_CAVLC(Ipp32u **pbs,
                                  Ipp32s *bitOffset,
                                  Ipp16s sNumCoeff,
                                  Ipp16s sNumTrOnes,
                                  Ipp16s *CoeffBuf)
{
    Ipp16u suffixLength = 0;        /* 0..6, to select coding method used for each coeff */
    Ipp16s uCoeffIndex;
    Ipp32u uCoeffLevel = 0;
    Ipp32u NumZeros;
    Ipp16u uBitsToGet;
    Ipp16u uFirstAdjust;
    Ipp16u uLevelOffset;
    Ipp32s w;

    if ((sNumCoeff > 10) && (sNumTrOnes < 3))
        suffixLength = 1;

    /* When NumTrOnes is less than 3, need to add 1 to level of first coeff */
    uFirstAdjust = (Ipp16u)((sNumTrOnes < 3) ? 1 : 0);

    /* read coeffs */
    for (uCoeffIndex = 0; uCoeffIndex<(sNumCoeff - sNumTrOnes); uCoeffIndex++)
    {
        /* update suffixLength */
        if ((uCoeffIndex == 1) && (uCoeffLevel > 3))
            suffixLength = 2;
        else if (suffixLength < 6)
        {
            if (uCoeffLevel > vlc_inc[suffixLength])
                suffixLength++;
        }

        /* Get the number of leading zeros to determine how many more */
        /* bits to read. */
        NumZeros = 0;
        h264GetBits1((*pbs), (*bitOffset), w);
        while (w == 0)
        {
            h264GetBits1((*pbs), (*bitOffset), w);
            NumZeros++;
        }

        if (15 >= NumZeros)
        {
            uBitsToGet     = (Ipp16u)(bitsToGetTbl16s[suffixLength][NumZeros]);
            uLevelOffset   = (Ipp16u)(addOffsetTbl16s[suffixLength][NumZeros]);

            if (uBitsToGet)
            {
                h264GetNBits((*pbs), (*bitOffset), uBitsToGet, NumZeros);
            }

            uCoeffLevel = (NumZeros>>1) + uLevelOffset + uFirstAdjust;

            CoeffBuf[uCoeffIndex] = (Ipp16s) ((NumZeros & 1) ? (-((signed) uCoeffLevel)) : (uCoeffLevel));
        }
        else
        {
            Ipp32u level_suffix;
            Ipp32u levelSuffixSize = NumZeros - 3;
            Ipp32s levelCode;

            h264GetNBits((*pbs), (*bitOffset), levelSuffixSize, level_suffix);
            levelCode = (Ipp16u) ((min(15, NumZeros) << suffixLength) + level_suffix) + uFirstAdjust*2 + sadd[suffixLength];
            levelCode = (Ipp16u) (levelCode + (1 << (NumZeros - 3)) - 4096);

            CoeffBuf[uCoeffIndex] = (Ipp16s) ((levelCode & 1) ?
                ((-levelCode - 1) >> 1) :
            ((levelCode + 2) >> 1));
        }

        uFirstAdjust = 0;

    } /* for uCoeffIndex*/

} /* static void _GetBlockCoeffs_CAVLC(Ipp32u **pbs, */

const Ipp32s ChromaDC422RasterScan[8]=
{
    0,2,
    1,4,
    6,3,
    5,7
};
IPPFUN (IppStatus,ippiDecodeCAVLCChromaDcCoeffs422_H264_1u16s,(Ipp32u **ppBitStream,
                                                                 Ipp32s *pOffset,
                                                                 Ipp16s *pNumCoeff,
                                                                 Ipp16s **ppPosCoefbuf,
                                                                 const Ipp32s *pTblCoeffToken,
                                                                 const Ipp32s **ppTblTotalZerosCR,
                                                                 const Ipp32s **ppTblRunBefore))
{
    Ipp16s        CoeffBuf[16];        /* Temp buffer to hold coeffs read from bitstream*/
    Ipp32u        uTR1Mask;
    Ipp32u        TrOneSigns;            /* return sign bits (1==neg) in low 3 bits*/
    Ipp32u        uCoeffIndex            = 0;
    Ipp32s        sTotalZeros            = 0;
    Ipp32s        sRunBefore;
    Ipp16s        sNumTrailingOnes;
    Ipp16s        sNumCoeff = 0;
    Ipp32s        pos;
    Ipp32s        i;

    /* check for the only codeword of all zeros*/


    /*ippiDecodeVLCPair_32s(ppBitStream, pOffset, pTblCoeffToken, */
    /*                              &sNumTrailingOnes,&sNumCoeff);*/
    register Ipp32s table_pos;
    register Ipp32s val;
    Ipp32u          table_bits;
    Ipp8u           code_len;
    Ipp32u*         tmp_pbs;
    Ipp32s          tmp_offs;

    /* check error(s) */

    tmp_pbs = *ppBitStream;
    tmp_offs = *pOffset;

    table_bits = *pTblCoeffToken;
    h264GetNBits((*ppBitStream), (*pOffset), table_bits, table_pos);
    val           = pTblCoeffToken[table_pos  + 1];
    code_len   = (Ipp8u) (val & 0xff);

    while (code_len & 0x80)
    {
        val        = val >> 8;
        table_bits = pTblCoeffToken[val];
        h264GetNBits((*ppBitStream), (*pOffset), table_bits, table_pos);
        val           = pTblCoeffToken[table_pos + val  + 1];
        code_len   = (Ipp8u) (val & 0xff);
    }

    h264UngetNBits((*ppBitStream), (*pOffset), code_len);

    if ((val>>8) == IPPVC_VLC_FORBIDDEN)
    {
        *ppBitStream = tmp_pbs;
        *pOffset    = tmp_offs;
        return ippStsH263VLCCodeErr;
    }
    sNumTrailingOnes  = (Ipp16s) ((val >> 8) &0xff);
    sNumCoeff = (Ipp16s) ((val >> 16) & 0xff);

    *pNumCoeff = sNumCoeff;

    if (sNumTrailingOnes)
    {
        h264GetNBits((*ppBitStream), (*pOffset), sNumTrailingOnes, TrOneSigns);
        uTR1Mask = 1 << (sNumTrailingOnes - 1);
        while (uTR1Mask)
        {
            CoeffBuf[uCoeffIndex++] = (Ipp16s) ((TrOneSigns & uTR1Mask) == 0 ? 1 : -1);
            uTR1Mask >>= 1;
        }

    }
    /* Get the sign bits of any trailing one coeffs */
    if (sNumCoeff)
    {
        /*memset((*ppPosCoefbuf), 0, 4*sizeof(Ipp16s)); */
#ifdef __ICL
#pragma vector always
#endif
        for (i = 0; i < 8; i++)
        {
            (*ppPosCoefbuf)[i] = 0;
        }
        /*((Ipp32s*)(*ppPosCoefbuf))[0] = 0; */
        /*((Ipp32s*)(*ppPosCoefbuf))[1] = 0; */
        /* get nonzero coeffs which are not Tr1 coeffs */
        if (sNumCoeff > sNumTrailingOnes)
        {
            _GetBlockCoeffs_CAVLC(ppBitStream, pOffset,sNumCoeff,
                sNumTrailingOnes, &CoeffBuf[uCoeffIndex]);

        }
        if (sNumCoeff < 8)
        {
            /*ippiVCHuffmanDecodeOne_1u32s(ppBitStream, pOffset,&sTotalZeros, */
            /*                                                ppTblTotalZerosCR[sNumCoeff]); */
            const Ipp32s *pDecTable = ppTblTotalZerosCR[sNumCoeff];


                table_bits = pDecTable[0];
            h264GetNBits((*ppBitStream), (*pOffset), table_bits, table_pos)

                val           = pDecTable[table_pos  + 1];
            code_len   = (Ipp8u) (val & 0xff);
            val        = val >> 8;


            while (code_len & 0x80)
            {
                table_bits = pDecTable[val];
                h264GetNBits((*ppBitStream), (*pOffset), table_bits, table_pos)

                    val           = pDecTable[table_pos + val + 1];
                code_len   = (Ipp8u) (val & 0xff);
                val        = val >> 8;
            }

            if (val == IPPVC_VLC_FORBIDDEN)
            {
                sTotalZeros =  val;
                return ippStsH263VLCCodeErr;
            }

            h264UngetNBits((*ppBitStream), (*pOffset),code_len)

                sTotalZeros =  val;

        }
        uCoeffIndex = 0;
        while (sNumCoeff)
        {
            if ((sNumCoeff > 1) && (sTotalZeros > 0))
            {
                /*ippiVCHuffmanDecodeOne_1u32s(ppBitStream, pOffset,&sRunBefore, */
                /*                                                ppTblRunBefore[sTotalZeros]); */
                const Ipp32s *pDecTable = ppTblRunBefore[sTotalZeros];


                    table_bits = pDecTable[0];
                h264GetNBits((*ppBitStream), (*pOffset), table_bits, table_pos)

                    val           = pDecTable[table_pos  + 1];
                code_len   = (Ipp8u) (val & 0xff);
                val        = val >> 8;


                while (code_len & 0x80)
                {
                    table_bits = pDecTable[val];
                    h264GetNBits((*ppBitStream), (*pOffset), table_bits, table_pos)

                        val           = pDecTable[table_pos + val + 1];
                    code_len   = (Ipp8u) (val & 0xff);
                    val        = val >> 8;
                }

                if (val == IPPVC_VLC_FORBIDDEN)
                {
                    sRunBefore =  val;
                    return ippStsH263VLCCodeErr;
                }

                h264UngetNBits((*ppBitStream), (*pOffset),code_len)

                    sRunBefore =  val;
            }
            else
                sRunBefore = sTotalZeros;

            pos             = sNumCoeff - 1 + sTotalZeros;
            sTotalZeros -= sRunBefore;

            /* The coeff is either in CoeffBuf or is a trailing one */
            (*ppPosCoefbuf)[ChromaDC422RasterScan[pos]] = CoeffBuf[uCoeffIndex++];

            sNumCoeff--;
        }
        (*ppPosCoefbuf) += 8;

    }

    return ippStsNoErr;

}


} // end namespace UMC
