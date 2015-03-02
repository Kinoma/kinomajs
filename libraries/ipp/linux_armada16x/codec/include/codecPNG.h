/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


#ifndef _CODECPNG_H_
#define _CODECPNG_H_

#include "codecDef.h"   /* General Codec external header file */


#ifdef __cplusplus
extern "C" {
#endif

/* Input by user */
typedef struct _IppPNGDecoderParam {
    IppColorFormat      nDesiredClrMd;
	IppiSize            nDesiredImageSize;
    Ipp8u               nAlphaValue;
} IppPNGDecoderParam;

/*warn: users are not allowed to modify below values*/
typedef enum {
    IPP_PNG_GREY                    = 0,
    IPP_PNG_TRUECOLOR               = 2,
    IPP_PNG_PLTE_RGB                = 3,
    IPP_PNG_GREY_A                  = 4,
    IPP_PNG_TRUECOLOR_A             = 6,
} IppPNGColorType;

/* Info from ancillary chunk */
typedef struct _IppPNGAncillaryInfo {
    int                         gama;
	Ipp8u                       *pAlpha;  //NULL if disble alpha information
    Ipp16u                      greyBkgd;
    Ipp16u                      RGBBkgd[3];
    Ipp8u                       plteBkgd;
    Ipp16u                      greyTrans;
    Ipp16u                      RGBTrans[3];
    Ipp8u                       sRGB;

    /* Availability of ancillary info */
    // Gama | Alpha | greybkgd | RGBbkgd | PLTEbkgd | greyTrans | RGBTrans | sRGB
    Ipp8u                       AncillaryInfoFlag;
} IppPNGAncillaryInfo;

/*************************************************************************************************************
                                  DecoderInitAlloc_PNG
   Description:
	   Init function for PNG decoding, it reads the header markers of PNG stream, 
	   builds the IppPNGDecoderState structure. It then allocates working buffer, 
	   inits all internal buffer pointers contained in decoder state for decoding.
   Input Arguments:
       pSrcBitStream   :  Pointer to a IppBitstream structure which, wich allocate a buffer for loading source file
	                      and define some stream attribut.
	   pDecoderParam   :  User input parameter, including output image size and colorformat.
	   pSrcCallbackTable: Pointer to the memory related call back functions table
   Output Arguments:
	   pDecoderState:	 Pointer to a initialized PNG decoder structure according to user's input.
	   pOutPic:  Pointer to a IppPicture containing alpha info, which know from decode IHDR info
   Returns:
		IPP_STATUS_NOERR				OK
		IPP_STATUS_BADARG_ERR			Bad arguments
		IPP_STATUS_NOTSUPPORTED_ERR		Not supported image format
		IPP_STATUS_NOMEM_ERR			Error because of memory allocate failure 
   Note:
       Currently only support BGRA8888,BGR888,BGR565,BGR555.
       If succeed, it will allocate memory for IppPNGDecoderState,
	   caller should responsible for release these memory!
	   or Failed, no memory will be allocated, caller have no need to worry about memory leak :)
*************************************************************************************************************/

IPPCODECAPI(IppCodecStatus, DecoderInitAlloc_PNG, (
			IppBitstream                *pSrcBitStream,			 
			IppPicture                  *pDstPicture,
			void                        **ppDecoderState,
			MiscGeneralCallbackTable    *pInCbTbl
))


/*******************************************************************************************************************
                                         Decode_PNG
Description:
	Decompress PNG stream into image user desired. 
	Accept half-baked stream.
Input Arguments:
	pSrcBitstream:	Pointer to a IppBitstream structure which defines the input bit-stream attributes.
	pDecoderState:	Pointer to input PNG decoder structure.
    pDecoderPar  :  Pointer to user input commands about output image attributtes.
Output Arguments:
	pSrcPicture:	Pointer to a IppPicture structure which defines the output image's attributes.
	pDecoderState:	Pointer to output PNG decoder structure.
	pAncillaryInfo: Pointer to the updated IppPNGAncillaryInfo structure.
Returns:
	IPP_STATUS_NOERR			OK, Stream finished
	IPP_STATUS_NEED_INPUT       Stream not finished, waiting for continuous stream input
	IPP_STATUS_BITSTREAM_ERR    Bit stream error
	IPP_STATUS_NOMEM_ERR
	IPP_STATUS_ERR				Error because of memory access failure or any unknown reasons
*********************************************************************************************************************/

IPPCODECAPI(IppCodecStatus, Decode_PNG, (
	IppBitstream                *pSrcBitstream,
    IppPNGDecoderParam          *pDecoderPar,
    IppPicture                  *pDstPicture,
    IppPNGAncillaryInfo         *pAncillaryInfo,
	void                        *pDecoderState
))


/*******************************************************************************************************************
                                      DecoderFree_PNG
Description:
	Clean up function of the PNG decoder. It releases the allocated working buffer and free the decoder state.
Input Arguments:
    ppSrcDecoderState	Pointer to the start address of PNG decoder state structure to free.
Output Arguments:
    None
Returns:
	IPP_STATUS_NOERR				OK
	IPP_STATUS_BADARG_ERR	        Bad arguments
*********************************************************************************************************************/

IPPCODECAPI(IppCodecStatus, DecoderFree_PNG, (
   void                         **ppDecoderState
))

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CODECPNG_H_ */

/* EOF */
