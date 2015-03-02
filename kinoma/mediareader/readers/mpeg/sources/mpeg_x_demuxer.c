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
#include "mpeg_x_demuxer.h"

#ifdef MPEG_DEBUG_MESSAGE
FILE *fMPEGErr = NULL;
#endif

#ifndef MPEG_DEBUG_MESSAGE
//void dlog(void *foo, ...) {}
#endif

//PF.J 2012.01.13
int parse_adts_header( const unsigned char *s, int *sizeOut, unsigned char *p, int *offset,  int *frame_size, int *sample_rate, int *channel_total, UInt32 *profile, UInt32 *level, int *bitrate, int frame_info_only );

void find_mp3_boundary_from_end( unsigned char *src, int size, int *boundary )
{
	*boundary = size;
	while( size > 0 ) //PF.J make sure boundary no less than Zero!
	{
		if( src[size] == 0xff && (src[size+1] == 0xfd || src[size+1] == 0xfc))
			break;

		size--;
	}

	*boundary = size;
}


static UInt16 bitrates_02_3[]  = {0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256};
static UInt16 bitrates_02_21[] = {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160};
static UInt16 bitrates_3_3[]   = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448};
static UInt16 bitrates_3_2[]   = {0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384};
static UInt16 bitrates_3_1[]   = {0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320};
int parse_mp3_header_2( unsigned char *scan, int *frame_len )
{
	int err = -1;
	int frameLenCoefficient = 0, frameLenPadding = 0;
	int version, layer, protection, bitrate, frequency, padding, privateF, channelmode, modeextension, copyright, original, emphasis;

	//int		mi_fileSize;

	//int		mi_dataOffset;
	//int		mi_dataSize;

	int			mi_bitrate;				// actually kbps
	int			mi_channelCount;
	int			mi_frequency;
	int			mi_frameLength;
	double			mi_duration;				// in seconds
	unsigned char	*mi_xingToc;

	//***FskMediaMetaData	meta;
	//int			mi_id3TagSize;
	int			mi_samplesPerFrame;
	int			mi_codec;

	if (0xff != scan[0]) goto done;
	if (0xe0 != (scan[1] & 0xe0)) goto done;

	// parse the mp3 format
	version = (scan[1] & 0x18) >> 3;
	layer = (scan[1] & 0x06) >> 1;
	protection = scan[1] & 1;
	bitrate = (scan[2] & 0xf0) >> 4;
	frequency = (scan[2] & 0x0c) >> 2;
	padding = (scan[2] & 0x02) >> 1;
	privateF = scan[2] & 0x01;
	channelmode = (scan[3] & 0xc0) >> 6;
	modeextension = (scan[3] & 0x30) >> 4;
	copyright = (scan[3] & 0x08) >> 3;
	original = (scan[3] & 0x04) >> 2;
	emphasis = scan[3] & 0x03;

	// validate
	if (1 == version) goto done;
	if (0 == layer) goto done;
	if ((0 == bitrate) || (15 == bitrate)) goto done;
	if (3 == frequency) goto done;

	// sort out the bit rate and codec
	mi_codec = 30;//kFskAudioFormatMP3;
	switch (version) {
		case 0:		// 2.5
		case 2:		// 2
			switch (layer) {
				case 3: { // layer I
					bitrate = bitrates_02_3[bitrate];
					mi_codec = 10;//kFskAudioFormatMP1;
					}
					break;

				case 2: // layer II
				case 1: { // layer IIII
					bitrate = bitrates_02_21[bitrate];
					if (2 == layer)
						mi_codec = 20;//kFskAudioFormatMP2;
					}
					break;
			}
			break;

		case 3:		// 1
			switch (layer) {
				case 3: { // layer I
					bitrate = bitrates_3_3[bitrate];
					mi_codec = 10;//kFskAudioFormatMP1;
					}
					break;

				case 2: { // layer II
					bitrate = bitrates_3_2[bitrate];
					mi_codec = 20;//kFskAudioFormatMP2;
					}
					break;

				case 1: { // layer III
					bitrate = bitrates_3_1[bitrate];
					}
					break;
			}
			break;
	}

	// sort out the frequency
	switch (version) {
		case 0:		// 2.5
			if (0 == frequency) frequency = 11025;
			else
			if (1 == frequency) frequency = 12000;
			else
			if (2 == frequency) frequency =  8000;
			break;

		case 2:		// 2
			if (0 == frequency) frequency = 22050;
			else
			if (1 == frequency) frequency = 24000;
			else
			if (2 == frequency) frequency = 16000;
			break;

		case 3:		// 1
			if (0 == frequency) frequency = 44100;
			else
			if (1 == frequency) frequency = 48000;
			else
			if (2 == frequency) frequency = 32000;
			break;
	}


	// figure out the channel count
	if (3 == channelmode)
		mi_channelCount = 1;
	else
		mi_channelCount = 2;

	// work on the frame size
	switch (version) {
		case 3:	// 1
			if (3 == layer)	{ // layer 1
				frameLenCoefficient = 48;
				frameLenPadding = padding * 4;
			}
			else {
				frameLenCoefficient = 144;
				frameLenPadding = padding;
			}
			break;

		case 2:	// 2
		case 0: // 2.5
			if (3 == layer)	{ // layer 1
				frameLenCoefficient = 24;
				frameLenPadding = padding * 4;
			}
			else {
				frameLenCoefficient = 72;
				frameLenPadding = padding;
			}
			break;
	}

	// work on the samples per frame
	mi_samplesPerFrame = 1152;
	switch (version) {
		case 3:	// 1
			if (3 == layer)
				mi_samplesPerFrame = 384;	 // layer 1
			break;

		case 2:	// 2
		case 0: // 2.5
			if (3 == layer)
				mi_samplesPerFrame = 384;	// layer 1
			else if (1 == layer)
				mi_samplesPerFrame = 576;	// layer 3
			break;
	}

	mi_frequency = frequency;
	mi_bitrate = bitrate;

	mi_frameLength = ((frameLenCoefficient * 1000 * bitrate) / frequency) + frameLenPadding;
	mi_duration = 0;
	mi_xingToc = NULL;

	err = 0;

	*frame_len = mi_frameLength;

done:
	return err;
}


int divide_mp3( unsigned char *s, int size, int *total, int *sizes, int *offsets )
{
	int frame_len;
	int idx = 0;
	int frame_total = 0;
	int err = 0;

	while( idx < size )
	{
		err = parse_mp3_header_2( &s[idx],  &frame_len );
		if( err != 0 )
			break;

		frame_total++;
		idx += frame_len;

		*sizes = frame_len;
		sizes++;

		*offsets = 0;
		offsets++;
	}

	*total = frame_total;

	return err;
}


