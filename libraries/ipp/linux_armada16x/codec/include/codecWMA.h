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


#ifndef _CODECWMA_H_
#define _CODECWMA_H_

#include "codecDef.h"	/* General Codec external header file*/


#ifdef __cplusplus
extern "C" {
#endif

#define WAVE_FORMAT_MSAUDIO1  0x0160
#define WAVE_FORMAT_WMAUDIO2  0x0161
#define WAVE_FORMAT_WMAUDIO3  0x0162

#define IPP_DECOPT_CHANNEL_DOWNMIXING	0x00000001
#define IPP_DECOPT_REQUANTTO16			0x00000040
#define IPP_DECOPT_DOWNSAMPLETO44OR48	0x00000080

#define IPP_WMA_MAX_CBSIZE 32
	
typedef struct {	
	/* Parameters which could get from stream */
	Ipp16u wFormatTag;			/* format type */
	Ipp16s nChannels;			/* number of channels (i.e. mono, stereo...) */
	Ipp32s nSamplesPerSec;		/* sample rate */
	Ipp32s nAvgBytesPerSec;		/* for buffer estimation */
	Ipp16s nBlockAlign;			/* block size of data */
	Ipp16s wBitsPerSample;		/* number of bits per sample of mono data */
	Ipp16s cbSize;				/* the count in bytes of the size of */
								/* extra information (after cbSize) */
	Ipp8u pData[IPP_WMA_MAX_CBSIZE];
								/* extra information data */
	Ipp16s iRmsAmplitudeRef;
	Ipp16s iRmsAmplitudeTarget;
	Ipp16s iPeakAmplitudeRef;
	Ipp16s iPeakAmplitudeTarget;

	/* Additional parameters set by user */
	int iDecoderFlags;			/* Flag for decoding, could be multiple set:
								   IPP_DECOPT_CHANNEL_DOWNMIXING, downmixing to two channels
								   IPP_DECOPT_REQUANTTO16, requant to 16 bit
								   IPP_DECOPT_DOWNSAMPLETO44OR48, downsample to 44.1 or 48 kHz
								*/

}IppWMADecoderConfig;

/***** Multimedia Codec Functions *********************************************/

/* WMA V8, V9 Std, V9 Pro, V10 Pro decoder */

IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_WMA, (
	IppBitstream *pBitStream,
	IppWMADecoderConfig *pDecoderConfig,
	MiscGeneralCallbackTable *pCallbackTable, 
	int *pMaxOutputBufLen, void **ppDecoderState
))


IPPCODECAPI( IppCodecStatus, Decode_WMA, (
	IppBitstream *pBitStream, 
	IppSound *pDstPcmAudio, 
	void *pDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderFree_WMA,(void **ppDecoderState))


#ifdef __cplusplus
}
#endif

#endif    /* #ifndef _CODECWMA_H_ */

/* EOF */
