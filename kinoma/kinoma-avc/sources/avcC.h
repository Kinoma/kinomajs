/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#ifndef __AVCC__
#define __AVCC__

typedef struct
{
	unsigned char 	configurationVersion;
	unsigned char 	AVCProfileIndication;
	unsigned char 	profile_compatibility;
	unsigned char 	ACVLevelIndication;
	unsigned char 	naluLengthSize;
	unsigned char 	numberofPictureParameterSets;
	unsigned char 	numberofSequenceParameterSets;

	unsigned char 	*sps;
	unsigned short 	spsSize;
	unsigned char 	*pps;
	unsigned short 	ppsSize;
	
	unsigned char 	spspps[256];
	unsigned short 	spsppsSize;
} AVCC;

typedef struct
{
	long			isValid;
	long			pixelFormat;
	long			width16;
	long			height16;
	long			left;
	long			top;
	long			right;
	long			bottom;
	long			rowBytesY;
	long			rowBytesCbCr;
	unsigned char  *y;
	unsigned char  *cb;
	unsigned char  *cr;
} YUVPlannar;

#endif
