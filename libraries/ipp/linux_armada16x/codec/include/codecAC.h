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


#ifndef _CODECAC_H_
#define _CODECAC_H_

/* Include Intel IPP external header file(s). */
#include "codecDef.h"	/* General Codec external header file*/

#ifdef __cplusplus
extern "C" {
#endif

/***** Data Types, Data Structures and Constants ******************************/

/***** Codec-specific Definitions *****/
/* Audio Codecs */
/************************
 AAC Bit Stream Constants
*************************/
#define	IPP_AACD_LAYNUM					8
#define IPP_AACD_MAX_CH_NUM			6
#define INPUT_BUF_SIZE_AAC			4096
#define OUTPUT_BUF_SIZE_AAC			4096

/************************
 MP3 Bit Stream Constants
*************************/
#define INPUT_BUF_SIZE_MP3				4096
#define OUTPUT_BUF_SIZE_MP3				4608
#define	IPP_MP3_HDR_BUF_SIZE			64   	/* Header buffer size */
#define	IPP_MP3_MAX_GRAN				2		/* Maximum number of granules per channel */
#define	IPP_MP3_MAX_CHAN				2		/* Maximum number of channels */
#define	IPP_MP3_SCF_BANDS				4		/* Number of scalefactor bands per channel */
#define	IPP_MP3_MAIN_DATA_BUF_SIZE		4096	/* Main data decoding buffer */
#define SAMPLE_RATES					3
#define BIT_RATES						15
#define _S_BITMASK(bits)				((1 << bits) - 1)

/* new part for send command */
typedef enum _IppACCommand{
	/* Command for MP3 decoder */
	IPPAC_MP3_GET_SAMPLERATE	 = 0,	/* Get sample rate */
	IPPAC_MP3_GET_BITRATE		 = 1,	/* Get bit rate  */
	IPPAC_MP3_GET_CHANNEL		 = 2,	/* Get channel number */
	IPPAC_MP3_GET_VERSION		 = 3, 	/* Get version */
	IPPAC_MP3_GET_LAYER			 = 4,   /* Get layer infor */
	IPPAC_MP3_GET_SERIES1		 = 5,	/* Get all information from 0~4 */
	IPPAC_MP3_FINDNEXT_SYNCWORD	 = 6,   /* find the next syncword */

	/* Command for AAC decoder */
	IPPAC_AAC_GET_SAMPLERATE	 = 100,	/* Get sample rate */
	IPPAC_AAC_GET_BITRATE		 = 101,	/* Get bit rate  */
	IPPAC_AAC_GET_CHANNEL		 = 102,	/* Get channel number */
	IPPAC_AAC_GET_VERSION		 = 103, /* Get version */
	IPPAC_AAC_FINDNEXT_SYNCWORD	 = 104, /* find the next syncword */
	IPPAC_AAC_SET_DOWNSAMPLE_MODE	 = 105,	/* Set down sample mode */
	IPPAC_AAC_SET_DOWNMIX_MODE	 = 106,	/* Set down mix mode */
	IPPAC_AAC_GET_CHANNEL_POSITION = 107,/* Get channel position info */
	/*Command for AAC encoder */
	IPPAC_AAC_SET_TNS_MODE		 = 200  /* Set tns mode, 1: enable TNS, 0: disable TNS */
} IppACCommand;
typedef enum _IppACChannelPos{
	IPP_FRONT_CENTER_CHANNEL	=	0,
	IPP_FRONT_LEFT_CHANNEL		=	1,
	IPP_FRONT_RIGHT_CHANNEL		=	2,
	IPP_REAR_SURROUND_CHANNEL	=	3,
	IPP_SIDE_LEFT_CHANNEL		=	4,
	IPP_SIDE_RIGHT_CHANNEL		=	5,
	IPP_FRONT_LFE_CHANNEL		=	6,
	IPP_BACK_LEFT_CHANNEL		=	7,
	IPP_BACK_RIGHT_CHANNEL		=	8
}IppACChannelPos;
typedef enum _IppACPCMFormat{
	IPP_PCM_16B_INTLVD		= 0,		/* 16 bit per PCM sample, interleaved */
	IPP_PCM_16B_NONINTLVD	= 1,		/* 16 bit per PCM sample, non-interleaved */
	IPP_PCM_32B_INTLVD		= 2,		/* 32 bit per PCM sample, interleaved */
	IPP_PCM_32B_NONINTLVD	= 3			/* 32 bit per PCM sample, non-interleaved */
}IppACPCMFormat;

typedef enum _IppAACDownmixMode{
	AAC_NO_DOWNMIX			= 0,		/* no downmix mode, all channels will be output */
	AAC_DOWNMIX_CHUNK		= 1,		/* for multi-channel stream, only two selected channels will be output */
	AAC_DOWNMIX_COMBINE		= 2			/* for multi-channel stream, two combination channels will be output */
}AACDownmixMode;

typedef enum {
	AAC_AOT_LC		= 2,
	AAC_AOT_LTP		= 4,
	AAC_AOT_HE		= 5,
	AAC_AOT_HE_PS	= 29,
}IppAACProfileType;

typedef enum {
	AAC_SF_MP2ADTS	= 0,
	AAC_SF_MP4ADTS	= 1,
	AAC_SF_ADIF		= 4,
	AAC_SF_RAW		= 6,
}IppAACStreamFmt;

typedef struct{
	int version;		/* 0:MPEG1, 1:MPEG2, 2:MPEG2.5 */
	int sampleRate;
	int bitRateIndex;
	int layer;
	int channelNum;
}IppMP3Info1;

typedef struct{
	IppAACProfileType profileType;			//2:LC, 4:LTP
    IppAACStreamFmt streamFmt;				//only when mpegFlag=4, audioObjectType=2, rawFormat=1 indicate MPEG4-LC raw data.
	int channelConfiguration;	//output channel configuration, 1:mono, 2:stereo, only support 1 and 2
	int samplingFrequencyIndex;	//0~11, defined in standard 14496-3
	IppACPCMFormat pcmFmt;								/* output PCM format, refer to IppACPCMFormat */
}IppAACDecoderConfig;

/* SBC Types */
typedef void IppSBCEncoderState;

typedef enum {
	MONO = 0,
	DUAL_MONO,
	STEREO,
	JOINT_STEREO
} IppSBCStreamChannelID;

typedef struct _IppSBCConfig {
	int quality;		/* 0 = low, 1 = middle, 2 = high */
	int subbands;		/* subbands can be 4 or 8 */
	int blocks;
	int bitpool;
	IppSBCStreamChannelID channelMode;	
	int allocation; /* 0 = loudness, 1 = SNR */
} IppSBCConfig;
													
/***** Multimedia Codec Functions *************************************************/
/* MP3 Decoder */
IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_MP3, (
	IppBitstream *pBitStream,
	MiscGeneralCallbackTable *pCallbackTable, 
	IppSound *pDstPcmAudio,
	void **pDecoderState
))

