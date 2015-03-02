/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "bstream.h"

unsigned int bstream_mask_table[33] = {
  0x0,
  0x01,         0x03,       0x07,       0x0F,
  0x01F,        0x03F,      0x07F,      0x0FF,
  0x01FF,       0x03FF,     0x07FF,     0x0FFF,
  0x01FFF,      0x03FFF,    0x07FFF,    0x0FFFF,
  0x01FFFF,     0x03FFFF,   0x07FFFF,   0x0FFFFF,
  0x01FFFFF,    0x03FFFFF,  0x07FFFFF,  0x0FFFFFF,
  0x01FFFFFF,   0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
  0x1FFFFFFF,   0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

/*******************************************************************/

unsigned int GetNumProcessedByte(sBitsreamBuffer* pBS)
{
  unsigned int ret;

  ret = (pBS->pCurrent_dword - pBS->pBuffer) * sizeof(*pBS->pBuffer);

  ret += (32 - pBS->nBit_offset) / 8;
  return ret;
}

void Byte_alignment(sBitsreamBuffer* pBS)
{
  int     i;

  i = pBS->nBit_offset % 8;
  pBS->nBit_offset -= i;

  if (pBS->nBit_offset == 0) {
    pBS->nBit_offset = 32;
    pBS->pCurrent_dword++;
    pBS->dword = BSWAP(pBS->pCurrent_dword[0]);
  }
}

unsigned int Getbits(sBitsreamBuffer* pBS,
                     unsigned int len)
{
  int     tmp_bit_number;
  unsigned int value;
  unsigned int current_dword;

  tmp_bit_number = pBS->nBit_offset - len;

  current_dword = BSWAP(pBS->pCurrent_dword[0]);
  if (tmp_bit_number > 0) {
    value = current_dword << (32 - pBS->nBit_offset);
    value >>= (32 - len);
    pBS->nBit_offset = tmp_bit_number;
  } else if (tmp_bit_number == 0) {
    value = current_dword << (32 - pBS->nBit_offset);
    value >>= (32 - len);
    pBS->pCurrent_dword++;
    pBS->nBit_offset = 32;
  } else {
    tmp_bit_number = len - pBS->nBit_offset;
    value = (current_dword << (32 - pBS->nBit_offset));
    value >>= (32 - pBS->nBit_offset);
    pBS->pCurrent_dword++;
    current_dword = BSWAP(pBS->pCurrent_dword[0]);
    value <<= tmp_bit_number;
    value += (current_dword >> (32 - tmp_bit_number));
    pBS->nBit_offset = (32 - tmp_bit_number);
  }
  return value;
}

void Putbits(sBitsreamBuffer *pBS,
             unsigned int value,
             int len)
{
  int     tmp_bit_number;
  unsigned int tmp_data;

  tmp_bit_number = pBS->nBit_offset - len;

  if (tmp_bit_number > 0) {
    tmp_data = (pBS->pCurrent_dword[0] >> pBS->nBit_offset);
    tmp_data <<= len;
    tmp_data += value;
    tmp_data <<= tmp_bit_number;
    pBS->pCurrent_dword[0] = tmp_data;
    pBS->nBit_offset = tmp_bit_number;
  } else if (tmp_bit_number == 0) {
    tmp_data = (pBS->pCurrent_dword[0] >> pBS->nBit_offset);
    tmp_data <<= len;
    tmp_data += value;
    pBS->pCurrent_dword[0] = tmp_data;
    pBS->pCurrent_dword++;
    pBS->nBit_offset = 32;
  } else {
    tmp_bit_number = len - pBS->nBit_offset;
    tmp_data = (pBS->pCurrent_dword[0] >> pBS->nBit_offset);
    tmp_data <<= pBS->nBit_offset;
    tmp_data += (value >> tmp_bit_number);
    pBS->pCurrent_dword[0] = tmp_data;
    pBS->pCurrent_dword++;
    pBS->nBit_offset = 32 - tmp_bit_number;
    tmp_data = (value << pBS->nBit_offset);
    pBS->pCurrent_dword[0] = tmp_data;
  }
}

void bs_save(sBitsreamBuffer * pBS)
{
  pBS->saved_nBit_offset = pBS->nBit_offset;
  pBS->saved_init_nBit_offset = pBS->init_nBit_offset;
  pBS->saved_nBufferLen = pBS->nBufferLen;
  pBS->saved_nDataLen = pBS->nDataLen;
  pBS->saved_pBuffer = pBS->pBuffer;
  pBS->saved_pCurrent_dword = pBS->pCurrent_dword;
  pBS->saved_dword = pBS->dword;
}

void bs_restore(sBitsreamBuffer* pBS)
{
  pBS->nBit_offset = pBS->saved_nBit_offset;
  pBS->init_nBit_offset = pBS->saved_init_nBit_offset;
  pBS->nBufferLen = pBS->saved_nBufferLen;
  pBS->nDataLen = pBS->saved_nDataLen;
  pBS->pBuffer = pBS->saved_pBuffer;
  pBS->pCurrent_dword = pBS->saved_pCurrent_dword;
  pBS->dword = pBS->saved_dword;
}

/***********************************************************************

                Alternative bitstream function(s)

***********************************************************************/
/*
#define BITS_IN_BYTE 8
#define BITS_IN_INT 32

unsigned int
get_bits( unsigned char **pp_bitstream, int *p_offset, int num_bits)
{
    unsigned char *p_bitstream = *pp_bitstream;
    unsigned int tmp;
    unsigned int factor = *p_offset + num_bits;
    unsigned int mask = ((unsigned int)( ~0 )) >> *p_offset;

    tmp = ( (unsigned int)( p_bitstream[0] << 3 * BITS_IN_BYTE ) |
            (unsigned int)( p_bitstream[1] << 2 * BITS_IN_BYTE ) |
            (unsigned int)( p_bitstream[2] << 1 * BITS_IN_BYTE ) |
            (unsigned int)( p_bitstream[3] )) & mask;
    tmp = tmp >> ( BITS_IN_INT - factor );
    *pp_bitstream += ( factor >> 3 );
    *p_offset = factor & 0x07;
    return tmp;
}
*/

unsigned int get_bits(unsigned char **pp_bs,
                      int *p_offset,
                      int n)
{
  unsigned int data;
  unsigned int tmp;

  data = 0;
  data += pp_bs[0][0];
  data <<= 8;
  data += pp_bs[0][1];
  data <<= 8;
  data += pp_bs[0][2];
  data <<= 8;
  data += pp_bs[0][3];

  tmp = (unsigned int)pp_bs[0][4];
  data <<= p_offset[0];
  tmp >>= (8 - p_offset[0]);
  data += tmp;

  p_offset[0] += n;
  pp_bs[0] += (p_offset[0] / 8);
  p_offset[0] %= 8;

  data >>= (32 - n);
  return data;
}

void byte_alignment(unsigned char **pp_bitstream,
                    int *p_offset)
{
  if (*p_offset & 0x7) {
    (*pp_bitstream)++;
    *p_offset = 0;
  }
}

int SwapBuffer(unsigned char *pBuffer,
               long len_buffer)
{
  int     i;
  long    len;
  unsigned long temp;
  unsigned long *pSwpBuffer = (unsigned long *)pBuffer;

  len = len_buffer >> 2;
  for (i = 0; i < len; i++) {
    temp = pSwpBuffer[i];
    pSwpBuffer[i] = BSWAP(temp);
  }

  return (len_buffer % (sizeof(unsigned int)));
}

/****************************************************************************/
