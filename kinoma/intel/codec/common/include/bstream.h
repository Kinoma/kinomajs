/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __BSTREAM_H
#define __BSTREAM_H

//***
#include "kinoma_ipp_env.h"

typedef struct
{
  unsigned int* pBuffer;                  //
  int           nBufferLen;               //
  int           nDataLen;                 //
  int           init_nBit_offset;         //
  int           nBit_offset;              // 32->1
  unsigned int  dword;                    //
  unsigned int* pCurrent_dword;           //

  unsigned int* saved_pBuffer;            //
  int           saved_nBufferLen;         //
  int           saved_nDataLen;           //
  int           saved_init_nBit_offset;   //
  int           saved_nBit_offset;        // 32->1
  unsigned int  saved_dword;              //
  unsigned int* saved_pCurrent_dword;     //

} sBitsreamBuffer;

/*******************************************************************/
#ifdef _BIG_ENDIAN_
#define BSWAP(x) (x)
#else
#define BSWAP(x) (unsigned int)(((x) << 24) | (((x)&0xff00) << 8) | (((x) >> 8)&0xff00) | ((x&0xff000000) >> 24));
#endif
/*******************************************************************/

#define LOAD_DWORD(pBS)                                             \
  (pBS)->dword = BSWAP((pBS)->pCurrent_dword[0]);                   \
  (pBS)->dword &= ~bstream_mask_table[(pBS)->nBit_offset];

#define GET_LOAD_DWORD(pBS)                                             \
  (pBS)->dword = BSWAP((pBS)->pCurrent_dword[0]);                   \
  (pBS)->dword &= bstream_mask_table[(pBS)->nBit_offset];


/*******************************************************************/

#define INIT_BITSTREAM(pBS, ptr)                                    \
{                                                                   \
  unsigned char *tmp_ptr = (unsigned char*)ptr;                     \
  (pBS)->pBuffer = (unsigned int *)(tmp_ptr - (((long)tmp_ptr) & 3));\
  (pBS)->nBit_offset = 32 - ((((long)tmp_ptr) & 3) << 3);            \
  (pBS)->init_nBit_offset = (pBS)->nBit_offset;                     \
  (pBS)->pCurrent_dword = (pBS)->pBuffer;                           \
  LOAD_DWORD(pBS)                                                   \
}

#define GET_INIT_BITSTREAM(pBS, ptr)                                    \
{                                                                   \
  unsigned char *tmp_ptr = (unsigned char*)ptr;                     \
  (pBS)->pBuffer = (unsigned int *)(tmp_ptr - (((long)tmp_ptr) & 3));\
  (pBS)->nBit_offset = 32 - ((((long)tmp_ptr) & 3) << 3);            \
  (pBS)->init_nBit_offset = (pBS)->nBit_offset;                     \
  (pBS)->pCurrent_dword = (pBS)->pBuffer;                           \
  GET_LOAD_DWORD(pBS)                                                   \
}

/*******************************************************************/

#define SAVE_BITSTREAM(pBS)                                         \
{                                                                   \
  int _nbits = (pBS)->nBit_offset;                                  \
  unsigned int _cw0 = (pBS)->dword;                                 \
  unsigned int _cw1;                                                \
  if (_nbits != 32) {                                               \
    _cw1 = (pBS)->pCurrent_dword[0];                                \
    _cw1 = BSWAP(_cw1);                                             \
    _cw1 &= bstream_mask_table[_nbits];                                     \
    _cw0 = _cw0 | _cw1;                                             \
    _cw0 = BSWAP(_cw0);                                             \
    (pBS)->pCurrent_dword[0] = _cw0;                                \
  }                                                                 \
}

/*******************************************************************/

