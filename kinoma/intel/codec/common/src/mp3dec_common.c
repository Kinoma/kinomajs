/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

//***
#include "kinoma_ipp_lib.h"

#include "mp3dec_own.h"

/*
//  Functions in this file.
//
//  int mp3dec_SetAllocTable(MP3Dec *state)
//  int mp3dec_GetSynch(sDecoderContext* DC)
//  int main_data_slots(sDecoderContext* DC)
//  int mp3dec_ReadMainData(sDecoderContext* DC)
*/

#define HDR_VERSION(x)       ((x & 0x80000) >> 19)
#define HDR_LAYER(x)         (4 - ((x & 0x60000) >> 17))
#define HDR_ERRPROTECTION(x) ((x & 0x10000) >> 16)
#define HDR_BITRADEINDEX(x)  ((x & 0x0f000) >> 12)
#define HDR_SAMPLINGFREQ(x)  ((x & 0x00c00) >> 10)
#define HDR_PADDING(x)       ((x & 0x00200) >> 9)
#define HDR_EXTENSION(x)     ((x & 0x00100) >> 8)
#define HDR_MODE(x)          ((x & 0x000c0) >> 6)
#define HDR_MODEEXT(x)       ((x & 0x00030) >> 4)
#define HDR_COPYRIGHT(x)     ((x & 0x00008) >> 3)
#define HDR_ORIGINAL(x)      ((x & 0x00004) >> 2)
#define HDR_EMPH(x)          ((x & 0x00003))

/* MPEG-1 12-bit code embedded in the audio bitstream that identifies
the start of a frame (p.20 ISO/IEC 11172-3) */
static unsigned int SYNC_WORD = 0xffe;

/* MPEG-1 bitrate tables: (p.21 ISO/IEC 11172-3) */
int     mp3dec_bitrate[2][3][15] = {
    { /* MPEG 2 */
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256}, /* Layer 1 */
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},      /* Layer 2 */
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}       /* Layer 3 */
    } ,
    { /* MPEG 1 */
      {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448}, /* Layer 1 */
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},    /* Layer 2 */
      {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}      /* Layer 3 */
    }
};

/* MPEG-1 sampling rates (p.21 ISO/IEC 11172-3) */
int mp3dec_frequency[3][4] = { {22050, 24000, 16000}, {44100, 48000, 32000, 0}, {11025, 12000,  8000} };

#define MP3DEC_END_OF_BUFFER(BS) \
    (((((BS)->pCurrent_dword - (BS)->pBuffer + 1) * 32 - (BS)->nBit_offset) <= \
            (BS)->nDataLen * 8 - 8) ? 0 : 1)

/*****************************************************************************/

/******************************************************************************
//  Name:
//    mp3dec_main_data_slots
//
//  Description:
//    calculate number of bytes beetween two successive frames minus length of
//    (header & side info)
//  Input Arguments:
//    DC - point to Decoder context
//
//  Returns:
//    number of bytes beetween two successive frames minus length of
//    (header & side info)
//
******************************************************************************/

static int mp3dec_main_data_slots(int stereo, int header_id, int header_protectionBit)
{
    int     nSlots = 0;

    if (header_id == 1) {
        if (stereo == 1)
            nSlots -= 17;
        else
            nSlots -= 32;
    }

    if (header_id == 0) {
        if (stereo == 1)
            nSlots -= 9;
        else
            nSlots -= 17;
    }

    if (header_protectionBit == 0)
        nSlots -= 2;

    return nSlots;
}

/******************************************************************************/