int divide_adts( unsigned char *s, int size, int *total, int offset, int *sizes, int *offsets )
{
	int frame_len;
	int idx = 0;
	int frame_total = 0;
	int err = 0;

	while( idx < size )
	{
		err = parse_adts_header( &s[idx],  NULL, NULL, &offset, &frame_len, NULL, NULL, NULL, NULL, NULL, 1 ); //PF.J 2012.01.13
		if( err != 0 )
			break;

		frame_total++;
		idx += frame_len + offset;

		*sizes = frame_len;// + offset;
		sizes++;
		
		if( offsets != NULL ) 
			*offsets++ = offset;
	}

	*total = frame_total;

	return err;
}


int adts_samplerate_from_index[16] =
{
	96000,	//0x0
	88200,	//0x1
	64000,	//0x2
	48000,	//0x3
	44100,	//0x4
	32000,	//0x5
	24000,	//0x6
	22050,	//0x7
	16000,	//0x8
	12000,	//0x9
	11025,	//0xa
	8000,	//0xb
	0,		//0xc		reserved
	0,		//0xd		reserved
	0,		//0xe		reserved
	0		//0xf		reserved
};



//***bnie: port this code to QTReader.c so that MP4 and MP3 media reader can use it
//beging portable
#define ShowBits(value, numBits) value=( bits32>>(32-(numBits)) );
#define PopBits(ptr, value, numBits)								\
{																	\
	unsigned int byteOffset;										\
	unsigned int tmpUINT;											\
																	\
	value=( bits32>>(32-(numBits)) );								\
																	\
	if(numBits==0) value=0;											\
																	\
	tmpUINT    =bitOffset+(numBits);								\
	byteOffset =(tmpUINT>>3); bitOffset=(tmpUINT&0x7);				\
	ptr		  +=byteOffset;											\
	bits32     =(ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|(ptr[3]<<0);	\
	bits32	 <<=bitOffset;											\
}

#ifndef STAND_ALONE
#include "QTReader.h"
#else
void get_extended_aac_profile_level() {}
#endif

int parse_adts_header( 
					  const unsigned char *s,
					  int			*sizeOut, 
					  unsigned char *p, 
					  int			*offset,  
					  int			*frame_size, 
					  int			*sample_rate, 
					  int			*channel_total, 
					  UInt32		*profile,
					  UInt32		*level,
					  int			*bitrate, 
					  int			frame_info_only 
					  )
{
	long   ID;
	long   Layer;
	long   protection_absent;
	long   Profile;
	long   sampling_frequency_index;
	long   private_bit;
	long   channel_configuration;
	long   original_copy;
	long   Home;
	// long   Emphasis;
	long	copyright_identification_bit;
	long	copyright_identification_start;
	long	aac_frame_length;
	long	adts_buffer_fullness;
	long	no_raw_data_blocks_in_frame;
	SInt32  sbr_present_flag = 0;
    UInt32  sample_rate_ext = 0;//, sbr_objecttypeindex, asc_fLF, asc_dOCC, asc_etnF, synEtnType;
	
	//unsigned int is 32 on both Mac 64-bit
	unsigned int   bits32    = (s[0]<<24)|(s[1]<<16)|(s[2]<<8)|(s[3]<<0);															
	unsigned int   bitOffset = 0;
	unsigned int	tmp;

	dlog("into parse_adts_header\n"); 

	PopBits(s, tmp,12);							//fff	;0,1
	//added by CSQ, check syncword if 'FFF'
	if (tmp != 0xfff)
	{
		printf("not adts syncword");
		return -1;
	}
	//added end

	//ADTS fixed header
	PopBits(s, ID,1);							//0		
	PopBits(s, Layer,2);						//00	
	PopBits(s, protection_absent,1);			//1;	
	PopBits(s, Profile,2);						//01	;2
	PopBits(s, sampling_frequency_index,4);		//01,01	
	PopBits(s, private_bit,1);					//0
	PopBits(s, channel_configuration,3);			//0;10	;3
	PopBits(s, original_copy,1);				//0
	PopBits(s, Home,1);							//0,

	//ADTS variable header
	PopBits(s, copyright_identification_bit,1);	//0		
	PopBits(s, copyright_identification_start,1);//0
	PopBits(s, aac_frame_length,13);			//00;0001,1101;000  4,5
	PopBits(s, adts_buffer_fullness,11);		//1, 1111;1111,11   6
	PopBits(s, no_raw_data_blocks_in_frame,2);	//00

	//added by CSQ
	if (protection_absent == 0)
	{
		PopBits(s, tmp,16);			//crc_check 16 only if protection_absent == 0 

		*frame_size = aac_frame_length - 9;
		*offset = 9;
	}
	else
	{
		*frame_size = aac_frame_length - 7;
		*offset = 7;
	}

	if( frame_info_only )
		return 0;
	
	*sample_rate	= adts_samplerate_from_index[sampling_frequency_index];
	*channel_total	= channel_configuration;
	*bitrate        = 0; //from adts_buffer_fullness?
	*profile        = Profile+1;
	
    //sbr_present_flag = asc_fLF = asc_dOCC = asc_etnF = synEtnType = 0;
	//sbr_sampling_frequency_index = sampling_frequency_index;
	//sbr_objecttypeindex = 2;
	
	get_extended_aac_profile_level(s, *sample_rate, *channel_total, &sample_rate_ext, &sbr_present_flag, profile, level);
	
#ifdef ADTS_ONLY
	return 0;
#endif

	{
		long	size = 0;
		//int		err = 0;
		
		size     = 43;
		*sizeOut = 0;

		*p = 0;					p++;		//version
		*p = 0;					p++;		//flags
		*p = 0;					p++;		
		*p = 0;					p++;		

		//esd starts here
		*p = 0x03;				p++;		//tag
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x22;				p++;		//size

		*p = 0;					p++;		//ES_ID
		*p = 0;					p++;		
		*p = 0;					p++;		//flags&priority: 

		//decoder config descriptor
		*p = 0x04;				p++;		//object type
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x14;				p++;		//size

		*p = 0x40;			p++;		//objectTypeId
		*p = 0x15;			p++;		//streamType

		*p = 0;				p++;		//bufferSizeDB, 24 bits: 0x1800
		*p = 0x18;			p++;		
		*p = 0;				p++;		

		*p = 0;				p++;		//maxBitRate, 32 bits:
		*p = 0x01;			p++;		
		*p = 0xf4;			p++;		
		*p = 0;				p++;		

		*p = 0;				p++;		//avgBitRate, 32 bits:
		*p = 0x01;			p++;		
		*p = 0xf4;			p++;		
		*p = 0;				p++;		

		//decoder specific info
		*p = 0x05;			p++;		//tag
		*p = 0x80;			p++;		//size
		*p = 0x80;			p++;		//size
		*p = 0x80;			p++;		//size
		*p = 0x02;			p++;		//size
		
		*p = (unsigned char )(0x10 |(sampling_frequency_index>>1));
		p++;//0x12;			p++;		//decoderConfig
		*p = (unsigned char )(((sampling_frequency_index&0x01)<<7)| ((channel_configuration&0x0f)<<3));
		p++;//0x10;			p++;
		
		//decoder specific info
		*p = 0x06;			p++;		//tag
		*p = 0x80;			p++;		//size
		*p = 0x80;			p++;		//size
		*p = 0x80;			p++;		//size
		*p = 0x01;			p++;		//size
		*p = 0x02;			p++;		//predefined: use timestamps from mp4 stbl

		//no IPI_DescrPointer
		//no IP_IdentificationDataSet
		//no IPMP_DescriptorPointer
		//no LanguageDescriptor
		//no QoS_Descriptor
		//no RegistrationDescriptor
		//no ExtensionDescriptor

		*sizeOut = size;
	}

//bail:
//	;
	return 0;
}

///
int until_next_start_code( unsigned char *d, int size, int *sc_len )
{
	unsigned char *first = d;
	unsigned char *last = d + size - 3;
	
	*sc_len = 0;

	if( size < 3 )
		return size;

	while( d <= last )
	{
		if( d[0] == 0x00 && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x01  )
		{
			*sc_len = 4;		
			break;
		}

		if( d[0] == 0x00 && d[1] == 0x00 && d[2] == 0x01  )
		{
			*sc_len = 3;		
			break;
		}

		d++;
	}

	if( d >= last+1 )
		return size;

	return d - first;
}



void SetSizeByLen( unsigned char *data, unsigned char len, int size )
{
	switch( len )
	{
	case 1:
		data[0] = (unsigned char)size;
		break;
	case 2:
		data[0] = (size>>8)&0x00ff;
		data[1] = size&0x00ff;
		break;
	case 4:
		data[0]	 = (size>>24)&0x00ff;
		data[1]	 = (size>>16)&0x00ff;
		data[2]	 = (size>>8 )&0x00ff;
		data[3]	 = (size>>0 )&0x00ff;
		break;
	default:
		break;
	}
}


int checkframestart( unsigned char *data, int size, int *hasframe )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;
	int				sc_len = 0;
	int				err = 0;
	int             last_pri_pic_type = -1;
    int             pri_pic_type = -1;

	block_size = until_next_start_code( data, rest_size, &sc_len );
	if( block_size >= rest_size )
		return -1;

	next_data += block_size + sc_len;
	rest_size -= block_size + sc_len;
	
	*hasframe = 0;

	while( 1 )
	{
		unsigned char nalu_type;

		nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
		block_size = until_next_start_code( next_data, rest_size, &sc_len );
        
        if (0x13 == nalu_type) {
            nalu_type = 0x13;
        }

		if ( NAL_UT_PD == nalu_type || 0x13 == nalu_type) {

            pri_pic_type = (next_data[1] >> 5) & 0x3;

            if (pri_pic_type != last_pri_pic_type && last_pri_pic_type != -1) {
                *hasframe = 1;
                break;
            }
            last_pri_pic_type = pri_pic_type;
        }

		next_data += block_size + sc_len;
		rest_size -= block_size + sc_len;

		if( rest_size <= 0 )
			break;
	}
    
    return err;
}

