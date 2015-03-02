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
/*
	RFC-3016
*/

#include "FskUtilities.h"
#include "FskEndian.h"
#include "RTPPacketParser.h"
#include "RTPPacketBuffer.h"
#include "QTReader.h"

#define SUPPORT_MAIN_PROFILE

// Private state
//...  0
#define kNALU_Type_NON_IDR  1
//...  2, 3, 4
#define kNALU_Type_IDR		5
#define kNALU_Type_SEI		6
#define kNALU_Type_SPS		7
#define kNALU_Type_PPS		8
#define kNALU_Type_Access	9
#define kNALU_Type_EOSeq	10
#define kNALU_Type_EOStr	11
#define kNALU_Type_Filler	12
//... 9 ~ 23
#define kNALU_Type_STAP_A   24
#define kNALU_Type_STAP_B   25
#define kNALU_Type_MTAP16   26
#define kNALU_Type_MTAP24   27
#define kNALU_Type_FU_A     28
#define kNALU_Type_FU_B     29
//... 30, 31

#define ReadLongB(  p )     ( p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3] )
#define ReadShortB( p )     ( p[0]<<8  | p[1] )
#define WriteLongB( v, p )  { p[0] = (UInt8)(((v)&0xff000000)>>24); p[1] = (UInt8)(((v)&0x00ff0000)>>16) ; p[2] = (UInt8)(((v)&0x0000ff00)>>8); p[3] = (UInt8)((v)&0x000000ff); }
#define WriteShortB( v, p ) { p[0] = ((v)&0xff00)>>8; p[1] = ((v)&0x00ff); }

#define OUTPUT_MULTIPLE_FRAMES 1

#define DEBUG_STRINGS					0

typedef struct
{
	unsigned char	sizeofNALULength;
	long			width;
	long			height;

	unsigned char	profile_id;
	unsigned char	profile_iop;
	unsigned char	constraint_set0_flag;
	unsigned char	constraint_set1_flag;
	unsigned char	constraint_set2_flag;
	unsigned char	level_idc;

	long			spsCount;
	unsigned char	*sps[32];
	short			spsSizes[32];

	long			ppsCount;
	unsigned char	*pps[32];
	short			ppsSizes[32];
} Overview;


typedef struct 
{
	Overview ov;
	UInt8 *esds;
	UInt32 esdsSize;


	UInt32 mediaFormat;
	UInt32 timeScale;

	UInt32 lastTimeStamp;
	//Boolean lastPacketIsVOG;
	//	UInt32 state;
	UInt8	*naluDataPtr;
	UInt32	naluDataSizeMax;
	long	cached_nalu_fu_Size;
	long	lastSequenceNumber;

	RTPCompressedMediaFrame frames;
} AVCPacketParser;

FskInstrumentedSimpleType(RTPPacketParserAVC, rtppacketparseravc);

void avcPacketParserInitialize(RTPPacketHandler handler);


static FskErr avcPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr avcPacketParserNew(RTPPacketParser parser);
static FskErr avcPacketParserDispose(RTPPacketParser parser);
static FskErr avcPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr avcPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr avcPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);
static FskErr avcPacketParserSetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 infoSize);
FskErr avcPacketParserFlush(RTPPacketParser parser);

//static Boolean HasMultipleVOP( long dataSize, UInt8 *dataPtr );

// @@ should live in FskUtilities.c
static void FskDecodeBase64(const char *inData, long *ioEncodedLength, char *outDecodedData, long *ioDecodedDataLength);

/* -----------------------------------------------------------------------*/

static void FindEnd( unsigned char *c, long len, long *count )
{
	long i;
	
	for( i = 0; i < len; i++ )
	{
		if( c[i] == ',' || c[i] == ' ' || c[i] == 0 )
		{
			*count = i;
			return;
		}
	}

	*count = len;
}


static void OverviewSetProfileLevel( const char *value, Overview *ov )
{
	ov->profile_id			 = (UInt8)FskStrHexToNum( value, 2 );
	ov->profile_iop			 = (UInt8)FskStrHexToNum( value+2, 2 );
	ov->constraint_set0_flag = (ov->profile_iop&0x80)>>7;
	ov->constraint_set1_flag = (ov->profile_iop&0x40)>>6;
	ov->constraint_set2_flag = (ov->profile_iop&0x20)>>5;
	ov->level_idc			 = (UInt8)FskStrHexToNum( value + 4, 2 );
}


static void OverviewDispose( Overview *ov )
{
	long i;
	
	for( i = 0; i < ov->spsCount; i++ )
	{
		if( ov->sps[i] != 0 )
		{
			FskMemPtrDispose( ov->sps[i] );
			ov->sps[i] = NULL;
		}
	}
	ov->spsCount = 0;

	for( i = 0; i < ov->ppsCount; i++ )
	{
		if( ov->pps[i] != 0 )
		{
			FskMemPtrDispose( ov->pps[i] );
			ov->pps[i] = NULL;
		}
	}
	ov->ppsCount = 0;
}


static long OverviewSetParamSets( unsigned char *value, Overview *ov )
{
	long			base64Len;
	long			psLen;
	unsigned char	*ps;
	unsigned char	*base64Ptr = value;
	long			err = 0;
	
	ov->spsCount = 0;
	ov->ppsCount = 0;
	while( 1 )
	{
		FindEnd( base64Ptr, FskStrLen( (const char*)base64Ptr ), &base64Len );
		if( base64Len > 0 )
		{
			err = FskMemPtrNewClear( base64Len, &ps ); //doesn't matter if it's bit bigger
			if (0 != err) 
				goto bail;	
			
			psLen = base64Len;
			FskDecodeBase64( (const char*)base64Ptr, &base64Len, (char*)ps, &psLen );
			
			if( ( ps[0]&0x1f) == 7 ) //sps
			{
				if (ov->spsCount >= 32) 
				{
					err = -1;
					goto bail;
				}
				
				ov->sps[ov->spsCount]		= ps;
				ov->spsSizes[ov->spsCount]	= (SInt16)psLen;
				ov->spsCount++;
			} 
			else if( ( ps[0]&0x1f) == 8 ) //pps
			{
				if (ov->ppsCount >= 32) 
				{
					err = -1;
					goto bail;
				}
				
				ov->pps[ov->ppsCount] = ps;
				ov->ppsSizes[ov->ppsCount] = (SInt16)psLen;
				ov->ppsCount++;
			} 
		}
		else
			break;

		base64Ptr += base64Len;

		if( base64Ptr[0] == ',')
			base64Ptr++;//skip divider
	}

bail:
	return err;
}



