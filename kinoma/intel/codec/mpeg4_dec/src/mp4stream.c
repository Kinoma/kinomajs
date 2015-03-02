/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    Decodes MPEG-4 bitstream.
//
*/

//***
#include "kinoma_ipp_lib.h"

#include "mp4.h"


#ifndef USE_INLINE_BITS_FUNC

Ipp32u mp4_ShowBits(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= pInfo->bitoff;
    tmp >>= 32 - n;
    return tmp;
}

Ipp32u mp4_ShowBit(mp4_Info* pInfo)
{
    Ipp32u tmp = pInfo->bufptr[0];
    tmp >>= 7 - pInfo->bitoff;
    return (tmp & 1);
}

Ipp32u mp4_ShowBits9(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] <<  8) | ptr[1];
    tmp <<= (pInfo->bitoff + 16);
    tmp >>= 32 - n;
    return tmp;
}

void mp4_FlushBits(mp4_Info* pInfo, int n)
{
    n = n + pInfo->bitoff;
    pInfo->bufptr += n >> 3;
    pInfo->bitoff = n & 7;
}

Ipp32u mp4_GetBits(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= pInfo->bitoff;
    tmp >>= 32 - n;
    n = n + pInfo->bitoff;
    pInfo->bufptr += n >> 3;
    pInfo->bitoff = n & 7;
    return tmp;
}

/*
Ipp32u mp4_GetBit(mp4_Info* pInfo)
{
    Ipp32u tmp = pInfo->bufptr[0];
    if (pInfo->bitoff == 7) {
        pInfo->bitoff = 0;
        pInfo->bufptr ++;
    } else {
        tmp >>= 7 - pInfo->bitoff;
        pInfo->bitoff ++;
    }
    return (tmp & 1);
}
*/

Ipp32u mp4_GetBits9(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 8) | ptr[1];
    tmp <<= (pInfo->bitoff + 16);
    tmp >>= 32 - n;
    n = n + pInfo->bitoff;
    pInfo->bufptr += n >> 3;
    pInfo->bitoff = n & 7;
    return tmp;
}

/*
//  align bit stream to the byte boundary
*/
void mp4_AlignBits(mp4_Info* pInfo)
{
    if (pInfo->bitoff > 0) {
        pInfo->bitoff = 0;
        (pInfo->bufptr)++;
    }
}

/*
//  align bit stream to the byte boundary (skip 0x7F)
*/
void mp4_AlignBits7F(mp4_Info* pInfo)
{
    if (pInfo->bitoff > 0) {
        pInfo->bitoff = 0;
        (pInfo->bufptr)++;
    } else {
        if (*pInfo->bufptr == 0x7F)
            (pInfo->bufptr)++;
    }
}

Ipp32u mp4_ShowBitsAlign(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bitoff ? (pInfo->bufptr + 1) : pInfo->bufptr;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp >>= 32 - n;
    return tmp;
}

Ipp32u mp4_ShowBitsAlign7F(mp4_Info* pInfo, int n)
{
    Ipp8u* ptr = pInfo->bitoff ? (pInfo->bufptr + 1) : pInfo->bufptr;
    Ipp32u tmp;
    if (!pInfo->bitoff) {
        if (*ptr == 0x7F)
            ptr ++;
    }
    tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp >>= 32 - n;
    return tmp;
}

#endif


/*
//  check next bits are resync marker
*/
int mp4_CheckResyncMarker(mp4_Info* pInfo, int rml)
{
    // check zero bit
    if (mp4_ShowBit(pInfo) != 0)
        return 0;
    // check stuffing bits
    if (mp4_ShowBits9(pInfo, 8 - pInfo->bitoff) != (Ipp32u)((1 << (7 - pInfo->bitoff)) - 1))
        return 0;
    {
        Ipp8u* ptr = pInfo->bufptr + 1;
        Ipp32u tmp;
        tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
        tmp >>= 32 - rml;
        return (tmp == 1);
    }
}


