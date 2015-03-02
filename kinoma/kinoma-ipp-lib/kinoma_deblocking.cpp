/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#include "kinoma_ipp_lib.h"
#include "umc_h264_dec.h"
#include "umc_h264_dec_deblocking.h"

#include "ippcore.h"

using namespace UMC;

unsigned char mp4v_to_h264_qp_lut[52] =
{
	1,
	1, 1, 1, 26,27,	//1~5
	28,28,29,30,30,	//6~10
	31,31,32,32,33,	//11~15
	33,34,34,35,36,	//16~20
	37,38,39,40,42,	//21~25
	44,46,48,49,50,	//26~30
	51				//31
};


static void ResetDeblockingVariablesKinoma(Ipp32u MBAddr, Ipp8u *pY, int width, int height, Ipp32s pic_pitch, DeblockingParameters *pParams)
{
    Ipp32u offset;
    Ipp32s MBYAdjust = 0;
    Ipp32u mbXOffset, mbYOffset;
    //Ipp32s pic_pitch;// = m_pCurrentFrame->pitch();
   // Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u nCurrMB_X, nCurrMB_Y;
   // H264SliceHeader *pHeader;

    // load slice header
   // pHeader = (m_bFrameDeblocking) ?
   //           (m_pSliceStore->GetSliceHeader(m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id)) :
   //           (m_pSliceHeader);

    // load planes
    //pY = m_pCurrentFrame->m_pYPlane;
    //pU = m_pCurrentFrame->m_pUPlane;
    //pV = m_pCurrentFrame->m_pVPlane;

    // prepare macroblock variables
    int mb_width  = width/16;
	//int mb_height = height/16;

	nCurrMB_X = (MBAddr % mb_width);
    nCurrMB_Y = (MBAddr / mb_width)- MBYAdjust;
	
    mbXOffset = nCurrMB_X * 16;
    mbYOffset = nCurrMB_Y * 16;

    // calc plane's offsets
    offset = mbXOffset + (mbYOffset * pic_pitch);
    pY += offset;
    offset >>= 1;
    //pU += offset;
    //pV += offset;

    // set external edge variables
    pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING] = (nCurrMB_X != 0);
    pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING] = (nCurrMB_Y != 0);

    // reset external edges strength
    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING], 0);
    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING], 0);

    // set neighbour addreses
    pParams->nNeighbour[VERTICAL_DEBLOCKING] = MBAddr - 1;
    pParams->nNeighbour[HORIZONTAL_DEBLOCKING] = MBAddr - mb_width;

    // set deblocking flag(s)
    pParams->DeblockingFlag[VERTICAL_DEBLOCKING] = 0;
    pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING] = 0;

    // save variables
    pParams->pLuma = pY;
    //pParams->pChroma[0] = pU;
    //pParams->pChroma[1] = pV;


	//***to bring out of loop --bnie
	pParams->pitch = pic_pitch;
    pParams->nMaxMVector = 4;//2;//4??(FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec) ? (2) : (4);
   // pParams->MBFieldCoded = (FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec);

    // set slice's variables
    pParams->nAlphaC0Offset = 0;//***pHeader->slice_alpha_c0_offset;
    pParams->nBetaOffset    = 0;//***pHeader->slice_beta_offset;
	//***to bring out of loop --bnie


} // void H264SegmentDecoder::ResetDeblockingVariablesKinoma(DeblockingParameters *pParams)


static void PrepareDeblockingParametersISlice(DeblockingParameters *pParams)
{
    // set deblocking flag(s)
    pParams->DeblockingFlag[VERTICAL_DEBLOCKING] = 1;
    pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING] = 1;

    // calculate strengths
    if (pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING])
    {
        // deblocking with strong deblocking of external edge
        SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 0, 4);
    }

    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 4, 0);
    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 8, 3);
    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 12, 0);

    if (pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING])
    {
		// deblocking with strong deblocking of external edge
		SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 0, 4);
    }

    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 4, 0);
    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 8, 3);
    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 12, 0);
}

