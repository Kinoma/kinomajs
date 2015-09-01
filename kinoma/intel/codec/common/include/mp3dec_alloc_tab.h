/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3DEC_ALLOC_TAB_H__
#define __MP3DEC_ALLOC_TAB_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
Table 3-B.2a Possible quantization per subband

Fs = 48 kHz Bit rates per channel = 56, 64, 80, 96, 112, 128, 160, 192 kbits/s,
and free format
Fs = 44.1 kHz Bit rates per channel = 56, 64, 80 kbits/s
Fs = 32 kHz Bit rates per channel = 56, 64, 80 kbits/s
*/
extern const unsigned char mp3dec_alloc_table1[];

extern const int mp3dec_nbal_alloc_table1[32];

/*
Table 3-B.2b. Possible quantization per subband

Fs = 48 kHz -------------- not relevant --------------
Fs = 44.1 kHz Bitrates per channel = 96, 112, 128, 160, 192 kbits/s
and free format
Fs = 32 kHz Bitrates per channel = 96, 112, 128, 160, 192 kbits/s
and free format
*/

extern const unsigned char mp3dec_alloc_table2[];

extern const int mp3dec_nbal_alloc_table2[32];

/*
Table 3-B.2c. Possible quantization per subband


Fs = 48 kHz Bitrates per channel = 32, 48 kbits/s
Fs = 44.1 kHz Bitrates per channel = 32, 48 kbits/s
Fs = 32 kHz -------- not relevant --------
*/
extern const unsigned char mp3dec_alloc_table3[];

extern const int mp3dec_nbal_alloc_table3[32];


/*
Table 3-B.2d. Possible quantization per subband


Fs = 48 kHz  ------- not relevant -------
Fs = 44.1kHz  ------- not relevant -------
Fs = 32 kHz  Bitrates per channel = 32, 48 kbits/s
*/

extern const unsigned char mp3dec_alloc_table4[];

extern const int mp3dec_nbal_alloc_table4[32];

/*
Table B-1 (MPEG 2). Possible quantization per subband

Fs = 16, 22.05, 24 kHz
*/


extern const unsigned char mp3dec_alloc_table5[];
extern const int mp3dec_nbal_alloc_table5[32];
extern const int mp3dec_cls_quant[17];
extern const int mp3dec_numbits[17];
extern const int mp3dec_sblimit_table[];
extern const short *mp3dec_degroup[];

#ifdef __cplusplus
}
#endif


#endif //__ALLOC_TAB_H__