int mp3dec_SetAllocTable(MP3Dec_com *state)
{
    IppMP3FrameHeader *header = &(state->header);
    int bit_rate = mp3dec_bitrate[header->id][header->layer - 1][header->bitRate] >> (state->stereo - 1);
    int freq = mp3dec_frequency[header->id + state->mpg25][header->samplingFreq];
    int           *nbal_alloc_table;
    unsigned char *alloc_table;
    int           sblimit;

    if (header->id == 0) {
        alloc_table = (unsigned char *)mp3dec_alloc_table5;
        nbal_alloc_table = (int *)mp3dec_nbal_alloc_table5;
        sblimit = mp3dec_sblimit_table[4];
    } else if (bit_rate <= 48) {
        if (freq == 32000) {
            alloc_table = (unsigned char *)mp3dec_alloc_table4;
            nbal_alloc_table = (int *)mp3dec_nbal_alloc_table4;
            sblimit = mp3dec_sblimit_table[3];
        } else {
            alloc_table = (unsigned char *)mp3dec_alloc_table3;
            nbal_alloc_table = (int *)mp3dec_nbal_alloc_table3;
            sblimit = mp3dec_sblimit_table[2];
        }
    } else if (bit_rate <= 80) {
        alloc_table = (unsigned char  *)mp3dec_alloc_table1;
        nbal_alloc_table = (int *)mp3dec_nbal_alloc_table1;
        sblimit = mp3dec_sblimit_table[0];
    } else if (bit_rate <= 192) {
        if (freq == 48000) {
            alloc_table = (unsigned char *)mp3dec_alloc_table1;
            nbal_alloc_table = (int *)mp3dec_nbal_alloc_table1;
            sblimit = mp3dec_sblimit_table[0];
        } else {
            alloc_table = (unsigned char *)mp3dec_alloc_table2;
            nbal_alloc_table = (int *)mp3dec_nbal_alloc_table2;
            sblimit = mp3dec_sblimit_table[1];
        }
    } else {
        return 0;
    }
    state->alloc_table = alloc_table;
    state->nbal_alloc_table = nbal_alloc_table;
    state->sblimit = sblimit;
    return 1;
}

/*****************************************************************************
//  Name:
//    mp3dec_GetSynch
//
//  Description:
//    check the MP3 frame header and get information if the header is valid
//
//  Input Arguments:
//    DC - point to sDecoderContext structure
//
//  Output Arguments:
//    DC - point to sDecoderContext structure
//
//  Returns:
//     1 - if the header is valid and supported
//    -1 - if synchronization is lost
//    -2 - if it is the end of stream
//    -4 - if the header is not supported
//
******************************************************************************/