static void DeblockLuma(int fqp, DeblockingParameters *pParams)
{
    Ipp8u *pY = pParams->pLuma;
    Ipp32s pic_pitch = pParams->pitch;
    //Ipp32u MBAddr = pParams->nMBAddr;
    Ipp8u Clipping[16];
    Ipp8u Alpha[2];
    Ipp8u Beta[2];
    Ipp32s AlphaC0Offset = pParams->nAlphaC0Offset;
    Ipp32s BetaOffset = pParams->nBetaOffset;
    //Ipp32s pmq_QP = m_mbinfo.mbs[MBAddr].QP;
    
	Ipp32s pmq_QP = fqp;//m_mbinfo.mbs[MBAddr].QP;
	Ipp8u  Alpha_i,Beta_i, *pClipTab_i;
	int const_51 = 51;

    int    index = pmq_QP + BetaOffset;
		
	Clip1(const_51, index);
		
	Beta_i = BETA_TABLE[index];

    index = pmq_QP + AlphaC0Offset;
	Clip1(const_51, index);
    Alpha_i = ALPHA_TABLE[index];
    pClipTab_i = CLIP_TAB[index];

    //dir
    // luma deblocking
    //
    if (pParams->DeblockingFlag[0])
    {
        Ipp8u *pClipTab;
        Ipp32s QP;
        Ipp8u *pStrength = pParams->Strength[0];

        if (pParams->ExternalEdgeFlag[0])
        {
            //Ipp32s pmp_QP;

            // get neighbour block QP
            //pmp_QP = m_mbinfo.mbs[pParams->nNeighbour[dir]].QP;

            // luma variables
            QP = fqp;//(pmp_QP + pmq_QP + 1) >> 1 ;

            // external edge variables
            //***index = IClip(0, 51, QP + BetaOffset);
            index = QP + BetaOffset; Clip1(const_51, index);
            Beta[0] = BETA_TABLE[index];

            //***index = IClip(0, 51, QP + AlphaC0Offset);
            index = QP + AlphaC0Offset; Clip1(const_51, index);
			Alpha[0] = ALPHA_TABLE[index];
            pClipTab = CLIP_TAB[index];

            // create clipping values
            Clipping[0] = pClipTab[pStrength[0]];
            Clipping[1] = pClipTab[pStrength[1]];
            Clipping[2] = pClipTab[pStrength[2]];
            Clipping[3] = pClipTab[pStrength[3]];
        }

        // internal edge variables
        {
            Ipp32u edge;

			Beta[1] = Beta_i; //BETA_TABLE[index];
			Alpha[1] = Alpha_i;		//ALPHA_TABLE[index];
			pClipTab = pClipTab_i;	//CLIP_TAB[index];

            for (edge = 4;edge < 16;edge += 4)
            {
                if (*((Ipp32u *) (pStrength + edge)))
                {
                    // create clipping values
                    Clipping[edge + 0] = pClipTab[pStrength[edge + 0]];
                    Clipping[edge + 1] = pClipTab[pStrength[edge + 1]];
                    Clipping[edge + 2] = pClipTab[pStrength[edge + 2]];
                    Clipping[edge + 3] = pClipTab[pStrength[edge + 3]];
                }
            }
        }

        // perform deblocking
		ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_x(pY, pic_pitch, Alpha, Beta, Clipping, pStrength);
		//ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_simple(pY, pic_pitch, Alpha, Beta, Clipping, pStrength);
	}

    if (pParams->DeblockingFlag[1])
    {
        Ipp8u *pClipTab;
        Ipp32s QP;
        Ipp8u *pStrength = pParams->Strength[1];

        if (pParams->ExternalEdgeFlag[1])
        {
            //Ipp32s pmp_QP;

            // get neighbour block QP
           // pmp_QP = m_mbinfo.mbs[pParams->nNeighbour[1]].QP;

            // luma variables
            QP = fqp;//(pmp_QP + pmq_QP + 1) >> 1 ;

            // external edge variables
			//***index = IClip(0, 51, QP + BetaOffset);
			index = QP + BetaOffset; Clip1(const_51, index);
            Beta[0] = BETA_TABLE[index];

			//***index = IClip(0, 51, QP + AlphaC0Offset);
			index = QP + AlphaC0Offset; Clip1(const_51, index);
            Alpha[0] = ALPHA_TABLE[index];
            pClipTab = CLIP_TAB[index];

            // create clipping values
            Clipping[0] = pClipTab[pStrength[0]];
            Clipping[1] = pClipTab[pStrength[1]];
            Clipping[2] = pClipTab[pStrength[2]];
            Clipping[3] = pClipTab[pStrength[3]];

        }

        // internal edge variables
        {
            Ipp32u edge;

			Beta[1] = Beta_i; //BETA_TABLE[index];
			Alpha[1] = Alpha_i;		//ALPHA_TABLE[index];
			pClipTab = pClipTab_i;	//CLIP_TAB[index];

            for (edge = 4;edge < 16;edge += 4)
            {
                if (*((Ipp32u *) (pStrength + edge)))
                {
                    // create clipping values
                    Clipping[edge + 0] = pClipTab[pStrength[edge + 0]];
                    Clipping[edge + 1] = pClipTab[pStrength[edge + 1]];
                    Clipping[edge + 2] = pClipTab[pStrength[edge + 2]];
                    Clipping[edge + 3] = pClipTab[pStrength[edge + 3]];
                }
            }
        }

        // perform deblocking
 		ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_x(pY, pic_pitch, Alpha, Beta, Clipping, pStrength);
 		//ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_simple(pY, pic_pitch, Alpha, Beta, Clipping, pStrength);
   }


} // void H264SegmentDecoder::DeblockLuma(Ipp32u dir, DeblockingParameters *pParams)