#define PUT_BITS(pBS, cw, n)                                        \
{                                                                   \
  int _nbits = (pBS)->nBit_offset;                                  \
  unsigned int _cw0 = (pBS)->dword;                                 \
  unsigned int _cw;                                                 \
  _cw = (cw) & bstream_mask_table[(n)];                                     \
  if (_nbits <= (n)) {                                              \
    _cw0 = _cw0 | (_cw >> ((n) - _nbits));                          \
    _cw0 = BSWAP(_cw0);                                             \
    (pBS)->pCurrent_dword[0] = _cw0;                                \
    (pBS)->pCurrent_dword++;                                        \
    _nbits = 32 - ((n) - _nbits);                                   \
    if (_nbits != 32) {                                             \
      _cw0 = _cw << _nbits;                                         \
    } else {                                                        \
      _cw0 = 0;                                                     \
    }                                                               \
  } else {                                                          \
    _nbits -= (n);                                                  \
    _cw0 = _cw0 | (_cw << _nbits);                                  \
  }                                                                 \
  (pBS)->nBit_offset = _nbits;                                      \
  (pBS)->dword = _cw0;                                              \
}

/*******************************************************************/

#define GET_BITS(pBS, res_value, nbits)                             \
{                                                                   \
  int     tmp_bit_number;                                           \
  unsigned int value;                                               \
  unsigned int current_dword;                                       \
  unsigned int blen = (nbits);                                      \
                                                                    \
  tmp_bit_number = (pBS)->nBit_offset - blen;                       \
                                                                    \
  current_dword = (pBS)->dword;                                     \
  if (tmp_bit_number > 0) {                                         \
    value = current_dword << (32 - (pBS)->nBit_offset);             \
    value >>= (32 - blen);                                          \
    (pBS)->nBit_offset = tmp_bit_number;                            \
  } else if (tmp_bit_number == 0) {                                 \
    value = current_dword << (32 - (pBS)->nBit_offset);             \
    value >>= (32 - blen);                                          \
    (pBS)->pCurrent_dword++;                                        \
    current_dword = BSWAP((pBS)->pCurrent_dword[0]);                \
    (pBS)->dword = current_dword;                                   \
    (pBS)->nBit_offset = 32;                                        \
  } else {                                                          \
    tmp_bit_number = blen - (pBS)->nBit_offset;                     \
    value = (current_dword << (32 - (pBS)->nBit_offset));           \
    value >>= (32 - (pBS)->nBit_offset);                            \
    (pBS)->pCurrent_dword++;                                        \
    current_dword = BSWAP((pBS)->pCurrent_dword[0]);                \
    (pBS)->dword = current_dword;                                   \
    value <<= tmp_bit_number;                                       \
    value += (current_dword >> (32 - tmp_bit_number));              \
    (pBS)->nBit_offset = (32 - tmp_bit_number);                     \
  }                                                                 \
  (res_value) = value;\
}

/*******************************************************************/

#define GET_BITS_COUNT(pBS, size)                                   \
  size =(((pBS)->pCurrent_dword) - ((pBS)->pBuffer)) * 32 +         \
          (pBS)->init_nBit_offset - ((pBS)->nBit_offset);

/*******************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

extern unsigned int bstream_mask_table[33];

unsigned int GetNumProcessedByte(sBitsreamBuffer* pBS);
void         Byte_alignment(sBitsreamBuffer* pBS);
unsigned int Getbits(sBitsreamBuffer* pBS, unsigned int len);
void         Putbits(sBitsreamBuffer* pBS, unsigned int value, int len);
void         bs_save(sBitsreamBuffer* pBS);
void         bs_restore(sBitsreamBuffer* pBS);


/***********************************************************************

                Alternative bitstream function(s)

***********************************************************************/

unsigned int get_bits( unsigned char **pp_bitstream, int *p_offset, int num_bits);
void         byte_alignment(unsigned char **pp_bitstream, int *p_offset);

int SwapBuffer(unsigned char *pBuffer, long len_buffer);

#ifdef  __cplusplus
}
#endif

#endif//__BSTREAM_H