static long OverviewSetParamSetsByParameterSet( unsigned char *str, Overview *ov )
{
	char *value0 = NULL, *value = NULL;
	long i, j;
	long err = 0;
	
	ov->spsCount = 0;
	ov->ppsCount = 0;
	
	value0 = FskStrDoCopy((char*)str);
	value  = value0;
	if( value0 == NULL )
	{
		err = 0;
		goto bail;
	}
	ov->spsCount = (unsigned char)FskStrHexToNum(value, 2 ); value +=2;
	for( i = 0; i < ov->spsCount; i++ )
	{
		unsigned char *thisData = NULL;
		short thisSize = (short)FskStrHexToNum( value, 4 ); value += 4;
		
		err = FskMemPtrNewClear( thisSize, &thisData ); //doesn't matter if it's bit bigger
		if (0 != err) 
			goto bail;	
		
		for( j = 0; j < thisSize; j++ )
		{
			thisData[j] = (UInt8)FskStrHexToNum(value, 2 ); value += 2;
		}
		
		ov->spsSizes[i] = thisSize;
		ov->sps[i]		= thisData;
	}
	
	ov->ppsCount = (unsigned char)FskStrHexToNum(value, 2 ); value += 2;
	for( i = 0; i < ov->ppsCount; i++ )
	{
		unsigned char	*thisData = NULL;
		short			thisSize = (short)FskStrHexToNum( value, 4 ); value += 4;
		
		err = FskMemPtrNewClear( thisSize, &thisData ); //doesn't matter if it's bit bigger
		if (0 != err) 
			goto bail;	
		
		for( j = 0; j < thisSize; j++ )
		{
			thisData[j] = (UInt8)FskStrHexToNum(value, 2 ); value += 2;
		}
		
		ov->ppsSizes[i] = thisSize;
		ov->pps[i]		= thisData;
	}
	

	FskMemPtrDispose(value0);
	
bail:
	return err;
}


static long OverviewCreateImageDesc(  Overview *ov, unsigned char **imageDesc, long *imageDescSize )
{
	long			i, size, avcCSize;
	unsigned char	*p = NULL;
	QTImageDescriptionRecord *idp;
	unsigned char	*avcC;
	long			err = 0;
	#define FskEndianU32_NtoN(v) (v)
	#define FskEndianU16_NtoN(v) (v)
	#define WriteLongN( v, p )  { p[0] = (UInt8)(((v)&0x000000ff)); p[1] = (UInt8)(((v)&0x0000ff00)>>8) ; p[2] = (UInt8)(((v)&0x00ff0000)>>16); p[3] = (UInt8)(((v)&0xff000000)>>24); }
	
	avcCSize = 7;	//minimum
	for( i = 0; i < ov->spsCount; i++ )
		avcCSize += 2 + ov->spsSizes[i];

	for( i = 0; i < ov->ppsCount; i++ )
		avcCSize += 2 + ov->ppsSizes[i];
	
	size = sizeof(QTImageDescriptionRecord) + 8 + avcCSize;
	err = FskMemPtrNewClear( size, (FskMemPtr*)&p);
	if (0 != err) 
		goto bail;	
	
	idp  = (QTImageDescriptionRecord *)p;
	avcC = (unsigned char *)( idp + 1 );

	idp->idSize			= FskEndianU32_NtoN(size);
	idp->cType			= FskEndianU32_NtoN('avc1');
	idp->resvd1			= FskEndianU32_NtoN(0);							// must be zero
	idp->resvd2			= FskEndianU16_NtoN(0);							// must be zero
	idp->dataRefIndex	= FskEndianU16_NtoN(0);						// must be zero
	idp->version		= FskEndianU16_NtoN(1);							// version of codec data format
	idp->revisionLevel	= FskEndianU16_NtoN(1);						// revision of codec data format
	idp->vendor			= FskEndianU32_NtoN('appl');			// Apple
	idp->temporalQuality= FskEndianU32_NtoN(0);					// no temporal compression
	idp->spatialQuality = FskEndianU32_NtoN(512);	// we could be clever, but nobody would care
	idp->width			= FskEndianU16_NtoN((UInt16)ov->width);
	idp->height			= FskEndianU16_NtoN((UInt16)ov->height);
	idp->hRes			= FskEndianU32_NtoN(72<<16);							// dots-per-inch 
	idp->vRes			= FskEndianU32_NtoN(72<<16);							// dots-per-inch 
	idp->dataSize		= FskEndianU32_NtoN(0);							// zero if unknown, which it is
	idp->frameCount		= FskEndianU16_NtoN(1);						// one frame at a time
	idp->name[0]		= 5;
	idp->name[1]		= 'H';
	idp->name[2]		= '.';
	idp->name[3]		= '2';
	idp->name[4]		= '6';
	idp->name[5]		= '4';
	idp->depth			= FskEndianU16_NtoN(24);							// color.
	idp->clutID			= FskEndianU16_NtoN(-1);							// not using a clut
	
	WriteLongN( avcCSize + 8, avcC );
	//*(long *)(&avcC[0]) = FskEndianU32_NtoB(avcCSize + 8);	//size
	avcC += 4;
	
	WriteLongN( 'avcC', avcC );
	//*(long *)(&avcC[0]) = FskEndianU32_NtoB('avcC');  //4CC
	avcC += 4;

	avcC[0] = 1;   // version
	avcC++;
	
	avcC[0] = ov->profile_id;
	avcC++;
	
	avcC[0] = ov->profile_iop;
	avcC++;
	
	avcC[0] = ov->level_idc;
	avcC++;
	
	avcC[0] = 0xFC | ( ov->sizeofNALULength - 1 );
	avcC++;
	
	avcC[0] = 0xE0 | (UInt8)ov->spsCount;
	avcC++;

	for( i = 0; i < ov->spsCount; i++) 
	{
		short spsSize = ov->spsSizes[i];
		
		WriteShortB( spsSize, avcC );
		avcC += 2;
		//avcC[0] = (unsigned char)((spsSize&0xff00)>>16);
		//avcC++;

		//avcC[0] = (unsigned char)(spsSize&0x00ff);
		//avcC++;

		FskMemCopy(avcC, ov->sps[i], spsSize);
		avcC += spsSize;
	}
	
	avcC[0] = (UInt8)ov->ppsCount;
	avcC++;

	for( i = 0; i < ov->ppsCount; i++) 
	{
		short ppsSize = ov->ppsSizes[i];
		
		WriteShortB( ppsSize, avcC );
		avcC += 2;

		//avcC[0] = (unsigned char)((ppsSize&0xff00)>>16);
		//avcC++;

		//avcC[0] = (unsigned char)(ppsSize&0x00ff);
		//avcC++;

		FskMemCopy(avcC, ov->pps[i], ppsSize);
		avcC += ppsSize;
	}

bail:
	if( err != 0 )
	{
		*imageDesc		= NULL;
		*imageDescSize	= 0; 
	}
	else
	{
		*imageDesc		= (unsigned char*)idp;
		*imageDescSize	= size; 
	}

	return err;
}

void avcPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle	 = avcPacketParserCanHandle;
	handler->doNew			 = avcPacketParserNew;
	handler->doDispose		 = avcPacketParserDispose;
	handler->doProcessPacket = avcPacketParserProcessPacket;
	handler->doDisposePacket = avcPacketParserDisposePacket;
	handler->doGetInfo		 = avcPacketParserGetInfo;
	handler->doSetInfo		 = avcPacketParserSetInfo;
	handler->doFlush		 = avcPacketParserFlush;
}

/* -----------------------------------------------------------------------*/

FskErr avcPacketParserNew(RTPPacketParser parser)
{
	FskErr err = kFskErrNone;
	AVCPacketParser *avcPacketParserPtr = NULL;
	SDPMediaDescription mediaDescription;

	err = FskMemPtrNewClear(sizeof(AVCPacketParser), &avcPacketParserPtr);
	if (0 != err) 
		goto bail;
	
	parser->mediaFormat = kRTPVideoFormatAVC;
	parser->handlerRefCon = avcPacketParserPtr;

	avcPacketParserPtr->mediaFormat		= kRTPVideoFormatAVC;
	avcPacketParserPtr->timeScale		= 90000;
	avcPacketParserPtr->lastTimeStamp	= 0;
	avcPacketParserPtr->lastSequenceNumber = -1;

	avcPacketParserPtr->ov.spsCount = 0;
	avcPacketParserPtr->ov.ppsCount = 0;
	avcPacketParserPtr->ov.width	= 0;
	avcPacketParserPtr->ov.height	= 0;

	avcPacketParserPtr->naluDataPtr	= NULL;
	avcPacketParserPtr->naluDataSizeMax= 0;
	avcPacketParserPtr->cached_nalu_fu_Size  = 0;

	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) 
	{
		SDPAttribute attribute;

		//if frame size if there, give it a shot
		// strickly speaking, width and height should be from sprop-parameter-sets
		// which is very hard to get...  --bryan 10/14/2005
		attribute = SDPFindMediaAttribute(mediaDescription, "framesize");
		if (NULL != attribute) 
		{
			char *value, *parts[3];
			UInt16 nParts = 2;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ' ', &parts[0]);
			splitToken(parts[1], &nParts, '-', &parts[0]);
			if (nParts > 0)
			{
				avcPacketParserPtr->ov.width  = FskStrToNum(parts[0]);
				avcPacketParserPtr->ov.height = FskStrToNum(parts[1]);
			}

			FskMemPtrDispose(value);
		}
		
		// Next try the cliprect attribute
		if (0 == avcPacketParserPtr->ov.width) {
			attribute = SDPFindMediaAttribute(mediaDescription, "cliprect");
			if (NULL != attribute) {
				char *value, *parts[5];
				UInt16 nParts;
				value = FskStrDoCopy(attribute->value);
				splitToken(value, &nParts, ',', &parts[0]);
				if (4 == nParts) {
					avcPacketParserPtr->ov.width  = FskStrToNum(parts[3]);
					avcPacketParserPtr->ov.height  = FskStrToNum(parts[2]);
				}
				FskMemPtrDispose(value);
			}
		}
		
		// Next try the Width and Height attributes
		if (0 == avcPacketParserPtr->ov.width) {
			attribute = SDPFindMediaAttribute(mediaDescription, "Width");
			if (NULL != attribute) 
			{
				char *value, *parts[3];
				UInt16 nParts = 2;
				value = FskStrDoCopy(attribute->value);
				splitToken(value, &nParts, ';', &parts[0]);
				if (nParts > 0)
					avcPacketParserPtr->ov.width  = FskStrToNum(parts[1]);

				FskMemPtrDispose(value);
			}
		}
		
		if (0 == avcPacketParserPtr->ov.height) {
			attribute = SDPFindMediaAttribute(mediaDescription, "Height");
			if (NULL != attribute) 
			{
				char *value, *parts[3];
				UInt16 nParts = 2;
				value = FskStrDoCopy(attribute->value);
				splitToken(value, &nParts, ';', &parts[0]);
				if (nParts > 0)
					avcPacketParserPtr->ov.height  = FskStrToNum(parts[1]);

				FskMemPtrDispose(value);
			}
		}

		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) 
		{
			char *value;
			UInt8 *desc, *esds;
			UInt32 descSize;
	
			if (0 != copyAttributeValue(attribute->value, "profile-level-id", &value)) 
			{
				OverviewSetProfileLevel( (const char *)value, &avcPacketParserPtr->ov );
				FskMemPtrDispose(value);
			}

			if (0 != copyAttributeValue(attribute->value, "sprop-parameter-sets", &value)) 
			{
				err = OverviewSetParamSets( (unsigned char *)value, &avcPacketParserPtr->ov );
				if( err != 0 )
					goto bail;
			
				FskMemPtrDispose(value);
			}
			else if (0 != copyAttributeValue(attribute->value, "parameter-sets", &value)) 
			{
				err = OverviewSetParamSetsByParameterSet( (unsigned char *)value, &avcPacketParserPtr->ov );
				if( err != 0 )
					goto bail;
			
				FskMemPtrDispose(value);
			}

			err = OverviewCreateImageDesc( &avcPacketParserPtr->ov, &desc, (long*)&descSize );
			if (0 != err) 
				goto bail;

			esds = QTVideoSampleDescriptionGetExtension((QTImageDescription)desc, 'avcC');
			if (esds) {
#if TARGET_RT_BIG_ENDIAN
				avcPacketParserPtr->esdsSize = (esds[0] << 24) | (esds[1] << 16) | (esds[2] << 8) | (esds[3] << 0);
#else
				avcPacketParserPtr->esdsSize = (esds[3] << 24) | (esds[2] << 16) | (esds[1] << 8) | (esds[0] << 0);
#endif
				err = FskMemPtrNewFromData(avcPacketParserPtr->esdsSize - 8, esds + 8, (FskMemPtr*)&avcPacketParserPtr->esds);
				if (err) goto bail;
			}
			FskMemPtrDispose(desc);
		}

		// Grab what we need from the "rtpmap" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "rtpmap");
		if (NULL != attribute) 
		{
			char *value, *parts[3];
			UInt16 nParts = 2;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ' ', &parts[0]);
			if (0 == FskStrCompareCaseInsensitiveWithLength(parts[1], "H264", FskStrLen("H264"))) {
				nParts = 3;
				splitToken(parts[1], &nParts, '/', &parts[0]);
				if (nParts > 0)
					avcPacketParserPtr->timeScale = FskStrToNum(parts[1]);
			}
			FskMemPtrDispose(value);
		}
	}