MP3Status  mp3dec_GetSynch(MP3Dec_com *state)
{
    int     MP3Header;
    unsigned int val, val_t;
    int     start_db = state->decodedBytes;
    IppMP3FrameHeader *header = &(state->header);
    IppMP3FrameHeader *header_good = &(state->header_good);
    sBitsreamBuffer *BS = &state->m_StreamData;
    Ipp8u *ptrStart = (Ipp8u *)BS->pCurrent_dword + ((32 - BS->nBit_offset) >> 3);
    int buflen = BS->nDataLen;
    int cont_flag;

    if (buflen <= (BS->pCurrent_dword - BS->pBuffer) * 4 || buflen < 4)
        return MP3_NOT_FIND_SYNCWORD;     // end of file

    GET_BITS(BS, val, 24);
    state->decodedBytes += 3;

    do {
      cont_flag = 0;
      for(;;) {
        val <<= 8;
        GET_BITS(BS, val_t, 8);
        val |= val_t;
        state->decodedBytes++;
        if (((val >> 20) & SYNC_WORD) == SYNC_WORD) {
          MP3Header = val;
          header->layer = HDR_LAYER(MP3Header);
          header->samplingFreq = HDR_SAMPLINGFREQ(MP3Header);
          header->bitRate = HDR_BITRADEINDEX(MP3Header);
          header->emphasis = HDR_EMPH(MP3Header);
          header->id = HDR_VERSION(MP3Header);
          state->mpg25 = ((val >> 20) & 1) ? 0 : 2;

          if (state->m_bInit) {
            if ((header->samplingFreq == header_good->samplingFreq) &&
              (header->layer == header_good->layer) &&
              (header->id == header_good->id) &&
              (state->mpg25 == state->mpg25_good) &&
              (header->bitRate != 15) &&
              (header->emphasis != 2)) {
                break;
              }
          } else {
            if ((header->samplingFreq != 3) &&
              (header->layer != 4) &&
              (header->bitRate != 15) &&
              (header->emphasis != 2) &&
              (((val >> 19) & 3) != 1)) {
                break;
              }
          }
        }
        if (MP3DEC_END_OF_BUFFER(BS)) {
          state->decodedBytes -= 3;
          return MP3_NOT_FIND_SYNCWORD;
        }
      }

      header->mode = HDR_MODE(MP3Header);
      header->modeExt = HDR_MODEEXT(MP3Header);
      header->copyright = HDR_COPYRIGHT(MP3Header);
      header->originalCopy = HDR_ORIGINAL(MP3Header);
      header->paddingBit = HDR_PADDING(MP3Header);
      header->privateBit = HDR_EXTENSION(MP3Header);
      header->protectionBit = HDR_ERRPROTECTION(MP3Header);

      if (header->bitRate != 0) {
        int size = 0;

        if (header->layer == 3) {
          size = 72000 * (header->id + 1);
        } else if (header->layer == 2) {
          size = 72000 * 2;
        } else if (header->layer == 1) {
          size = 12000;
        }

        state->MP3nSlots =
          size * mp3dec_bitrate[header->id][header->layer - 1][header->bitRate] /
          mp3dec_frequency[header->id + state->mpg25][header->samplingFreq] + header->paddingBit;

        if (header->layer == 1)
          state->MP3nSlots *= 4;

        state->MP3nSlots -= 4; /* header */
        state->decodedBytes += state->MP3nSlots;
		
		if(0)//*** why do we even need this check??? bnie 1/29/07
        if (BS->nDataLen < state->decodedBytes - start_db +
          (state->m_bInit ? 0 : 2)) {
          state->decodedBytes -= state->MP3nSlots + 4;
          return MP3_NOT_ENOUGH_DATA;
        }
        if(0)//*** this check is causing it not being able to find a sync word bnie 2/6/07
		if (!state->m_bInit && (ptrStart[state->decodedBytes - start_db] != 0xff ||
          (ptrStart[state->decodedBytes - start_db] & 0xe0) != 0xe0)) {
            state->decodedBytes -= state->MP3nSlots;

            if (MP3DEC_END_OF_BUFFER(BS)) {
              state->decodedBytes -= 3;
              return MP3_NOT_FIND_SYNCWORD;
            }
            cont_flag = 1;
            continue;
          }
	  } else {
        unsigned int ubuf, usyncbuf, good;
        bs_save(BS);

        if (state->m_nBitrate != header->bitRate) { /* VBR is forbidden */
          state->decodedBytes -= 3;
          return MP3_BAD_STREAM;
        }

        usyncbuf = 0xffe000 | ((2 - state->mpg25) << 11) | (header->id << 11) | ((4 - header->layer) << 9) |
          (header->samplingFreq << 2);

        state->MP3nSlots = -2;
        ubuf = 0;
        good = 0;

        while (MP3DEC_END_OF_BUFFER(BS) != 1) {
          GET_BITS(BS, val, 8);
          ubuf = (ubuf << 8) | val;
          if ((ubuf & 0xfffefc) == usyncbuf)
          {
            good = 1;
            break;
          }
          state->MP3nSlots++;
        }

        bs_restore(BS);

        if (good == 0) {
          return MP3_NOT_ENOUGH_DATA;
        }

        state->decodedBytes += (state->MP3nSlots);
      }
    }while (cont_flag);

    state->m_nBitrate = header->bitRate;

    state->stereo = (header->mode == 0x3) ? 1 : 2;

    if (header->mode == 0x01) {
        state->intensity = 0;
        state->ms_stereo = 0;

        if (header->modeExt & 0x1)
            state->intensity = 1;

        if (header->modeExt & 0x2)
            state->ms_stereo = 1;

    }

    if (header->protectionBit == 0)
        GET_BITS(BS, val, 16);

    if (header->layer == 2) {
//        if (state->alloc_table == 0) {
            mp3dec_SetAllocTable(state);
//        }

        state->jsbound = 32;

        if (header->mode  == 0x01) {
            state->jsbound = (header->modeExt + 1) * 4;
        }
        if (state->jsbound > state->sblimit) {
            state->jsbound = state->sblimit;
        }
    } else if (header->layer == 1) {
        state->jsbound = 32;

        if (header->mode  == 0x01) {
            state->jsbound = (header->modeExt + 1) * 4;
        }
    }

    return MP3_OK;
}