int startcode2lenskip( unsigned char *data, int size, unsigned char *qt_data, int *out_size, int *skip_bytes )
{
    unsigned char	*data_out	= qt_data;
    unsigned char	*next_data	= data;
    int				block_size	= 0;
    int				rest_size	= size;
    int				sc_len = 0;
    int				err = 0;
    int             last_pri_pic_type = -1;
    int             pri_pic_type = -1;
    
    *out_size = 0;
    block_size = until_next_start_code( data, rest_size, &sc_len );
    if( block_size >= rest_size )
        return -1;
    
    next_data += block_size + sc_len;
    rest_size -= block_size + sc_len;
    
    while( 1 )
    {
        unsigned char nalu_type;
        
        nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
        block_size = until_next_start_code( next_data, rest_size, &sc_len );
        
        if( (NAL_UT_SLICE == nalu_type )  || (NAL_UT_IDR_SLICE == nalu_type ) )
        {
            SetSizeByLen( qt_data, NALU_LEN, block_size );
            qt_data	  += NALU_LEN;
            FskMemCopy((void *)qt_data, (void const*)next_data, block_size);
            
            qt_data   += block_size;
            *out_size += NALU_LEN + block_size;
        }
        else if ( NAL_UT_PD == nalu_type )
        {
            pri_pic_type = (next_data[1] >> 5) & 0x3;
            
            if (pri_pic_type != last_pri_pic_type && last_pri_pic_type != -1) {
                
                *skip_bytes = next_data - data - sc_len;
                break;
            }
            last_pri_pic_type = pri_pic_type;
        }
        
        next_data += block_size + sc_len;
        rest_size -= block_size + sc_len;
        
        if( rest_size <= 0 ) {
            *skip_bytes = size;
            break;
        }
    }
    
#if 1
    {
        int size_out = *out_size;
        int is_bad = check_avc_sample_integrity(data_out, size_out, NALU_LEN );
        if( is_bad )
        {
            err = -1;
            //dlog( "avc data failed integrity check!!!\n");
        }
    }
#endif
    
    //bail:
    return err;
}

int startcode2len( unsigned char *data, int size, unsigned char *qt_data, int *out_size )
{
	unsigned char	*data_out	= qt_data;
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;
	int				sc_len = 0;
	int				err = 0;

	 *out_size = 0;
	block_size = until_next_start_code( data, rest_size, &sc_len );
	if( block_size >= rest_size )
		return -1;

	next_data += block_size + sc_len;
	rest_size -= block_size + sc_len;
	
	while( 1 )
	{
		unsigned char nalu_type;

		nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
		block_size = until_next_start_code( next_data, rest_size, &sc_len );

		if( (NAL_UT_SLICE == nalu_type )  || (NAL_UT_IDR_SLICE == nalu_type ) )
		{
			SetSizeByLen( qt_data, NALU_LEN, block_size );
			qt_data	  += NALU_LEN;
			FskMemCopy((void *)qt_data, (void const*)next_data, block_size);

			qt_data   += block_size;
			*out_size += NALU_LEN + block_size;
		}
			
		next_data += block_size + sc_len;
		rest_size -= block_size + sc_len;

		if( rest_size <= 0 )
			break;
	}

#if 1
	{
		int size_out = *out_size;
		int is_bad = check_avc_sample_integrity(data_out, size_out, NALU_LEN );
		if( is_bad )
		{
			err = -1;
			//dlog( "avc data failed integrity check!!!\n");
		}
	}
#endif

//bail:
	return err;
}