bail:
	return err;
}


FskErr avcPacketParserDispose(RTPPacketParser parser)
{
	AVCPacketParser *avcPacketParserPtr = (AVCPacketParser *)parser->handlerRefCon;

	if (avcPacketParserPtr) 
	{
		RTPCompressedMediaFrame frames		= avcPacketParserPtr->frames;
		RTPCompressedMediaFrame framesNext	= NULL;
		
		while(frames)
		{			
			framesNext = frames->next;
			// Remove any stored data from our list
			if( frames != NULL )
				FskMemPtrDispose(frames);

			frames = framesNext;
		}
		
		OverviewDispose( &avcPacketParserPtr->ov );

		if( avcPacketParserPtr->naluDataPtr !=NULL )
			FskMemPtrDispose( avcPacketParserPtr->naluDataPtr );

		if( avcPacketParserPtr->esds !=NULL )
			FskMemPtrDispose(avcPacketParserPtr->esds);
		
		if( avcPacketParserPtr !=NULL )
			FskMemPtrDispose(avcPacketParserPtr);
	}

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr avcPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	Boolean validProfileID = false;
	FskErr err;

	if (0 != FskStrCompareCaseInsensitive("H264", encodingName)) 
		return kFskErrRTSPPacketParserUnsupportedFormat;

	if (NULL != mediaDescription) 
	{
		SDPAttribute attribute;
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) 
		{
			char *value;

			if (0 != copyAttributeValue(attribute->value, "profile-level-id", &value)) 
			{
				Overview		ov;
				
				OverviewSetProfileLevel( value, &ov );
#ifdef SUPPORT_MAIN_PROFILE
				if( ov.profile_id == 66 || ov.profile_id == 77 )
#else
				if( ov.profile_id == 66 )
#endif
					validProfileID = true;

				FskMemPtrDispose(value);
			}
		}
	}

	if (validProfileID)
		err = 0;
	else
		err = kFskErrRTSPPacketParserUnsupportedFormat;

	return err;
}

/* -----------------------------------------------------------------------*/