/******************************************************************************
//  Name:
//    ReadMainData
//
//  Description:
//    Copy data form global buffer, which contains all streams, into special buffer,
//    which contains only main data (i.e. headers of frames & side information are excluded)
//
//  Input Arguments:
//    DC - point to sDecoderContext structure
//
//  Output Arguments:
//    MainData - special buffer
//
//  Returns:
//    1 - is not ok.
//    0 - all ok
//
******************************************************************************/
int mp3dec_ReadMainData(MP3Dec_com *state)
{
    unsigned char *GlBS;
    unsigned char *MDBS;
    int     nSlots;
    int     status = 0;

    sBitsreamBuffer *m_MainData = &state->m_MainData;
    sBitsreamBuffer *m_StreamData = &state->m_StreamData;

    // detect start point
    if ((int)(m_MainData->nDataLen - state->si_main_data_begin) >= 0)
    {
        m_MainData->pCurrent_dword =
            m_MainData->pBuffer + (m_MainData->nDataLen - state->si_main_data_begin) / 4;
        m_MainData->dword = BSWAP(m_MainData->pCurrent_dword[0]);
        m_MainData->nBit_offset =
            32 - ((m_MainData->nDataLen - state->si_main_data_begin) % 4) * 8;
    } else {
        status = 1;
    }

    // GIBS points to the start point of the undecoded main data in globalBuffer
    GlBS =
        (unsigned char *)(m_StreamData->pCurrent_dword) + (4 -
        m_StreamData->
        nBit_offset / 8);

    // nSlots = number of bytes between two successive frames (without header and
    // side info).
    nSlots = state->MP3nSlots + mp3dec_main_data_slots(state->stereo, state->header.id,
        state->header.protectionBit);

    // nSlots contains only main data cannot be bigger than the defined maximum
    // (MAINDATASIZE).
    if (nSlots > state->MAINDATASIZE) {
        return 1;
    }

    if (nSlots + m_MainData->nDataLen > state->MAINDATASIZE) {
        // bytes = number of bytes which have been received, but not decoded yet.
        int bytes;
        if (status == 0) {
            bytes =
                m_MainData->nDataLen - (m_MainData->pCurrent_dword -
                m_MainData->pBuffer) * 4;
            // copies the undecoded bytes (bytes beginning at pBuffer+iDWDoneNum)
            // to the start of the MainData buffer.
            ippsCopy_8u_x((unsigned char *)(m_MainData->pCurrent_dword),
                (unsigned char *)m_MainData->pBuffer, bytes);
        } else {
            bytes =
                m_MainData->nDataLen - (state->MAINDATASIZE - nSlots);
            ippsCopy_8u_x((unsigned char *)m_MainData->pBuffer +
                m_MainData->nDataLen - bytes,
                (unsigned char *)m_MainData->pBuffer, bytes);
        }

        // effectively shifts the undecoded bytes to the start of the MainData buffer
        m_MainData->pCurrent_dword = m_MainData->pBuffer;
        m_MainData->nDataLen = bytes;
    }
    // MDBS points to the first
    MDBS = (unsigned char *)m_MainData->pBuffer + m_MainData->nDataLen;
    ippsCopy_8u_x(GlBS, MDBS, nSlots);
    m_MainData->nDataLen += nSlots;

    return status;
}

MP3Status mp3dec_ReceiveBuffer(sBitsreamBuffer *m_StreamData, void *in_GetPointer, int in_GetDataSize)
{
    GET_INIT_BITSTREAM(m_StreamData, in_GetPointer)
    m_StreamData->nDataLen = in_GetDataSize;
    return MP3_OK;
}

MP3Status mp3dec_GetID3Len(Ipp8u *in,
                           int inDataSize,
                           MP3Dec_com *state)
{
  if ((state->m_bInit != 0) || (state->id3_size) > 0)
    return MP3_OK;

  if (inDataSize < 10)
    return MP3_NOT_ENOUGH_DATA;

  if ((in[0] == 'I') && (in[1] == 'D') && (in[2] == '3') && /* 'ID3' */
      (in[3] < 0xFF) && (in[4] < 0xFF) &&                  /* Version or revision will never be 0xFF */
      (in[6] < 0x80) && (in[7] < 0x80) && (in[8] < 0x80) && (in[9] < 0x80)) { /* size */
    state->id3_size = (in[6] << 21) + (in[7] << 14) + (in[8] << 7) + in[9] + 10;
  } else {
    state->id3_size = 0;
  }

  return MP3_OK;
}

MP3Status mp3dec_SkipID3(int inDataSize,
                         int *skipped,
                         MP3Dec_com *state)
{
  *skipped = 0;
  if (state->id3_size > 0) {
    if (inDataSize < state->id3_size) {
      *skipped = inDataSize;
      state->id3_size -= inDataSize;
      return MP3_NOT_ENOUGH_DATA;
    } else {
      *skipped = state->id3_size;
      state->id3_size = 0;
      return MP3_OK;
    }
  }
  return MP3_OK;
}