int FakeAVCC( unsigned char *data, int size, mpeg_AVCC *avcC )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;
	int				sc_len = 0;
	int				err = 0;

	FskMemSet( avcC, 0, sizeof(mpeg_AVCC) );

	//skip every thing before the first starcode
	block_size = until_next_start_code( data, rest_size, &sc_len );
	if( block_size >= rest_size )//every thing skipped???
		return -1;

	next_data += block_size + sc_len;
	rest_size -= block_size + sc_len;
	
	avcC->naluLengthSize = NALU_LEN;
	avcC->sps = avcC->spspps;

	while( 1 )
	{
		unsigned char nalu_type;

		nalu_type = next_data[0] & NAL_UNITTYPE_BITS;
		block_size = until_next_start_code( next_data, rest_size, &sc_len );

		if( NAL_UT_SPS == nalu_type && avcC->numberofSequenceParameterSets == 0 )
		{
			avcC->spsSize	  = block_size;
			SetSizeByLen( avcC->sps, NALU_LEN, block_size );
			avcC->sps		 += NALU_LEN;
			avcC->spsppsSize  = NALU_LEN + block_size;
			FskMemCopy((void*)avcC->sps, (void*)next_data, block_size);

			avcC->pps		  = avcC->spspps + avcC->spsppsSize;
			avcC->numberofSequenceParameterSets = 1;
		}
		else if( NAL_UT_PPS == nalu_type && avcC->numberofPictureParameterSets == 0 && avcC->numberofSequenceParameterSets == 1)
		{
			avcC->ppsSize	  = block_size;
			SetSizeByLen( avcC->pps, NALU_LEN, block_size );
			avcC->pps		 += NALU_LEN;
			avcC->spsppsSize += NALU_LEN + block_size;
			FskMemCopy((void *)avcC->pps, (void *)next_data, block_size);

			avcC->numberofPictureParameterSets = 1;
		}

		next_data += block_size + sc_len;
		rest_size -= block_size + sc_len;

		if( rest_size <= 0 )
			break;
	}

	if( avcC->numberofSequenceParameterSets == 0 || avcC->numberofPictureParameterSets == 0 )
		err = -1;

//bail:
	return err;
}	


FRAME_BUFFER *get_fb_by_idx( FRAME_BUFFER *track, int pid, int stream_id )
{
	FRAME_BUFFER *ttt = NULL;
	int i;

	for( i = 0; i < MAX_TRACK; i++ )
	{
		FRAME_BUFFER *t = track + i;

		if( t->pid == -1 )
		{
			t->pid = pid; 
			t->stream_id = stream_id; 
			ttt = t;
			break;
		}
		else if( t->pid == pid )
		{
			ttt = t;
			break;
		}
	}

	return ttt;
}


int my_check_err(int err)
{
	int a = 0;
	
	return a;
}


void my_assert(int is_true )
{
	if( !is_true )
	{
		dlog( "MPEG Demuxer assert error!!!\n");
	}
}

void my_umimplemented()
{
	dlog( "unimplemented!!!\n");
}


int parse_pes_packet( unsigned char *p, PES_HEADER *ph, int *offset, int *size )
{
	int mark_bit;
	int	tmp;

	*offset = 0;
	*size   = 0;

	//my_assert(( p[0]== 0x00 && p[1]== 0x00 && p[2]== 0x01 ));
	if (!(p[0]==0x0 && p[1]==0x0 && p[2]==0x1))
		goto bail;

	//FskMemSet( ph, 0, sizeof(PES_HEADER) );

	ph->stream_id = p[3];

	p += 4;
	GET_SHORT_BE(ph->pes_packet_length, p)
	*size   = ph->pes_packet_length + 6;

	if( !IS_VIDEO_STREAM(ph->stream_id) && !IS_AUDIO_STREAM(ph->stream_id))
		goto bail;

	SHOW_BYTE( tmp, p );
	
	if ( ((tmp>>6)&0x3) == 0x2 ) {//MPEG2
		tmp = p[0]; p++;
		ph->always_10				= (tmp>>6)&0x03;
		ph->pes_scrambling_control	= (tmp>>4)&0x03;
		ph->pes_priority			= (tmp>>3)&0x01;
		ph->data_alignment_indicator= (tmp>>2)&0x01;
		ph->copy_right				= (tmp>>1)&0x01;
		ph->original_or_copy		= (tmp>>0)&0x01;

		tmp = p[0]; p++;
		ph->pts_dts_flag				= (tmp>>6)&0x03;
		ph->escr_flag				= (tmp>>5)&0x01;
		ph->es_rate_flag				= (tmp>>4)&0x01;
		ph->dsm_trick_mode_flag		= (tmp>>3)&0x01;
		ph->additional_copy_info_flag = (tmp>>2)&0x01;
		ph->pes_crc_flag				= (tmp>>1)&0x01;
		ph->pes_extension_flag		= (tmp>>0)&0x01;

		tmp = p[0]; p++;
		ph->pes_header_data_length	= tmp;

		*offset  = 9 + ph->pes_header_data_length;
		*size   -= (*offset);

		if( ph->pts_dts_flag == 0x02 )
		{
			DECODE_TS(p, ph->presentation_time_stamp.hi, ph->presentation_time_stamp.lo, 0x02 ) 
		}

		if( ph->pts_dts_flag == 0x03 )
		{
			DECODE_TS(p, ph->presentation_time_stamp.hi,ph->presentation_time_stamp.lo,	0x03 ) 
			DECODE_TS(p, ph->decoding_time_stamp.hi,	ph->decoding_time_stamp.lo,		0x01 ) 
		}

		if( ph->pts_dts_flag == 1)
		{
			my_umimplemented();
		}

		if( ph->escr_flag == 1)
		{
			int tmp32;
			int	tmp16;

			GET_LONG_BE(tmp32, p);
			GET_SHORT_BE(tmp16, p);

			//***bnie: go to detail when needed
			my_umimplemented();
		}

		if( ph->es_rate_flag == 1)
		{
			GET_24BITS_BE(tmp, p);
		
			mark_bit    = (tmp>>23)&0x01;
			ph->ES_rate = (tmp>>1)&0x003fffff;
		}

		if( ph->dsm_trick_mode_flag == 1)
		{
			tmp = p[0]; p++;
			ph->trick_mode_control = (tmp>>5)&0x07;

			//***bnie: go to detail when needed
			my_umimplemented();
		}

		if( ph->additional_copy_info_flag == 1)
		{
			tmp = p[0]; p++;
		
			mark_bit = (tmp>7)&0x01;
			ph->additional_copy_info = tmp&0x7f;
		}

		if( ph->pes_crc_flag == 1)
		{
			GET_SHORT_BE(tmp, p);
			ph->previous_PES_packet_CRC = tmp;
		}

		if( ph->pes_extension_flag == 1)
		{
			my_umimplemented();
		}
	}
	else { //MPEG1
		*offset = 6;
		if ( ((tmp>>6)&0x3) == 0x1 ) {
			GET_SHORT_BE(tmp, p); //2:01, 1:STD_buffer_scale, 13:STD_buffer_size
			SHOW_BYTE(tmp, p);
			*offset += 2;
		}
		if ( ((tmp>>4)&0xf) == 0x2 ) {
			DECODE_TS(p, ph->presentation_time_stamp.hi, ph->presentation_time_stamp.lo, 0x02 )
			*offset += 5;
		}
		else if ( ((tmp>>4)&0xf) == 0x3 ) {
			DECODE_TS(p, ph->presentation_time_stamp.hi, ph->presentation_time_stamp.lo, 0x03 )
			DECODE_TS(p, ph->decoding_time_stamp.hi,	 ph->decoding_time_stamp.lo,	 0x01 )
			*offset += 2*5;
		}
		else {
			GET_BYTE(tmp, p); //0xf
			*offset += 1;
		}
		*size -= (*offset);
	}

	ph->is_valid = 1;

	//else//***bnie: we don't deal with any thing other than audio and video
	//if( IS_PADDING_STREAM(ph->stream_id ) )
	//if( IS_PRIVATE_STREAM(ph->stream_id ) )
	//{
	//	*offset = 6 + ph->pes_packet_length;
	//}

bail:
	return 0;
}