FskErr avcPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	AVCPacketParser *avcPacketParserPtr = (AVCPacketParser *)parser->handlerRefCon;
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorESDS:
			*(UInt8**)info = avcPacketParserPtr->esds;
			if (infoSize)
				*infoSize = avcPacketParserPtr->esdsSize;
			break;

		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = avcPacketParserPtr->mediaFormat;
			break;
			
		case kRTPPacketParserSelectorVideoWidth:
			*(UInt32*)info = avcPacketParserPtr->ov.width;
			break;

		case kRTPPacketParserSelectorVideoHeight:
			*(UInt32*)info = avcPacketParserPtr->ov.height;
			break;
		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorVideoTimeScale:
			*(UInt32*)info = avcPacketParserPtr->timeScale;
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr avcPacketParserSetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 infoSize)
{
	AVCPacketParser *avcPacketParserPtr = (AVCPacketParser *)parser->handlerRefCon;
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorVideoWidth:
			avcPacketParserPtr->ov.width = *(UInt32*)info;
			break;

		case kRTPPacketParserSelectorVideoHeight:
			avcPacketParserPtr->ov.height = *(UInt32*)info;
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

#ifdef LINKED_FRAME_UTILITIES_FOR_AVC
/* -----------------------------------------------------------------------*/
static int PushDataToLinkedFrames( int compositionTimeStamp, int decodeTimeStamp, long size, unsigned char *dataPtr, RTPCompressedMediaFrame *storedFrames, unsigned char **cachedHerePtr )
{
	RTPCompressedMediaFrame	frame = NULL;
	UInt32					err   = 0;

	err	= FskMemPtrNewClear( size + sizeof(RTPCompressedMediaFrameRecord), &frame);
	if (0 != err) 
		goto bail;
	
	frame->compositionTimeStamp = compositionTimeStamp; 
	frame->decodeTimeStamp		= decodeTimeStamp;
	frame->next					= NULL;
	frame->length				= size;
	FskMemMove( frame + 1, dataPtr, size );		
	
	if( cachedHerePtr != NULL )
		 *cachedHerePtr = (unsigned char *)(frame + 1);

	// Add the current buffer to our link list
	if( (*storedFrames) == NULL )
		*storedFrames = frame;
	else 
	{
		RTPCompressedMediaFrame  currentFrames = *storedFrames;

		while( currentFrames->next != NULL )
			currentFrames = currentFrames->next;

		currentFrames->next = frame;
	}

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "						append: %d", size);

bail:
	if( err != 0  && frame != NULL )
		FskMemPtrDispose(frame);

	return err;
}


static long GetLinkededFramesSize( RTPCompressedMediaFrame storedFrames, UInt32	*storedSizeOut )
{
	UInt32	storedSize		= 0;
	
	// if we have saved data, get it and prepend it to the current data
	while(storedFrames)
	{
		storedSize += storedFrames->length;
		storedFrames = storedFrames->next;
	}

	*storedSizeOut = storedSize;

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "						GetLinkededFramesSize: %d", storedSize);

	return 0;
}


static long MoveLinkedFramesDataToFrame( RTPCompressedMediaFrame storedFrames, RTPCompressedMediaFrame frame )
{
	long			copiedSize = 0;
	unsigned char	*pData = NULL;
	int				compositionTimeStamp;		// used by player
	int				decodeTimeStamp;
	
	frame->next	= NULL;
	
	// Walk our list again and really prepend the data
	pData = (unsigned char *)( frame + 1 ); // Move pass the header
	
	while(storedFrames)
	{
		RTPCompressedMediaFrame currStoredFrames = storedFrames;
		UInt32					currStoredSize	 = storedFrames->length;
		
		compositionTimeStamp = storedFrames->compositionTimeStamp;
		decodeTimeStamp		 = storedFrames->decodeTimeStamp;
		FskMemMove(pData, storedFrames+1, currStoredSize);
		storedFrames = storedFrames->next;
		
		// Remove our stored data from our list
		FskMemPtrDispose(currStoredFrames);
		currStoredFrames = NULL;
		
		// increment our frame ptr
		pData += currStoredSize;
		copiedSize += currStoredSize;
	}

	frame->compositionTimeStamp	= compositionTimeStamp;
	frame->decodeTimeStamp		= decodeTimeStamp;
	frame->length				= copiedSize;

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "						MoveLinkedFramesDataToFrame: %d", frame->length);

	return 0;
}

static long ClearLinkedFrames( RTPCompressedMediaFrame storedFrames )
{
	while(storedFrames)
	{
		RTPCompressedMediaFrame currStoredFrames = storedFrames;
		
			storedFrames = storedFrames->next;
		
		// Remove our stored data from our list
		FskMemPtrDispose(currStoredFrames);
		currStoredFrames = NULL;
	}

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "						ClearLinkedFrames");

	return 0;
}


#endif

#if 0
static long SanityCheckFrame( RTPCompressedMediaFrame f )
{
	unsigned char *p     = (unsigned char *)(f+1);
	long		   len   = f->length;
	long		   total = 0;
	
	while( 1 )
	{
		//long thisSize = FskEndianU32_BtoN(*(long *)(&p[0]));
		long thisSize = ReadLongB(p);
		
		if( thisSize <= 0 )
		{
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "out put frame is bad: size is wrong");
			return 1;	
		}

		if( total + thisSize + 4 <= len && thisSize > 0 )
		{
			p		+= thisSize + 4;
			total	+= thisSize + 4;

			if( total == len )
				break;
		}
		else
			break;		
	}

	if( total == len )
	{
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "out put frame is good: total = %d", total);
		return 0;
	}
	else
	{
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "out put frame is bad: total = %d", total);
		return 1;
	}
}
#endif

static long Size_NALU4_By_NALU2( long srcSize, unsigned char *src )
{
	long srcCount = 0;
	long dstSize  = 0;
	
	while( 1 )
	{
		//long thisSize = FskEndianU16_BtoN(*(short *)(&src[0]));
		long thisSize = ReadShortB(src);
		
		srcCount += thisSize + 2;
		src		 += 2;

		if( srcCount <= srcSize && thisSize > 0 )
		{
			dstSize += thisSize + 4;
			src		+= thisSize;
		}
		else
			break;		
	}

	return dstSize;
}


static long ResizeBuffer( long keep, long newSize, long *bufSize, unsigned char **bufWanted )
{
	unsigned char	*oldBuf = *bufWanted;
	unsigned char	*newBuf = NULL;
	long			err		= 0;
	
	if( *bufSize < newSize )
	{
		if( !keep && oldBuf != NULL )
			FskMemPtrDispose( oldBuf );

		err	= FskMemPtrNewClear( newSize, &newBuf );
		if (0 != err) 
			goto bail;

		if( keep && oldBuf != NULL )
		{
			FskMemMove(newBuf, oldBuf, *bufSize);
			FskMemPtrDispose( oldBuf );
		}

		*bufSize   = newSize;
		*bufWanted = newBuf;
	}

bail:
	return err;
}