IPPCODECAPI( IppCodecStatus, Decode_MP3, (
    IppBitstream * pSrcBitstream,
    IppSound * pDstPcmAudio,
    void * pDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderFree_MP3, (
    void ** ppDecoderState
))

/* MPEG-4 AAC LC/LTP, MPEG-2 AAC LC */

IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_AAC, (
	IppBitstream *pBitStream,
	IppAACDecoderConfig *pDecoderConfig,
	MiscGeneralCallbackTable *pCallbackTable, 
	void **ppDecoderState
))


IPPCODECAPI( IppCodecStatus, Decode_AAC, (
	IppBitstream *pBitStream, 
	IppSound *pDstPcmAudio, 
	void *pDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderFree_AAC,(void **ppDecoderState))

/* Bluetooth SBC */

IPPCODECAPI( IppCodecStatus, EncoderInitAlloc_SBC, (IppSBCEncoderState **ppDs, 
			MiscGeneralCallbackTable* pCallBackTable, 
			IppSBCConfig *pEncoderConfig))

IPPCODECAPI( IppCodecStatus, Encode_SBC, (IppSound *pSrcSound, 
	IppBitstream *pDstBitstream, IppSBCEncoderState *pSBCEncoderState))

IPPCODECAPI( IppCodecStatus, EncoderFree_SBC, (IppSBCEncoderState **ppDs, 
			MiscGeneralCallbackTable* pCallBackTable))

/* New API to get stream informations */
IPPCODECAPI (IppCodecStatus, DecodeSendCmd_MP3Audio, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pDecoderState
))
IPPCODECAPI (IppCodecStatus, DecodeSendCmd_AACAudio, (
        int  cmd,          
        void *pInParam,       
        void *pOutParam,     
        void *pDecoderState 
))

/* AAC LC encoder */
#define AAC_FRAME_SIZE	1024

typedef struct
{
	int channelConfiguration;			/* Input channel configuration, 1:mono, 2:stereo, only support 1 and 2 */
	int samplingRate;					/* 8000~96000Hz, defined in standard 14496-3 */
	int bitRate;						/* bit rate in kbps */
	int bsFormat;			
}IppAACEncoderConfig;

IPPCODECEXAPI(IppCodecStatus, Encode_AAC, (IppSound *pSrcPcmAudio,
										IppBitstream *pDstBitStream,
										void *pEncoderState))

IPPCODECEXAPI(IppCodecStatus, EncoderInitAlloc_AAC, (IppAACEncoderConfig *pEncoderConfig,
												  MiscGeneralCallbackTable *pCallbackTable,
												  void **ppEncoderState))

IPPCODECEXAPI(IppCodecStatus, EncoderFree_AAC, (void **pEncoderState))


IPPCODECEXAPI(IppCodecStatus, EncodeSendCmd_AAC, (int cmd, 
												void *pInParam, 
												void *pOutParam, 
												void *pEncoderState))

#ifdef __cplusplus
}
#endif

#endif    /* #ifndef _CODECAC_H_ */

/* EOF */