void rest_frame_buffer( FRAME_BUFFER *f )
{
	f->size = 0;	//reset frame data bufer
	f->has_frame = 0;
	INVALIDATE_TIME(&f->time_pre);
	INVALIDATE_TIME(&f->time_dec);
}

void refit_frame_buffer( FRAME_BUFFER *f, int size )
{
	f->size -= size;	//reset frame data bufer
	f->has_frame = 0;
	FskMemMove(f->buf, f->buf + size, f->size);
}

int check_nalu( int is_startcode, unsigned char *data, int size, long *nalu_type_out, long *ref_idc_out )
{
	unsigned char	*next_data	= data;
	int				block_size	= 0;
	int				rest_size	= size;
	int				current_box_size = 0;
	int				sc_len	= 4;

	*nalu_type_out = 0;
	*ref_idc_out   = 0;

	if( is_startcode )
		block_size = until_next_start_code( data, rest_size, &sc_len );
	else
	{
		block_size = 0;
		current_box_size = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]<<0); 
	}

	if( block_size >= rest_size )
		return -1;

	next_data += block_size + sc_len;
	rest_size -= block_size + sc_len;
	
	while( 1 )
	{
		unsigned char nalu_type;
		unsigned char ref_idc;
#define NAL_REF_IDC_BITS 0x60
		nalu_type  = next_data[0] & NAL_UNITTYPE_BITS;
		ref_idc    = next_data[0] & NAL_REF_IDC_BITS;

		if( is_startcode )
			block_size = until_next_start_code( next_data, rest_size, &sc_len );
		else
		{
			unsigned char *n_d = next_data + current_box_size;
			block_size = current_box_size;
			current_box_size = (n_d[0]<<24)|(n_d[1]<<16)|(n_d[2]<<8)|(n_d[3]<<0); 
		}

		if( ( NAL_UT_SLICE == nalu_type )  || ( NAL_UT_IDR_SLICE == nalu_type ) )
		{
			*nalu_type_out = nalu_type;
			*ref_idc_out   = ref_idc;
			return 0;
		}
			
		next_data += block_size + sc_len;
		rest_size -= block_size + sc_len;

		if( rest_size <= 0 )
			break;
	}

	return -1;
}

int check_avc_sample_integrity(unsigned char *src, int size, int naluLengthSize )
{
	int	 s = 0;
	int found_invalid_nalu_size = 0;
	int found_invalid_nalu_type = 0;
#if 0
	int nalu_count = 0;
	static int global_count = 0;

	dlog( "checking #%d: %d\n", global_count, size );
	global_count++;
	if( global_count == 480 )
	{
		int a = 1;
	}
#endif

	while( s <= size - 3 )
	{
		int				src_size = 0;
		//int				dst_size = 0;
		unsigned char	nalu_type;
		int				is_valid_nalu;

		if( naluLengthSize == 4 )
			src_size = (src[s+0]<<24)|(src[s+1]<<16)|(src[s+2]<<8)|(src[s+3]);
		else if(  naluLengthSize == 2 )
			src_size = (src[s+0]<<8)|(src[s+1]<<0);
		else
			src_size = src[s+0];

#if 0
		dlog( "    nalu #%d: %d\n", nalu_count, src_size+naluLengthSize );
		nalu_count++;
#endif

		s += naluLengthSize;

		if( s + src_size > size || src_size <= 0 )	//safe guard frame boundary by checking slice boundary
		{
			found_invalid_nalu_size = 1;
			break;
		}
		nalu_type = src[s] & NAL_UNITTYPE_BITS;
		is_valid_nalu = (NAL_UT_SPS   == nalu_type )   || 
						(NAL_UT_PPS   == nalu_type )	||
						(NAL_UT_SLICE == nalu_type )   || 
						(NAL_UT_IDR_SLICE == nalu_type );

		if( !is_valid_nalu && !found_invalid_nalu_type )
			found_invalid_nalu_type = 1;

		s += src_size;
	}

	return found_invalid_nalu_type || found_invalid_nalu_size;
}


// sloppy mp3 implementation

#define AudioStreamType int
enum
{
    UNDEF_AUDIO             = 0x00000000,
    PCM_AUDIO               = 0x00000001,
    LPCM_AUDIO              = 0x00000002,
    AC3_AUDIO               = 0x00000004,
    TWINVQ_AUDIO            = 0x00000008,

    MPEG1_AUDIO             = 0x00000100,
    MPEG2_AUDIO             = 0x00000200,
    MPEG_AUDIO_LAYER1       = 0x00000010,
    MPEG_AUDIO_LAYER2       = 0x00000020,
    MPEG_AUDIO_LAYER3       = 0x00000040,