static long NALU2_To_NALU4( long srcSize, unsigned char *src, long *dstSizeMax, long *dstSizeOut, unsigned char **dstOut )
{
	unsigned char *dst = NULL;
	long srcCount = 0;
	long err	  = 0;

	*dstSizeOut = Size_NALU4_By_NALU2( srcSize, src );
	if( *dstSizeOut == 0 )
	{
		err = -1;
		goto bail;
	}
	
	err = ResizeBuffer( 0, *dstSizeOut, dstSizeMax, dstOut );
	if( err != 0 )
		goto bail;
	
	dst = *dstOut;
	while( 1 )
	{
		//long thisSize = FskEndianU16_BtoN(*(short *)(&src[0]));
		long thisSize = ReadShortB(src);
		
		srcCount += thisSize + 2;
		src		 += 2;

		if( srcCount <= srcSize && thisSize > 0 )
		{
			WriteLongB( thisSize, dst );
			//(*(long *)(&dst[0])) = FskEndianU32_NtoB( thisSize );
			dst		 += 4;
			
			FskMemMove( dst, src, thisSize );
			dst	 += thisSize;
			src	 += thisSize;
		}
		else
			break;		
	}
	
bail:
	if( err != 0  && *dstOut !=NULL )
	{
		FskMemPtrDispose( *dstOut );
		*dstSizeMax	= 0;
		*dstSizeOut	= 0;
		*dstOut		= NULL;		
	}

	return err;
}


static long PushDataByOffset( long keep, long offset, long srcSize, unsigned char *src, long *dstSizeMax, long *dstSizeOut, unsigned char **dstOut )
{
	unsigned char	*dst = NULL;
	long			err	 = 0;
	
	err = ResizeBuffer( keep, srcSize + offset, dstSizeMax, dstOut );
	if( err != 0 )
		goto bail;
	
	dst = *dstOut;	
	dst += offset;
	FskMemMove( dst, src, srcSize );
	*dstSizeOut = srcSize + offset;

bail:
	if( err != 0  && *dstOut !=NULL )
	{
		FskMemPtrDispose( *dstOut );
		*dstSizeMax	= 0;
		*dstSizeOut	= 0;
		*dstOut		= NULL;		
	}

	return err;
}

FskErr avcPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
/*
test movie
rtsp://stats.kinoma.com/jarhead_avc_native.mp4
rtsp://testbox.pcslab.com/qt/deep_qta_H264_96k_12fps_AAC_48k_baseline.mp4
rtsp://testbox.pcslab.com/env/newword_env_H264_128k_10fps_AACp_32k.3gp
*/
	UInt32					storedSize				= 0;
	RTPCompressedMediaFrame	outFrame				= NULL;
	AVCPacketParser		*avcPacketParserPtr			= NULL;
	Boolean					frameChanged			= false;
	Boolean					packetSkipped			= false;
	//Boolean					isKeyFrame				= false;
	Boolean					isLastPacketOfAFrame	= false;
	long					naluDataSize			= 0;
	UInt32					err = 0;
	
	if(!parser || !rtpHeader) 
	{
		err = kFskErrRTSPBadPacket;
		goto bail;
	}

	avcPacketParserPtr = (AVCPacketParser *)parser->handlerRefCon;
	if( avcPacketParserPtr == NULL ) 
	{
		err = kFskErrBadData;
		goto bail;
	}
	
	{
		unsigned char naluHeader			= rtpHeader->data[0];
		unsigned char forbidden_zero_bit	= (naluHeader&0x80)>>7;
		unsigned char nal_ref_idc			= (naluHeader&0x60)>>5;
		unsigned char nal_unit_type			= (naluHeader&0x1f);
		
		if( forbidden_zero_bit != 0 || nal_unit_type == 0 )
		{
			err = kFskErrBadData;
			goto bail;
		}
				
		frameChanged		 = avcPacketParserPtr->lastTimeStamp != rtpHeader->timestamp;
		isLastPacketOfAFrame = rtpHeader->marker;
		avcPacketParserPtr->lastTimeStamp   = rtpHeader->timestamp;
		packetSkipped		 = UInt16Diff( rtpHeader->sequenceNumber, avcPacketParserPtr->lastSequenceNumber ) != 1 ;
		avcPacketParserPtr->lastSequenceNumber = rtpHeader->sequenceNumber;
		if( frameChanged )
			avcPacketParserPtr->cached_nalu_fu_Size = 0;

		switch( nal_unit_type )
		{
			case kNALU_Type_Access:
			case kNALU_Type_EOSeq:
			case kNALU_Type_EOStr:
			case kNALU_Type_Filler:
			//	break;
		
			case kNALU_Type_NON_IDR:
			case kNALU_Type_IDR:
			case kNALU_Type_SEI:
			case kNALU_Type_SPS:
			case kNALU_Type_PPS:
				//don't keep, offset 4
				PushDataByOffset( 0, 4, rtpHeader->dataSize, &rtpHeader->data[0], (long*)&avcPacketParserPtr->naluDataSizeMax, &naluDataSize, &avcPacketParserPtr->naluDataPtr );
				WriteLongB( rtpHeader->dataSize, avcPacketParserPtr->naluDataPtr );
				//(*(long *)avcPacketParserPtr->naluDataPtr) = FskEndianU32_NtoB( rtpHeader->dataSize );
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "  NALU");
				break;

			case kNALU_Type_STAP_A:
 				NALU2_To_NALU4( rtpHeader->dataSize - 1, &rtpHeader->data[1], (long*)&avcPacketParserPtr->naluDataSizeMax, (long*)&naluDataSize, &avcPacketParserPtr->naluDataPtr );
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "  STP_A");
				break;
			
			case kNALU_Type_FU_A:
			{
				unsigned char fuHeader = rtpHeader->data[1];
				unsigned char S		   = (fuHeader&0x80)>>7;
				unsigned char E		   = (fuHeader&0x40)>>6;
				//unsigned char R		   = (fuHeader&0x20)>>5;
				unsigned char nal_unit_type_fu	= (fuHeader&0x1f);
				
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "  FU_A, cached_nalu_fu_Size: %d", avcPacketParserPtr->cached_nalu_fu_Size);
				if( S == 1 )
				{
					unsigned char naluHdaderFu =   ((forbidden_zero_bit & 0x01)<<7) | 
													((nal_ref_idc       & 0x03)<<5) |
													(nal_unit_type_fu   & 0x1f);
					
					PushDataByOffset( 0, 5, rtpHeader->dataSize - 2, &rtpHeader->data[2], (long*)&avcPacketParserPtr->naluDataSizeMax, (long*)&avcPacketParserPtr->cached_nalu_fu_Size, &avcPacketParserPtr->naluDataPtr );
					avcPacketParserPtr->naluDataPtr[0] = 0xff;//mark it as pending...
					avcPacketParserPtr->naluDataPtr[1] = 0xff;//mark it as pending...
					avcPacketParserPtr->naluDataPtr[2] = 0xff;//mark it as pending...
					avcPacketParserPtr->naluDataPtr[3] = 0xff;//mark it as pending...
					avcPacketParserPtr->naluDataPtr[4] = naluHdaderFu;
					
					FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "    first, discarding any cached nalu data");
				}
				else
				{
					if( !packetSkipped )
					{
						if( avcPacketParserPtr->cached_nalu_fu_Size != 0 )
						{
							PushDataByOffset( 1, avcPacketParserPtr->cached_nalu_fu_Size, rtpHeader->dataSize-2, &rtpHeader->data[2], (long*)&avcPacketParserPtr->naluDataSizeMax, (long*)&avcPacketParserPtr->cached_nalu_fu_Size, &avcPacketParserPtr->naluDataPtr );
							FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "    not first, appending FU");
						}
						else
						{
							avcPacketParserPtr->cached_nalu_fu_Size = 0;
							FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "    !!!: not first without start, this fragment will be dropped!!!");
						}
					}
					else
					{
						avcPacketParserPtr->cached_nalu_fu_Size = 0;
						FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "    !!!: packet skipped in FU_A, this fragment will be dropped!!!");
					}
				}

				if( E == 1 && avcPacketParserPtr->cached_nalu_fu_Size > 0)
				{
					FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "    last, ready for output if data is complete");
					naluDataSize = avcPacketParserPtr->cached_nalu_fu_Size;
					WriteLongB( naluDataSize - 4, avcPacketParserPtr->naluDataPtr )
				}
				
				break;
			}

			default:
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "  !!!not supported nalu type!!!");
				break;
		}

		if( nal_unit_type != kNALU_Type_FU_A )
			avcPacketParserPtr->cached_nalu_fu_Size = 0;
	}