/*
static void ResetDeblockingVariablesKinoma2(int width, DeblockingParameters *pParams)
{
    Ipp32s nCurrMB_X = pParams->nCurrMB_X, nCurrMB_Y = pParams->nCurrMB_Y;		// signed int
    int mb_width  = width/16;

	// prepare macroblock variables
    nCurrMB_X = nCurrMB_X +1;
	if(nCurrMB_X == mb_width)
	{
	    nCurrMB_Y ++;
		nCurrMB_X = 0;
		pParams->nCurrMB_Y = nCurrMB_Y;
	}

	pParams->nCurrMB_X = nCurrMB_X;
}
*/

void deblock_frame(int fqp, unsigned char *y, int width, int height, int y_rowbytes )
{
	int mb_w     = width/16;
	int mb_h     = height/16;
	int total_mb = mb_w * mb_h;
	int idx      = 0;
	
	if( fqp > 1 )
		for( idx = 0; idx < total_mb; idx++ )
		{
			DeblockingParameters pParams;
			
			ResetDeblockingVariablesKinoma(idx, y, width, height, y_rowbytes, &pParams);
			PrepareDeblockingParametersISlice(&pParams);
			DeblockLuma(fqp, &pParams);
			//ResetDeblockingVariablesKinoma2(width, &pParams);
		}
}

#if 0//test code
#include "kinoma_utilities.h"
#define TOTAL_FRAMES	1//900

int FskHardwareGetARMCPU()
{
	return FSK_ARCH_ARM_V6;
}


#ifdef _WIN32_WCE
#define INPUT_DIR_PATH		"\\Storage Card\\wce_test\\"
#else
#define INPUT_DIR_PATH		"C:\\test_avc\\"
#endif

#define INPUT_DEFAULT_FILE		"m.yuv"

#ifdef __cplusplus
extern "C" {
#endif
void halt_here(void);
#ifdef __cplusplus
}
#endif

void halt_here(void)
{
	int a = 1;
}