    MP1L1_AUDIO             = MPEG1_AUDIO|MPEG_AUDIO_LAYER1,
    MP1L2_AUDIO             = MPEG1_AUDIO|MPEG_AUDIO_LAYER2,
    MP1L3_AUDIO             = MPEG1_AUDIO|MPEG_AUDIO_LAYER3,
    MP2L1_AUDIO             = MPEG2_AUDIO|MPEG_AUDIO_LAYER1,
    MP2L2_AUDIO             = MPEG2_AUDIO|MPEG_AUDIO_LAYER2,
    MP2L3_AUDIO             = MPEG2_AUDIO|MPEG_AUDIO_LAYER3,

    VORBIS_AUDIO            = 0x00000400,
    AAC_AUDIO               = 0x00000800,
    AAC_FMT_UNDEF           = 0x00000000,   /// Undefined stream format, the decoder have to identify by bitsream
    AAC_FMT_RAW             = 0x00000001,   /// Raw input stream format, the fisrt frame keeps init information
    AAC_FMT_EX_GA           = 0x00000010,   /// GASpecificConfig header within the first frame.
    AAC_MPEG4_STREAM        = AAC_AUDIO | AAC_FMT_RAW | AAC_FMT_EX_GA,

    AMR_NB_AUDIO            = 0x00000777,  //narrow band amr
    AMR_WB_AUDIO            = 0x00000778   //wide band amr
};


const int    MPEGAFequency[2][4] =
{{22050, 24000, 16000},{44100, 48000, 32000, 0}};

const int MPEGABitrate[2][3][15] = {
/* MPEG 2 */
{
    /* Layer 1  */
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176,  192, 224, 256},
    /* Layer 2  */
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    /* Layer 3  */
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
}
,
/* MPEG 1 */
{
    /* Layer 1  */
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
    /* Layer 2  */
    {0,32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
    /* Layer 3  */
    {0,32, 40, 48, 56, 64, 80, 96,  112, 128, 160, 192, 224, 256, 320}
}
};

const int MPEGAChannels[]={2,2,2,1};

#define CheckUInt32(data, i)   (long)((( header[i]<<24)& 0xff000000) | ((header[i+1]<<16) & 0x00ff0000) | ( (header[i+2]<<8) & 0x0000ff00) | (header[i+3]&0x000000ff) );
int parse_mp3_header(unsigned char *header, long *sampleRate, long *channelCount, long *bitrate, int *layer )
{
    unsigned char   code1;
    unsigned long   code2;

    //long stream_type1;
	long stream_type2;
    long m_MPEGASyncWord;
    long sample_frequency;
    long channels;
    long bitPerSample;
    //long bitrate;

	dlog("into parse_mp3_header\n");
	
	code1 = header[0];
	code2 = CheckUInt32( header, 0 );

	//if( !( ((code2 & 0xfff00000) == 0xfff00000) || 
	//	((code2 & 0xffffff00) == 0x49443300) || 
	//	((code2 == 0)) ) )
	//	;//return -1;

	{
        unsigned char aux_byte1;
        unsigned char aux_byte2;
        unsigned char aux_byte3;
		int id;
		int la;
		int br;
		int fr;
		int md;
		AudioStreamType mpeg_type[] = {MPEG2_AUDIO, MPEG1_AUDIO};

        aux_byte1 = header[1];
        aux_byte2 = header[2];
        aux_byte3 = header[3];
 
        id  = (aux_byte1&0x08)>>3;
        la  = ((aux_byte1&0x06)>>1);
        br  = (aux_byte2&0xf0) >> 4;
        fr  = (aux_byte2&0x0c)>>2;
        md  = (aux_byte3&0xc0)>>6;

        if((la == 0) && (id == 1))
			return -1;
		
		*layer = la;

        //stream_type1		= MPEG1_PURE_AUDIO_STREAM;
		stream_type2		=  (AudioStreamType) (mpeg_type[id] | (1<<((4-la)+3)));
        m_MPEGASyncWord     = 0x0000fffa;

        sample_frequency   = MPEGAFequency[id][fr];
        channels           = MPEGAChannels[md];
        bitPerSample       = 16;
        *bitrate            = 1000*MPEGABitrate[id][la][br];
    }
	
	*sampleRate   = sample_frequency;
	*channelCount = channels;

	return 0;//false;
}

const int AC3_SamplingRate[] = 
{
	48000,
	44100,
	32000
};

const int AC3_nfchans[] = 
{
	2,
	1,
	2,
	3,
	3,
	4,
	4,
	5
};

const int AC3_bps[] = //kbps
{
	32000,	
	40000,	
	48000,	
	56000,	
	64000,	
	80000,	
	96000,	
	112000,	
	128000,	
	160000,	
	192000,	
	224000,	
	256000,	
	320000,	
	384000,	
	448000,	
	512000,	
	576000,	
	640000	
};

//PF.J 2012.01.16 Function parsing ac3 audio header
int parse_ac3_header( const unsigned char *header, int total_size, long *sampleRate, long *channelCount, long *bitrate )
{
	int err = 0, tmp, code;
	int offset = 0;
	int crc1, fscod, frmsizecod;
	int bsid, bsmod, acmod, cmixlev, surmixlev, dsurmod, lfeon;
	const unsigned char *p = header;

	dlog("into parse_ac3_header, total_size: %d, header: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
			total_size, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15]);
	
	SHOW_SHORT_BE( tmp, p );
	
	while ( tmp != 0x0B77 ) { //check syncword 16 bits
		p ++;
		offset ++;
		
		if (offset >= total_size)
			return kMPEGDemuxerErrNeedMoreTime;
		
		SHOW_SHORT_BE( tmp, p );
	}
	dlog("BREAK2: offset: %d\n", (int)offset);
	
	//Synchronization Information
	GET_SHORT_BE( tmp, p );
	GET_SHORT_BE( crc1, p );
	GET_BYTE( tmp, p );
	fscod      = (tmp >> 6) & 0x3;
	frmsizecod = tmp & 0x3f;
	
	//Bit Stream Information
	GET_BYTE( tmp, p );
	bsid  = (tmp >> 3) & 0x1f;
	bsmod = tmp & 0x7;
	
	GET_BYTE( tmp, p );
	acmod = (tmp >> 5) & 0x7;
	code = (tmp >> 1) & 0x3;
	if ((acmod & 0x1) && (acmod != 0x1))
		cmixlev = code;
	else if (acmod & 0x4)
		surmixlev = code;
	else if (acmod == 0x2)
		dsurmod = code;
	
	lfeon = tmp & 0x1;
	
	//no used stuff, parsing if need.
	//......
	*sampleRate = AC3_SamplingRate[fscod];
	*channelCount = AC3_nfchans[acmod];
	{
		int idx = frmsizecod>>1;
		if( idx <= 18 )
			*bitrate = AC3_bps[idx];
		else
			*bitrate = 0;
	}
	
	return err;
}