#if DEBUG_STRINGS
	{
		static long frameIndx = 0;
		
		frameIndx++;
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "#%d, rtpHeader->dataSize = %d", frameIndx, rtpHeader->dataSize);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "#%d, rtpHeader->timestamp = %d", frameIndx, rtpHeader->timestamp);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "#%d, rtpHeader->sequenceNumber = %d", frameIndx, rtpHeader->sequenceNumber);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "#%d, isLastPacketOfAFrame = %d", frameIndx, isLastPacketOfAFrame);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "#%d, frameChanged = %d", frameIndx, frameChanged);
	}
#endif

	err = GetLinkededFramesSize( avcPacketParserPtr->frames, &storedSize );
	if( err != 0 ) goto bail;

	if( frameChanged && storedSize != 0 )
	{
		err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), (FskMemPtr*)&outFrame );
		if( err != 0 )	
			goto bail;

		err = MoveLinkedFramesDataToFrame( avcPacketParserPtr->frames, outFrame );
		if( err != 0 )	goto bail;
		avcPacketParserPtr->frames = NULL;

		//put this frame on top of output frame link list
		rtpHeader->frames	= outFrame;
		rtpHeader->dataSize = outFrame->length;//***hack
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "						!!!probably pcket loss, output from stored data: %d!!!", rtpHeader->dataSize);

		if(  naluDataSize > 0 )
		{
			//***if this's the last packet of a frame
			// we'll have to wait next call to output it, not very nice...
			// waiting for multiple frame output support
			// --bryan 6/30/2005
			avcPacketParserPtr->cached_nalu_fu_Size = 0;
#if OUTPUT_MULTIPLE_FRAMES
			if( isLastPacketOfAFrame )
			{
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "************************attaching extra!!!");
				err = PushDataToLinkedFrames( rtpHeader->timestamp, 0, naluDataSize, avcPacketParserPtr->naluDataPtr,&rtpHeader->frames );
			}
			else
#endif			
			err = PushDataToLinkedFrames( rtpHeader->timestamp, 0, naluDataSize, avcPacketParserPtr->naluDataPtr,&avcPacketParserPtr->frames );
			if( err != 0 )	goto bail;
		}
	}
	else if( naluDataSize > 0 )
	{
		avcPacketParserPtr->cached_nalu_fu_Size = 0;
		err = PushDataToLinkedFrames( rtpHeader->timestamp, 0, naluDataSize, avcPacketParserPtr->naluDataPtr, &avcPacketParserPtr->frames );
		if( err != 0 )	goto bail;

		if( !isLastPacketOfAFrame )	//keep appending
		{
			rtpHeader->frames	= NULL;
			rtpHeader->dataSize = 0;//***hack
			goto bail;
		}
		err = GetLinkededFramesSize( avcPacketParserPtr->frames, &storedSize );
		if( err != 0 ) 
			goto bail;

		err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), (FskMemPtr*)&outFrame );
		if( err != 0 ) 
			goto bail;
		
		err = MoveLinkedFramesDataToFrame( avcPacketParserPtr->frames, outFrame );
		if( err != 0 ) 
			goto bail;

		avcPacketParserPtr->frames = NULL;
		
		//***when multiple frame output is supported, 
		// outFrame should be appended to rtpHeader->frames
		// and out size should be set in a appropriate way
		// --bryan 6/30/2005
		//rtpHeader->presentationTime	= outFrame->presentationTime;
		//rtpHeader->timestamp	= outFrame->timestamp;
		rtpHeader->frames	= outFrame;
		rtpHeader->dataSize	= outFrame->length;
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "						output from stored data: %d", rtpHeader->dataSize);
	}
	else
	{
		rtpHeader->frames	= NULL;
		rtpHeader->dataSize	= 0;//*** hack
	}