int main(const int argc, const char *argv[])
{
	char		src_yuv_path[256];
	char		dst_yuv_path_1[256];
	char		dst_yuv_path_2[256];
	FILE		*src_yuv		= NULL;
	FILE		*dst_yuv_1		= NULL;
	FILE		*dst_yuv_2		= NULL;
	long		src_yuv_size;

	unsigned char	*yuv_frame_bytes_1 = NULL;
	unsigned char	*yuv_frame_bytes_22 = NULL;
	unsigned char	*yuv_frame_bytes_2 = NULL;
	int				total_bytes = 0;
	int			total = 0;
	int			width = 352;
	int			height= 192;
	long		frame_size = width*height*3/2;

	strcpy( src_yuv_path, INPUT_DIR_PATH );
	strcat( src_yuv_path, INPUT_DEFAULT_FILE );
	src_yuv = fopen(src_yuv_path, "rb");
	if( src_yuv == NULL )
		return -1;

	fseek(src_yuv, 0, SEEK_END);
	src_yuv_size = ftell(src_yuv);
	fseek(src_yuv, 0, SEEK_SET);
	
	//dst_yuv
	strcpy( dst_yuv_path_1, src_yuv_path );
	strcat( dst_yuv_path_1, ".1.yuv" );
	strcpy( dst_yuv_path_2, src_yuv_path );
	strcat( dst_yuv_path_2, ".2.yuv" );
	//strcpy( ref_path, src_yuv_path );
	//strcat( ref_path, ".ref.yuv" );

	dst_yuv_1 = fopen(dst_yuv_path_1, "wb");
	dst_yuv_2 = fopen(dst_yuv_path_2, "wb");

	yuv_frame_bytes_1 = (unsigned char *)malloc(frame_size);
	yuv_frame_bytes_22 = (unsigned char *)malloc(frame_size+width*16);
	yuv_frame_bytes_2 = yuv_frame_bytes_22 + width*8;

	//kinoma_ipp_lib_mp4v_deblock_init(FSK_ARCH_ARM_V5);
	//kinoma_ipp_lib_avc_init(FSK_ARCH_ARM_V6);
	//kinoma_ipp_lib_avc_init(FSK_ARCH_C);

	while( total_bytes < src_yuv_size )		// totalSamples
	{	
		int qp = 20;
		fread( yuv_frame_bytes_1, 1, frame_size, src_yuv );
		if(1)
		{
			int i, j;

			for( j = 0; j < height; j++ )
			{
				int hh = j%16;
				for( i = 0; i < width; i++ )
				{
					int ww = i%16;

					yuv_frame_bytes_1[j*width+i] = j;//(((hh/4)*5)<<4)|((ww/4)*5);
				}
			}
		}
		memcpy( yuv_frame_bytes_2, yuv_frame_bytes_1, frame_size );
		total_bytes += frame_size;

#if 1
		fprintf( stderr, "ARM_V5 decoded #%d\n", total );
		//kinoma_ipp_lib_mp4v_deblock_init(FSK_ARCH_ARM_V5);
		kinoma_ipp_lib_avc_init(FSK_ARCH_C);
		
		deblock_frame(qp, yuv_frame_bytes_1, width, height, width );
		fwrite(yuv_frame_bytes_1, 1, frame_size, dst_yuv_1 );
		fflush(dst_yuv_1);
#endif

#if 1
		fprintf( stderr, "						ARM_V6 decoded #%d\n", total );
		//kinoma_ipp_lib_mp4v_deblock_init(FSK_ARCH_ARM_V5);
		kinoma_ipp_lib_avc_init(FSK_ARCH_ARM_V6);
		deblock_frame(qp, yuv_frame_bytes_2, width, height, width );
		fwrite(yuv_frame_bytes_2, 1, frame_size, dst_yuv_2 );
		fflush(dst_yuv_2);
#endif
		total++;
		if( total == TOTAL_FRAMES )
			break;
	}

	free( yuv_frame_bytes_1 );
	free( yuv_frame_bytes_2 );
	yuv_frame_bytes_1 = NULL;
	yuv_frame_bytes_2 = NULL;

	fclose(src_yuv);
	fclose(dst_yuv_1);
	fclose(dst_yuv_2);

	{
		int diff = 0;

		diff = diff_files( width, height, dst_yuv_path_1, dst_yuv_path_2 );

		if( diff )
		{
			fprintf( stderr, "\ndifferenc found!!!\n" );	
		}
		else
		{
			fprintf( stderr, "ok\n" );	
		}
	}
}
#endif