/*
//  find MPEG-4 start code in buffer
*/
Ipp8u* mp4_FindStartCodePtr(mp4_Info* pInfo)
{
    int     i, len = pInfo->buflen - (pInfo->bufptr - pInfo->buffer);
    Ipp8u*  ptr = pInfo->bufptr;
    for (i = 0; i < len - 3; i++) {
        if (ptr[i] == 0 && ptr[i + 1] == 0 && ptr[i + 2] == 1) {
            return ptr + i + 3;
        }
    }
    return NULL;
}


/*
//  find MPEG-4 start code or short_vedo_header code in buffer
*/
Ipp8u* mp4_FindStartCodeOrShortPtr(mp4_Info* pInfo)
{
    int     i, len = pInfo->buflen - (pInfo->bufptr - pInfo->buffer);
    Ipp8u*  ptr = pInfo->bufptr;
    for (i = 0; i < len - 3; i++) {
        if (ptr[i] == 0 && ptr[i + 1] == 0 && ptr[i + 2] == 1) {
            return ptr + i + 3;
        }
        // short_video_header
        //***
        if (ptr[i] == 0 && ptr[i + 1] == 0 && (((ptr[i + 2] & 0xFC) == 0x80)|| ((ptr[i + 2] & 0xFC) == 0x84)) ) {
            return ptr + i;
        }
    }
    return NULL;
}


/*
//  find MPEG-4 start code in stream
*/
int mp4_SeekStartCodePtr(mp4_Info* pInfo)
{
    Ipp8u* ptr;

    if (pInfo->bitoff) {
        pInfo->bufptr ++;
        pInfo->bitoff = 0;
    }
    ptr = mp4_FindStartCodePtr(pInfo);
    if (ptr) {
        pInfo->bufptr = ptr;
        return 1;
    } else {
        pInfo->bufptr = pInfo->buffer + pInfo->buflen - 3;
        return 0;
    }
}


/*
//  find MPEG-4 start code or short_vedo_header code  in stream
*/
int mp4_SeekStartCodeOrShortPtr(mp4_Info* pInfo)
{
    Ipp8u* ptr;

    if (pInfo->bitoff) {
        pInfo->bufptr ++;
        pInfo->bitoff = 0;
    }
    ptr = mp4_FindStartCodeOrShortPtr(pInfo);
    if (ptr) {
        pInfo->bufptr = ptr;
        return 1;
    } else {
        pInfo->bufptr = pInfo->buffer + pInfo->buflen - 3;
        return 0;
    }
}


/*
//  find MPEG-4 start code value in stream
*/
int mp4_SeekStartCodeValue(mp4_Info* pInfo, Ipp8u code)
{
    while (mp4_SeekStartCodePtr(pInfo)) {
        if (*(pInfo->bufptr) == code) {
            (pInfo->bufptr) ++;
            return 1;
        }
    }
    return 0;
}


/*
//  find ptr to ShortVideoStartMarker in buffer
*/
Ipp8u* mp4_FindShortVideoStartMarkerPtr(mp4_Info* pInfo)
{
    int     i, len = pInfo->buflen - (pInfo->bufptr - pInfo->buffer);
    Ipp8u*  ptr = pInfo->bufptr;
    for (i = 0; i < len - 3; i++) 
	{//***
		if (
			(ptr[i] == 0 && ptr[i + 1] == 0 && (ptr[i + 2] & (~3)) == 0x80) ||
			(ptr[i] == 0 && ptr[i + 1] == 0 && (ptr[i + 2] & 0xF0) == 0x80) 
			)
		{
            return ptr + i + 2;
        }
    }
    return NULL;
}


/*
//  find ShortVideoStartMarker value in stream
*/
int mp4_SeekShortVideoStartMarker(mp4_Info* pInfo)
{
    Ipp8u* ptr;

    if (pInfo->bitoff) {
        pInfo->bufptr ++;
        pInfo->bitoff = 0;
    }
    ptr = mp4_FindShortVideoStartMarkerPtr(pInfo);
    if (ptr) {
        pInfo->bufptr = ptr;
        return 1;
    } else {
        pInfo->bufptr = pInfo->buffer + pInfo->buflen - 3;
        return 0;
    }
}