//PF.J 2011.01.11 Function parsing mpeg video header.
float frame_rate_value[10] =
{
	0.0,
	23.976,
	24,
	25,
	29.97,
	30,
	50,
	59.94,
	60,
	0.0
};

int next_start_code(unsigned char *mpeg_byts, unsigned int *offset)
{
	unsigned int code, tmp;
	unsigned char *p = mpeg_byts;
	
	SHOW_LONG_BE( code, p );
	
	while ( (code>>8)!=0x01L) {
		GET_BYTE( tmp, p );
		SHOW_LONG_BE( code, p );
	}
	
	*offset = (unsigned int)(p - mpeg_byts);
	
	return code;
}

void sequence_header(MPEG_DEMUXER *md, unsigned char *mpeg_bytes)
{
	int tmp;
	unsigned char *p = mpeg_bytes;
	
	GET_LONG_BE( tmp, p );
	md->msh.horizontal_size = (tmp >> (32-12)) & 0xfff;
	md->msh.vertical_size   = (tmp >> (32-24)) & 0xfff;
	md->msh.aspect_fatio_information = (tmp >> (32-28)) & 0xf;
	md->msh.frame_rate_code = (tmp) & 0xf;
	
	GET_LONG_BE( tmp, p );
	md->msh.bit_rate_code   = (tmp >> (32-18)) & 0x3ffff;
	
	//no used stuff, parsing if need.
	//......
	
	extension_and_user_data( md, p );
	
	return;
}

static void sequence_extension(MPEG_DEMUXER *md, unsigned char *mpeg_bytes)
{
	int tmp;
	unsigned char *p = mpeg_bytes;
	
	GET_LONG_BE( tmp, p );
	//extension_start_code_identifier	4;
	md->mseh.profile_and_level_indication = (tmp >> (32-12)) & 0xff;
	md->mseh.progressive_sequence         = (tmp >> (32-13)) & 0x1;
	md->mseh.chroma_format                = (tmp >> (32-15)) & 0x3;
	md->mseh.horizontal_size_extension    = (tmp >> (32-17)) & 0x3;
	md->mseh.vertical_size_extension	  = (tmp >> (32-19)) & 0x3;
	md->mseh.bit_rate_extension           = (tmp >> (32-31)) & 0xfff;
	//marker_bit	1;
	
	GET_SHORT_BE( tmp, p );
	//vbv_buffer_size_extension	8;
	md->mseh.low_delay					  = (tmp >> (16-9)) & 0x1;
	md->mseh.frame_rate_extension_n		  = (tmp >> (16-11)) & 0x3;
	md->mseh.frame_rate_extension_d		  = (tmp >> (16-16)) & 0x1f;
	
	//no used stuff, parsing if need.
	//......
	
	return;
}

void group_of_pictures_header(MPEG_DEMUXER *md, unsigned char *mpeg_bytes)
{
	;
}

void picture_header(MPEG_DEMUXER *md, unsigned char *mpeg_bytes)
{
	;
}

void extension_and_user_data(MPEG_DEMUXER *md, unsigned char *mpeg_bytes)
{
	int code, ext_ID, tmp;
	unsigned char *p = mpeg_bytes;
	unsigned int offset;
    
    code = next_start_code( p, &offset );
	p += offset;
    
    while (code == EXTENSION_START_CODE || code == USER_DATA_START_CODE)
    {
        GET_LONG_BE( tmp, p ); //skip start code
        if (code == EXTENSION_START_CODE)
        {
            SHOW_BYTE( tmp, p );
			ext_ID = ( tmp>>4 )&0xf;
			
            switch (ext_ID)
            {
				case SEQUENCE_EXTENSION_ID:
					sequence_extension( md, p );
					break;
				case PICTURE_CODING_EXTENSION_ID:
					//picture_coding_extension();
					break;
				default:
					break;
            }
        }
		
        code = next_start_code( p, &offset ); //find next start code
		p += offset;
		
    }
	
}
int parse_mpeg_video_header( unsigned char *data, int size, MPEG_DEMUXER *md )
{	
	unsigned int code, tmp;
	unsigned int offset, enough = 0;
    
    for (;;)
    {
        /* look for next_start_code */
        code = next_start_code( data, &offset );
		data += offset;
        GET_LONG_BE( tmp, data );
        
        switch (code)
        {
			case SEQUENCE_HEADER_CODE:
				sequence_header( md, data );
				enough = 1;
				break;
			case GROUP_START_CODE:
				group_of_pictures_header( md, data );
				break;
			case PICTURE_START_CODE:
				picture_header( md, data );
				//enough = 1;
				break;
			case SEQUENCE_END_CODE:
				//enough = 1;
				break;
			default:
				break;
        }
		if (enough) {
			break;
		}
    }
	
	//MPEG Meta Data
	md->video_profile = (md->mseh.profile_and_level_indication >> 4) & 0x7;
	md->video_level   = (md->mseh.profile_and_level_indication) & 0xf;
	md->video_width   = md->msh.horizontal_size | ((md->mseh.horizontal_size_extension&03) << 12);
	md->video_height  = md->msh.vertical_size | ((md->mseh.vertical_size_extension&0x3) << 12);
	md->video_bitrate = (md->msh.bit_rate_code | ((md->mseh.bit_rate_extension&0xfff) << 18))*400 / 1000;
	md->video_framerate = frame_rate_value[md->msh.frame_rate_code * (md->mseh.frame_rate_extension_n+1)/(md->mseh.frame_rate_extension_d+1)];
	
	return 0;

}

static int find_mpeg_header_size(unsigned char *bs_bytes, int size)
{
	unsigned char *p = bs_bytes;

	while (p < bs_bytes + size)
	{
		if ( *p==0x0 && *(p+1)==0x0 && *(p+2)==0x1 && *(p+3)==0x0 )
			break;
		else
			p ++;
	}
	
	return (int)(p - bs_bytes);
}

int md_new( MPEG_DEMUXER **md_out  )
{
	MPEG_DEMUXER *md;
	int i;
	int err = 0;

	err = FskMemPtrNewClear( sizeof( MPEG_DEMUXER ), (FskMemPtr *)&md );
	BAIL_IF_ERR( err );

	for( i = 0; i < MAX_TRACK; i++ )
	{
		md->fb_ary[i].buf		= NULL;
		md->fb_ary[i].buf_size	= 0;
		md->fb_ary[i].has_frame = 0;
		md->fb_ary[i].pid		= -1;
		md->fb_ary[i].stream_id	= -1;

		rest_frame_buffer(&md->fb_ary[i]);
	}

bail:
	if( err != 0 )
	{
		md_dispose( md );
		md = NULL;
	}

	*md_out = md;

	return err;
}