bail:
	if( err != 0 && outFrame != NULL ) {
		FskMemPtrDispose(outFrame);
		rtpHeader->frames = NULL;
	}

	//if( rtpHeader->frames != NULL && SanityCheckFrame( rtpHeader->frames )!= 0 )
	//	SanityCheckFrame( rtpHeader->frames ); 
	
	//***check key frame 
	if (NULL != rtpHeader) {
		RTPCompressedMediaFrame thisFrame = rtpHeader->frames;
		
		while(  thisFrame )
		{
			unsigned char *p  = (unsigned char *)(thisFrame + 1);
			unsigned char nal_unit_type = 0;
			int			  droppable = 1;
			unsigned long thisSize, total = 0;
		
			while( total < thisFrame->length )
			{
				unsigned char b125 = p[4];
				unsigned char nal_ref_idc;

				thisSize		= ReadLongB(p);
				nal_ref_idc		= (b125&0x60);
				nal_unit_type	= (b125&0x1f);
				
				if( nal_ref_idc != 0 )
					droppable = 0;
				
				if( nal_unit_type == 5 )
					break;
				
				p     += thisSize + 4;
				total += thisSize + 4;
			}

			thisFrame->key = (nal_unit_type == 5);
			thisFrame->droppable = droppable;
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserAVCTypeInstrumentation, "                      ******isKeyFrame: %d", rtpHeader->frames->key);
			thisFrame = thisFrame->next;

		}
	}

	return err;
}

/* -----------------------------------------------------------------------*/

FskErr avcPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr avcPacketParserFlush(RTPPacketParser parser)
{
	AVCPacketParser		*avcPacketParserPtr;
	
	avcPacketParserPtr = (AVCPacketParser *)parser->handlerRefCon;
	
	ClearLinkedFrames( avcPacketParserPtr->frames );
	avcPacketParserPtr->frames = NULL;	
	avcPacketParserPtr->lastTimeStamp = 0;
//	avcPacketParserPtr->lastPacketIsVOG = false;
	
	return 0;
}

// @@ Should live in FskUtilities.c
#define kBase64BadLookupChar				64
#define kBase64PadLookupChar				65
#define kBase64EncodedCharsPerGroup			4

static const char sBase64DecodingTable [256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 65, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

void FskDecodeBase64(const char *inData, long *ioEncodedLength, char *outDecodedData, long *ioDecodedDataLength)
{
	long		encodedDataProcessed = 0;
	const char	*current;
	const char	*end;
	char		lookupChar;
	char		*decodedCurrent;
	char		tempBuffer[4];
	long			countInTempBuffer = 0;
	long			tempNumToDecode;
	long			foundPadChar = 0;
	
	// we will always decode 4 bytes into 3 characters
	current			= inData;
	end				= inData + *ioEncodedLength;
	decodedCurrent	= outDecodedData;

	// process encoded bytes - notice that if the encoded chars
	// (illegal base64 chars are not counted in this - e.g. '\n')
	// are not a mulple of 4, we will only process in multiples of 4
	// and leave any leftovers behind
	while (current < end)  
	{
		if ((lookupChar = sBase64DecodingTable[(unsigned char)(*current)]) != kBase64BadLookupChar)  
		{
			tempBuffer[countInTempBuffer] = lookupChar;
			++countInTempBuffer;
			if ((lookupChar == kBase64PadLookupChar)  &&  (foundPadChar == 0))  
				foundPadChar = countInTempBuffer;

			if (countInTempBuffer == kBase64EncodedCharsPerGroup)  
			{
				// we have 4 encoded characters now - decode it to 3
				// we have to account for padding characters
				switch (foundPadChar)  
				{
					case 1:
						// the whole thing is padding characters??
						// shouldn't happen but you never know
						tempNumToDecode = 0;
						break;
					case 2:
					case 3:
						tempNumToDecode = 1;
						break;
					case 4:
						tempNumToDecode = 2;
						break;
					default:
						tempNumToDecode = 3;
						break;
				}

				if ((long)(decodedCurrent - outDecodedData + tempNumToDecode) > *ioDecodedDataLength)  
					break;// done processing because we ran out of room in the decode buffer
				
				if (tempNumToDecode > 0)  
					decodedCurrent[0] = (tempBuffer [0] << 2) | ((tempBuffer [1] & 0x30) >> 4);

				if (tempNumToDecode > 1)  
					decodedCurrent [1] = ((tempBuffer [1] & 0x0F) << 4) | ((tempBuffer [2] & 0x3C) >> 2);

				if (tempNumToDecode > 2)  
					decodedCurrent [2] = ((tempBuffer [2] & 0x03) << 6) | (tempBuffer [3] & 0x3F);

				countInTempBuffer	 = 0;
				decodedCurrent		+= tempNumToDecode;
				encodedDataProcessed = (current+1 - inData);
				foundPadChar		 = 0;
			}
		}  
		else if (countInTempBuffer == 0)  // if the buffer is full of just illegal characters (e.g. \n), we will deal with it
				encodedDataProcessed = (current+1 - inData);

		++current;
	}

	*ioEncodedLength	 = encodedDataProcessed;
	*ioDecodedDataLength = (decodedCurrent - outDecodedData);
}