void md_dispose( MPEG_DEMUXER *md )
{
	int i;
	//int err = 0;

	if( md != NULL )
	{
		for( i = 0; i < MAX_TRACK; i++ )
		{
			if( md->fb_ary[i].buf != NULL )
				FskMemPtrDisposeAt( (void **)&md->fb_ary[i].buf );
		}

		if( md->audio_codec_header_data != NULL )
			FskMemPtrDisposeAt( (void **)&md->audio_codec_header_data );
		
		if( md->video_codec_header_data != NULL )
			FskMemPtrDisposeAt( (void **)&md->video_codec_header_data );

		FskMemPtrDisposeAt( (void **)&md );
	}
}


int md_reset_buffer( MPEG_DEMUXER *md )
{
	int i;

	for( i = 0; i < MAX_TRACK; i++ )
		rest_frame_buffer( &md->fb_ary[i] );

	return 0;
}

#define VIDEO_IS_MPEG2
#define VIDEO_IS_AVC
#define AUDIO_IS_AAC
#define AUDIO_IS_MP3

int md_parse_audio_codec( MPEG_DEMUXER *md, unsigned char *bs_bytes, int bs_size )
{
	int				aac_esds_size;
	unsigned char	aac_esds[64];
	int				audio_frame_size = 0;
	int				err = 0;

	dlog("into md_parse_audio_codec\n");
	
	if( md->audio_format == TRACK_AAC || md->audio_format == TRACK_AAC_x )
	{	
		//PF.J 2012.01.13
		err = parse_adts_header( bs_bytes, &aac_esds_size, aac_esds, &md->audio_sample_offset, &audio_frame_size, &md->audio_samplerate, &md->audio_channel_total, &md->audio_profile, &md->audio_level, &md->audio_bitrate, 0 );
		if( err == 0 )
		{
			err = FskMemPtrNewClear( sizeof( MPEG_DEMUXER ), &md->audio_codec_header_data );
			BAIL_IF_ERR( err );

			FskMemCopy(md->audio_codec_header_data, aac_esds, aac_esds_size);
			md->audio_codec_header_data_size = aac_esds_size;
			md->audio_header_initialized = 1;
		}
		else
			err = kMPEGDemuxerErrNeedMoreTime;
	}
	else if( md->audio_format == TRACK_MPEGA || md->audio_format == TRACK_MPEGA_x )
	{
		err = parse_mp3_header((unsigned char *)bs_bytes, (long *)&md->audio_samplerate, (long *)&md->audio_channel_total, (long *)&md->audio_bitrate, (int *)&md->audio_layer);
		if( err == 0 )
			md->audio_header_initialized = 1;
		else
			err = kMPEGDemuxerErrNeedMoreTime;
	}
	else if( md->audio_format == TRACK_AC3)
	{
		err = parse_ac3_header(bs_bytes, bs_size, (long *)&md->audio_samplerate, (long *)&md->audio_channel_total, (long *)&md->audio_bitrate);
		if (err == 0) 
		{
			md->audio_header_initialized = 1;
		}
		else if (err == kMPEGDemuxerErrNeedMoreTime)
		{
			err = 0;
			md->audio_header_initialized = 0;
		}
	}
	else
		err = -9;//kFskErrUnimplemented

bail:
	dlog("out of md_parse_audio_codec, samplerate: %d, channel_total: %d\n", md->audio_samplerate, md->audio_channel_total);
	
	return err;
}

int md_parse_video_codec( MPEG_DEMUXER *md, unsigned char *bs_bytes, int bs_size )
{
	int err = 0;

	dlog("into md_parse_video_codec\n");

	if( md->video_format == TRACK_H264 || md->video_format == TRACK_H264_x )
	{
		mpeg_AVCC	avcC;

		my_assert( (bs_bytes[0]==0x00 && bs_bytes[1]==0x00 && bs_bytes[2]==0x00 && bs_bytes[3]==0x01 && bs_bytes[4]==0x09  ));
		
		err = FakeAVCC( bs_bytes, bs_size, &avcC );	
		if( err == 0 )
		{
			md->video_header_initialized = 1;
			md->video_timescale			= MPEG_VIDEO_DEFAULT_TIME_SCALE;	

			err = FskMemPtrNewClear( avcC.spsppsSize, &md->video_codec_header_data );
			BAIL_IF_ERR( err );

			FskMemCopy(md->video_codec_header_data, avcC.spspps, avcC.spsppsSize);
			md->video_codec_header_data_size = avcC.spsppsSize;
		}
		err = 0;
	}
	else if( md->video_format == TRACK_MPEG1V || md->video_format == TRACK_MPEG2V )
	{
#if 0//PF.J 2012.01.12
		err = -9;//kFskErrUnimplemented
#else		
		int video_codec_header_data_size = find_mpeg_header_size(bs_bytes, bs_size);
		
		err = parse_mpeg_video_header(bs_bytes, bs_size, md);
		
		if ( err == 0 )
		{
			//***put the whole chunk here for now  --bnie 1/6/2012
			md->video_header_initialized = 1;
			md->video_timescale			= MPEG_VIDEO_DEFAULT_TIME_SCALE;	
		
			err = FskMemPtrNewClear( video_codec_header_data_size, &md->video_codec_header_data );
			BAIL_IF_ERR( err );
		
			FskMemCopy(md->video_codec_header_data, bs_bytes, video_codec_header_data_size);
			md->video_codec_header_data_size = video_codec_header_data_size;
		}
		err = 0;
#endif		
	}
	else
	{
		md->video_header_initialized = 1;
		md->video_timescale			 = MPEG_VIDEO_DEFAULT_TIME_SCALE;	
	}

bail:
	return err;
}


#ifdef STAND_ALONE

void FskMemCopy(void *dst, const void *src, UInt32 count)
{
	memcpy( dst, src, count );
}


void FskMemSet(void *dst, char fill, UInt32 count)
{
	memset(dst, fill, count );
}

FskErr FskMemPtrNew(UInt32 size, FskMemPtr *newMemory)
{
	*newMemory = malloc(size);
	if( *newMemory == NULL )
		return -1;
	else
		return 0;
}

FskErr FskMemPtrNewClear(UInt32 size, FskMemPtr *newMemory)
{
	*newMemory = malloc(size);
	if( *newMemory == NULL )
		return -1;

	memset(*newMemory, 0, size);

	return 0;
}

FskErr FskMemPtrDisposeAt(void **ptr)
{
	if( *ptr != NULL )
		free( *ptr );
	else
		return -1;

	*ptr = NULL;

	return 0;
}


#endif
