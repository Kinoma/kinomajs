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
#include "kinoma_mpeg4_com_bitstream.h"

IppStatus (__STDCALL *ippiEncodeCoeffsIntra_H263_16s1u_universal) 	(Ipp16s* pQCoef, Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  advIntraFlag, int modQuantFlag, int scan)=NULL;
IppStatus (__STDCALL *ippiEncodeDCIntra_H263_16s1u_universal) 		(Ipp16s  qDC, Ipp8u** ppBitStream, int* pBitOffset)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsIntra_MPEG4_16s1u_universal) 	(const Ipp16s*  pCoeffs,  Ipp8u** ppBitStream, int* pBitOffset, int  countNonZero, int  rvlcFlag,int noDCFlag,int scan)=NULL;
IppStatus (__STDCALL *ippiEncodeDCIntra_MPEG4_16s1u_universal) 		(Ipp16s dcCoeff, Ipp8u**  ppBitStream,int*  pBitOffset, int blockType)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsInter_H263_16s1u_universal) 	(Ipp16s* pQCoef, Ipp8u** ppBitStream, int*  pBitOffset, int  countNonZero, int modQuantFlag, int  scan)=NULL;
IppStatus (__STDCALL *ippiEncodeCoeffsInter_MPEG4_16s1u_universal) 	(const Ipp16s*  pCoeffs,  Ipp8u**  ppBitStream, int* pBitOffset,int  countNonZero,int rvlcFlag,int scan)=NULL;


/* DC prediction sizes */
static Ipp32s DCtab_lum[13][2] =
{
  {3,3}, {3,2}, {2,2}, {2,3}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7},
  {1,8}, {1,9}, {1,10}, {1,11}
}; 

static Ipp32s DCtab_chrom[13][2] =
{
  {3,2}, {2,2}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8},
  {1,9}, {1,10}, {1,11}, {1,12}
}; 

/* DCT coefficients. Four tables, two for last = 0, two for last = 1.
   the sign bit must be added afterwards. */

/* first part of coeffs for last = 0. Indexed by [run][level-1] */

static Ipp32s coeff_tab0[2][12][2] =
{
  /* run = 0 */
  {
    {0x02, 2}, {0x0f, 4}, {0x15, 6}, {0x17, 7},
    {0x1f, 8}, {0x25, 9}, {0x24, 9}, {0x21,10},
    {0x20,10}, {0x07,11}, {0x06,11}, {0x20,11}
  },
  /* run = 1 */
  {
    {0x06, 3}, {0x14, 6}, {0x1e, 8}, {0x0f,10},
    {0x21,11}, {0x50,12}, {0x00, 0}, {0x00, 0},
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  }
};

/* rest of coeffs for last = 0. indexing by [run-2][level-1] */

static Ipp32s coeff_tab1[25][4][2] =
{
  /* run = 2 */
  {
    {0x0e, 4}, {0x1d, 8}, {0x0e,10}, {0x51,12}
  },
  /* run = 3 */
  {
    {0x0d, 5}, {0x23, 9}, {0x0d,10}, {0x00, 0}
  },
  /* run = 4-26 */
  {
    {0x0c, 5}, {0x22, 9}, {0x52,12}, {0x00, 0}
  },
  {
    {0x0b, 5}, {0x0c,10}, {0x53,12}, {0x00, 0}
  },
  {
    {0x13, 6}, {0x0b,10}, {0x54,12}, {0x00, 0}
  },
  {
    {0x12, 6}, {0x0a,10}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x11, 6}, {0x09,10}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x10, 6}, {0x08,10}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x16, 7}, {0x55,12}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x15, 7}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x14, 7}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1c, 8}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1b, 8}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x21, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x20, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1f, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1e, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1d, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1c, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1b, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x1a, 9}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x22,11}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x23,11}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x56,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  },
  {
    {0x57,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}
  }
};

/* first coeffs of last = 1. indexing by [run][level-1] */

static Ipp32s coeff_tab2[2][3][2] =
{
  /* run = 0 */
  {
    {0x07, 4}, {0x19, 9}, {0x05,11}
  },
  /* run = 1 */
  {
    {0x0f, 6}, {0x04,11}, {0x00, 0}
  }
};

/* rest of coeffs for last = 1. indexing by [run-2] */

static Ipp32s coeff_tab3[40][2] =
{
  {0x0e, 6}, {0x0d, 6}, {0x0c, 6},
  {0x13, 7}, {0x12, 7}, {0x11, 7}, {0x10, 7},
  {0x1a, 8}, {0x19, 8}, {0x18, 8}, {0x17, 8},
  {0x16, 8}, {0x15, 8}, {0x14, 8}, {0x13, 8},
  {0x18, 9}, {0x17, 9}, {0x16, 9}, {0x15, 9},    
  {0x14, 9}, {0x13, 9}, {0x12, 9}, {0x11, 9},    
  {0x07,10}, {0x06,10}, {0x05,10}, {0x04,10},    
  {0x24,11}, {0x25,11}, {0x26,11}, {0x27,11},    
  {0x58,12}, {0x59,12}, {0x5a,12}, {0x5b,12},    
  {0x5c,12}, {0x5d,12}, {0x5e,12}, {0x5f,12},
  {0x00, 0}               
};

/* New tables for Intra luminance coefficients. Same codewords,
   different meaning */

/* Coeffs for last = 0, run = 0. Indexed by [level-1] */

static Ipp32s coeff_tab4[27][2] =
{
    /* run = 0 */
    {0x02, 2}, {0x06, 3}, {0x0f, 4}, {0x0d, 5},
    {0x0c, 5}, {0x15, 6}, {0x13, 6}, {0x12, 6}, 
    {0x17, 7}, {0x1f, 8}, {0x1e, 8}, {0x1d, 8},
    {0x25, 9}, {0x24, 9}, {0x23, 9}, {0x21, 9},
    {0x21,10}, {0x20,10}, {0x0f,10}, {0x0e,10},
    {0x07,11}, {0x06,11}, {0x20,11}, {0x21,11},
    {0x50,12}, {0x51,12}, {0x52,12}
};

/* Coeffs for last = 0, run = 1. Indexed by [level-1] */

static Ipp32s coeff_tab5[10][2] =
{
    {0x0e, 4}, {0x14, 6}, {0x16, 7}, {0x1c, 8},
    {0x20, 9}, {0x1f, 9}, {0x0d,10}, {0x22,11},
    {0x53,12}, {0x55,12}
};

/* Coeffs for last = 0, run = 2 -> 9. Indexed by [run-2][level-1] */

static Ipp32s coeff_tab6[8][5][2] =
{
    /* run = 2 */
    {
	{0x0b, 5}, {0x15, 7}, {0x1e, 9}, {0x0c,10},
	{0x56,12}
    },
    /* run = 3 */
    {
	{0x11, 6}, {0x1b, 8}, {0x1d, 9}, {0x0b,10},
	{0x00, 0}
    },
    /* run = 4 */
    {
	{0x10, 6}, {0x22, 9}, {0x0a,10}, {0x00, 0},
	{0x00, 0}
    },
    /* run = 5 */
    {
	{0x0d, 6}, {0x1c, 9}, {0x08,10}, {0x00, 0},
	{0x00, 0}
    },
    /* run = 6 */
    {
	{0x12, 7}, {0x1b, 9}, {0x54,12}, {0x00, 0},
	{0x00, 0}
    },
    /* run = 7 */
    {
	{0x14, 7}, {0x1a, 9}, {0x57,12}, {0x00, 0},
	{0x00, 0}
    },
    /* run = 8 */
    {
	{0x19, 8}, {0x09,10}, {0x00, 0}, {0x00, 0},
	{0x00, 0}
    },
    /* run = 9 */
    {
	{0x18, 8}, {0x23,11}, {0x00, 0}, {0x00, 0},
	{0x00, 0}
    }
};

/* Coeffs for last = 0, run = 10 -> 14. Indexed by [run-10] */

static Ipp32s coeff_tab7[5][2] =
{
    {0x17, 8}, {0x19, 9}, {0x18, 9}, {0x07,10},
    {0x58,12}
};

/* Coeffs for last = 1, run = 0. Indexed by [level-1] */

static Ipp32s coeff_tab8[8][2] =
{
    {0x07, 4}, {0x0c, 6}, {0x16, 8}, {0x17, 9},
    {0x06,10}, {0x05,11}, {0x04,11}, {0x59,12}
};

/* Coeffs for last = 1, run = 1 -> 6. Indexed by [run-1][level-1] */

static Ipp32s coeff_tab9[6][3][2] =
{
    /* run = 1 */
    {
	{0x0f, 6}, {0x16, 9}, {0x05,10}
    },
    /* run = 2 */
    {
	{0x0e, 6}, {0x04,10}, {0x00, 0}
    },
    /* run = 3 */
    {
	{0x11, 7}, {0x24,11}, {0x00, 0}
    },
    /* run = 4 */
    {
	{0x10, 7}, {0x25,11}, {0x00, 0}
    },
    /* run = 5 */
    {
	{0x13, 7}, {0x5a,12}, {0x00, 0}
    },
    /* run = 6 */
    {
	{0x15, 8}, {0x5b,12}, {0x00, 0}
    }
};

/* Coeffs for last = 1, run = 7 -> 20. Indexed by [run-7] */

static Ipp32s coeff_tab10[14][2] =
{
    {0x14, 8}, {0x13, 8}, {0x1a, 8}, {0x15, 9},
    {0x14, 9}, {0x13, 9}, {0x12, 9}, {0x11, 9},
    {0x26,11}, {0x27,11}, {0x5c,12}, {0x5d,12},
    {0x5e,12}, {0x5f,12}
};



/* RVLC tables */
/* DCT coefficients. Four tables, two for last = 0, two for last = 1.
   the sign bit must be added afterwards. */

/* DCT  coeffs (intra) for last = 0.  */

/* Indexed by [level-1] */

static Ipp32s coeff_RVLCtab1[27][2] =
{
  /* run = 0 */
    {     0x6,  3},
    {     0x7,  3},
    {     0xa,  4},
    {     0x9,  5},
    {    0x14,  6},
    {    0x15,  6},
    {    0x34,  7},
    {    0x74,  8},
    {    0x75,  8},
    {    0xdd,  9},
    {    0xec,  9},
    {   0x1ec, 10},
    {   0x1ed, 10},
    {   0x1f4, 10},
    {   0x3ec, 11},
    {   0x3ed, 11},
    {   0x3f4, 11},
    {   0x77d, 12},
    {   0x7bc, 12},
    {   0xfbd, 13},
    {   0xfdc, 13},
    {   0x7bd, 12},
    {   0xfdd, 13},
    {  0x1fbd, 14},
    {  0x1fdc, 14},
    {  0x1fdd, 14},
    {  0x1ffc, 15}  
};


/* Indexed by [level-1] */

static Ipp32s coeff_RVLCtab2[13][2] =
{
  /* run = 1 */
    {     0x1,  4},
    {     0x8,  5},
    {    0x2d,  7},
    {    0x6c,  8},
    {    0x6d,  8},
    {    0xdc,  9},
    {   0x1dd, 10},
    {   0x3dc, 11},
    {   0x3dd, 11},
    {   0x77c, 12},
    {   0xfbc, 13},
    {  0x1f7d, 14},
    {  0x1fbc, 14}  
};


/* Indexed by [level-1] */

static Ipp32s coeff_RVLCtab3[11][2] =
{
  /* run = 2 */

    {     0x4,  5},
    {    0x2c,  7},
    {    0xbc,  9},
    {   0x1dc, 10},
    {   0x3bc, 11},
    {   0x3bd, 11},
    {   0xefd, 13},
    {   0xf7c, 13},
    {   0xf7d, 13},
    {  0x1efd, 14},
    {  0x1f7c, 14}  
};


/* Indexed by [level-1] */

static Ipp32s coeff_RVLCtab4[9][2] =
{
  /* run = 3 */
    {     0x5,  5},
    {    0x5c,  8},
    {    0xbd,  9},
    {   0x37d, 11},
    {   0x6fc, 12},
    {   0xefc, 13},
    {  0x1dfd, 14},
    {  0x1efc, 14},
    {  0x1ffd, 15}  
};


/* Indexed by [run-4][level-1] */

static Ipp32s coeff_RVLCtab5[2][6][2] =
{
  /* run = 4 */
  {
    {     0xc,  6},
    {    0x5d,  8},
    {   0x1bd, 10},
    {   0x3fd, 12},
    {   0x6fd, 12},
    {  0x1bfd, 14}  
  },
  /* run = 5 */
  {
    {     0xd,  6},
    {    0x7d,  9},
    {   0x2fc, 11},
    {   0x5fc, 12},
    {  0x1bfc, 14},
    {  0x1dfc, 14}  
  }
};


/* Indexed by [run-6][level-1]       */

static Ipp32s coeff_RVLCtab6[2][5][2] =
{

  /* run = 6 */
  {
    {    0x1c,  7},
    {   0x17c, 10},
    {   0x2fd, 11},
    {   0x5fd, 12},
    {  0x2ffc, 15}  
  },
  /* run = 7 */
  {
    {    0x1d,  7},
    {   0x17d, 10},
    {   0x37c, 11},
    {   0xdfd, 13},
    {  0x2ffd, 15}  
  }
 
};
/* Indexed by [run-8][level-1] */

static Ipp32s coeff_RVLCtab7[2][4][2] =
{
   /* run = 8 */
  {
    {    0x3c,  8},
    {   0x1bc, 10},
    {   0xbfd, 13},
    {  0x17fd, 14}  
  },
  /* run = 9 */
  {
    {    0x3d,  8},
    {   0x1fd, 11},
    {   0xdfc, 13},
    {  0x37fc, 15}, 
  }
};


/* Indexed by [run-10][level-1] */

static Ipp32s coeff_RVLCtab8[3][2][2] =
{
  /* run = 10 */
  {
    {    0x7c,  9},
    {   0x3fc, 12}  
  },
  /* run = 11 */
  {
    {    0xfc, 10},
    {   0xbfc, 13}  
  },
  /* run = 12 */
  {
    {    0xfd, 10},
    {  0x37fd, 15}  
  }
};


/* Indexed by [level-1] */

static Ipp32s coeff_RVLCtab9[7][2] =
{
  /* run = 13 -> 19 */
    {   0x1fc, 11},
    {   0x7fc, 13},
    {   0x7fd, 13},
    {   0xffc, 14},
    {   0xffd, 14},
    {  0x17fc, 14},
    {  0x3bfc, 15}  
};


/* first coeffs of last = 1. indexing by [run][level-1] */

static Ipp32s coeff_RVLCtab10[2][5][2] =
{
  /* run = 0 */
  {
    {     0xb,  4},
    {    0x78,  8},
    {   0x3f5, 11},
    {   0xfec, 13},
    {  0x1fec, 14}    
  },
  /* run = 1 */
  {
    {    0x12,  5},
    {    0xed,  9},
    {   0x7dc, 12},
    {  0x1fed, 14},
    {  0x3bfd, 15}    
  }

};

static Ipp32s coeff_RVLCtab11[3][2] =
{
  /* run = 2 */
  {    0x13,  5},
  {   0x3f8, 11},
  {  0x3dfc, 15}      
  
};

static Ipp32s coeff_RVLCtab12[11][2][2] =
{
  /* run = 3 */
  {
    {    0x18,  6},
    {   0x7dd, 12}    
  },
  /* run = 4 */
  {
    {    0x19,  6},
    {   0x7ec, 12}    
  },
  /* run = 5 */
  {
    {    0x22,  6},
    {   0xfed, 13}    
  },
  /* run = 6 */
  {
    {    0x23,  6},
    {   0xff4, 13}    
  },
  /* run = 7 */
  {
    {    0x35,  7},
    {   0xff5, 13}    
  },
  /* run = 8 */
  {
    {    0x38,  7},
    {   0xff8, 13}    
  },
  /* run = 9 */
  {
    {    0x39,  7},
    {   0xff9, 13}    
  },
  /* run = 10 */
  {
    {    0x42,  7},
    {  0x1ff4, 14}    
  },
  /* run = 11 */
  {
    {    0x43,  7},
    {  0x1ff5, 14}    
  },
  /* run = 12 */
  {
    {    0x79,  8},
    {  0x1ff8, 14}    
  },
  /* run = 13 */
  {
    {    0x82,  8},
    {  0x3dfd, 15}    
  }

};

static Ipp32s coeff_RVLCtab13[32][2] =
{
  /* run = 14 -> 45 */
    {    0x83,  8},
    {    0xf4,  9},
    {    0xf5,  9},
    {    0xf8,  9},
    {    0xf9,  9},
    {   0x102,  9},
    {   0x103,  9},
    {   0x1f5, 10},
    {   0x1f8, 10},
    {   0x1f9, 10},
    {   0x202, 10},
    {   0x203, 10},
    {   0x3f9, 11},
    {   0x402, 11},
    {   0x403, 11},
    {   0x7ed, 12},
    {   0x7f4, 12},
    {   0x7f5, 12},
    {   0x7f8, 12},
    {   0x7f9, 12},
    {   0x802, 12},
    {   0x803, 12},
    {  0x1002, 13},
    {  0x1003, 13},
    {  0x1ff9, 14},
    {  0x2002, 14},
    {  0x2003, 14},
    {  0x3efc, 15},
    {  0x3efd, 15},
    {  0x3f7c, 15},
    {  0x3f7d, 15}    
};


/* Coeffs for last = 0, run = 0. Indexed by [level-1] */

static Ipp32s coeff_RVLCtab14[19][2] =
{
  /* run = 0 */
    {     0x6,  3},
    {     0x1,  4},
    {     0x4,  5},
    {    0x1c,  7},
    {    0x3c,  8},
    {    0x3d,  8},
    {    0x7c,  9},
    {    0xfc, 10},
    {    0xfd, 10},
    {   0x1fc, 11},
    {   0x1fd, 11},
    {   0x3fc, 12},
    {   0x7fc, 13},
    {   0x7fd, 13},
    {   0xbfc, 13},
    {   0xbfd, 13},
    {   0xffc, 14},
    {   0xffd, 14},
    {  0x1ffc, 15}    
};

static Ipp32s coeff_RVLCtab15[10][2] =
{
  /* run = 1 */
    {     0x7,  3},
    {     0xc,  6},
    {    0x5c,  8},
    {    0x7d,  9},
    {   0x17c, 10},
    {   0x2fc, 11},
    {   0x3fd, 12},
    {   0xdfc, 13},
    {  0x17fc, 14},
    {  0x17fd, 14}    
};

static Ipp32s coeff_RVLCtab16[2][7][2] =
{
  /* run = 2 */
  {
    {     0xa,  4},
    {    0x1d,  7},
    {    0xbc,  9},
    {   0x2fd, 11},
    {   0x5fc, 12},
    {  0x1bfc, 14},
    {  0x1bfd, 14}    
  },
  /* run = 3 */
  {
    {     0x5,  5},
    {    0x5d,  8},
    {   0x17d, 10},
    {   0x5fd, 12},
    {   0xdfd, 13},
    {  0x1dfc, 14},
    {  0x1ffd, 15}    
  }
};

static Ipp32s coeff_RVLCtab17[5][2] =
{
  /* run = 4 */
    {     0x8,  5},
    {    0x6c,  8},
    {   0x37c, 11},
    {   0xefc, 13},
    {  0x2ffc, 15}    
};

static Ipp32s coeff_RVLCtab18[3][4][2] =
{
  /* run = 5 */
  {
    {     0x9,  5},
    {    0xbd,  9},
    {   0x37d, 11},
    {   0xefd, 13}    
  },
  /* run = 6 */
  {
    {     0xd,  6},
    {   0x1bc, 10},
    {   0x6fc, 12},
    {  0x1dfd, 14}    
  },
  /* run = 7 */
  {
    {    0x14,  6},
    {   0x1bd, 10},
    {   0x6fd, 12},
    {  0x2ffd, 15}    
  }
};

static Ipp32s coeff_RVLCtab19[2][3][2] =
{
  /* run = 8 */
  {
    {    0x15,  6},
    {   0x1dc, 10},
    {   0xf7c, 13}    
  },
  /* run = 9 */
  {
    {    0x2c,  7},
    {   0x1dd, 10},
    {  0x1efc, 14}    
  }
};

static Ipp32s coeff_RVLCtab20[8][2][2] =
{
  /* run = 10 */
  {
    {    0x2d,  7},
    {   0x3bc, 11}    
  },
  /* run = 11 */
  {
    {    0x34,  7},
    {   0x77c, 12}    
  },
  /* run = 12 */
  {
    {    0x6d,  8},
    {   0xf7d, 13}    
  },
  /* run = 13 */
  {
    {    0x74,  8},
    {  0x1efd, 14}    
  },
  /* run = 14 */
  {
    {    0x75,  8},
    {  0x1f7c, 14}    
  },
  /* run = 15 */
  {
    {    0xdc,  9},
    {  0x1f7d, 14}    
  },
  /* run = 16 */
  {
    {    0xdd,  9},
    {  0x1fbc, 14}    
  },
  /* run = 17 */
  {
    {    0xec,  9},
    {  0x37fc, 15}    
  }
};

static Ipp32s coeff_RVLCtab21[21][2] =
{
  /* run = 18 -> 38 */
    {   0x1ec, 10},
    {   0x1ed, 10},
    {   0x1f4, 10},
    {   0x3bd, 11},
    {   0x3dc, 11},
    {   0x3dd, 11},
    {   0x3ec, 11},
    {   0x3ed, 11},
    {   0x3f4, 11},
    {   0x77d, 12},
    {   0x7bc, 12},
    {   0x7bd, 12},
    {   0xfbc, 13},
    {   0xfbd, 13},
    {   0xfdc, 13},
    {   0xfdd, 13},
    {  0x1fbd, 14},
    {  0x1fdc, 14},
    {  0x1fdd, 14},
    {  0x37fd, 15},
    {  0x3bfc, 15}    
};


/* first coeffs of last = 1. indexing by [run][level-1] */

static Ipp32s coeff_RVLCtab22[2][5][2] =
{
  /* run = 0 */
  {
    {     0xb,  4},
    {    0x78,  8},
    {   0x3f5, 11},
    {   0xfec, 13},
    {  0x1fec, 14}    
  },
  /* run = 1 */
  {
    {    0x12,  5},
    {    0xed,  9},
    {   0x7dc, 12},
    {  0x1fed, 14},
    {  0x3bfd, 15}    
  }

};

static Ipp32s coeff_RVLCtab23[3][2] =
{
  /* run = 2 */
  {    0x13,  5},
  {   0x3f8, 11},
  {  0x3dfc, 15}      
  
};

static Ipp32s coeff_RVLCtab24[11][2][2] =
{
  /* run = 3 */
  {
    {    0x18,  6},
    {   0x7dd, 12}    
  },
  /* run = 4 */
  {
    {    0x19,  6},
    {   0x7ec, 12}    
  },
  /* run = 5 */
  {
    {    0x22,  6},
    {   0xfed, 13}    
  },
  /* run = 6 */
  {
    {    0x23,  6},
    {   0xff4, 13}    
  },
  /* run = 7 */
  {
    {    0x35,  7},
    {   0xff5, 13}    
  },
  /* run = 8 */
  {
    {    0x38,  7},
    {   0xff8, 13}    
  },
  /* run = 9 */
  {
    {    0x39,  7},
    {   0xff9, 13}    
  },
  /* run = 10 */
  {
    {    0x42,  7},
    {  0x1ff4, 14}    
  },
  /* run = 11 */
  {
    {    0x43,  7},
    {  0x1ff5, 14}    
  },
  /* run = 12 */
  {
    {    0x79,  8},
    {  0x1ff8, 14}    
  },
  /* run = 13 */
  {
    {    0x82,  8},
    {  0x3dfd, 15}    
  }

};

static Ipp32s coeff_RVLCtab25[32][2] =
{
  /* run = 14 -> 45 */
    {    0x83,  8},
    {    0xf4,  9},
    {    0xf5,  9},
    {    0xf8,  9},
    {    0xf9,  9},
    {   0x102,  9},
    {   0x103,  9},
    {   0x1f5, 10},
    {   0x1f8, 10},
    {   0x1f9, 10},
    {   0x202, 10},
    {   0x203, 10},
    {   0x3f9, 11},
    {   0x402, 11},
    {   0x403, 11},
    {   0x7ed, 12},
    {   0x7f4, 12},
    {   0x7f5, 12},
    {   0x7f8, 12},
    {   0x7f9, 12},
    {   0x802, 12},
    {   0x803, 12},
    {  0x1002, 13},
    {  0x1003, 13},
    {  0x1ff9, 14},
    {  0x2002, 14},
    {  0x2003, 14},
    {  0x3efc, 15},
    {  0x3efd, 15},
    {  0x3f7c, 15},
    {  0x3f7d, 15}    
};




/* -------------------------------------- vlc about functions, modified from the VM's same name functions -------------------------------*/

int PutCoeff_InterL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run < 2 && level < 13 ) 
	{
		length = coeff_tab0[run][level-1][1];
		k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab0[run][level-1][0],	length);
	}
	else if (run > 1 && run < 27 && level < 5) 
	{
		length = coeff_tab1[run-2][level-1][1];
		k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab1[run-2][level-1][0],length);
	}
	return length;
}

int PutCoeff_InterL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run < 2 && level < 4) 
	{
		length = coeff_tab2[run][level-1][1];
		k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab2[run][level-1][0],length);
	}
	else if (run > 1 && run < 42 && level == 1) 
	{
		length = coeff_tab3[run-2][1];
		k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab3[run-2][0],length);
	}
	return length;
}



int PutCoeff_IntraL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run == 0 && level < 28 ) 
	{
		length = coeff_tab4[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab4[level-1][0],length);
	}
	else if (run == 1 && level < 11) 
	{
		length = coeff_tab5[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab5[level-1][0],length);
	}
	else if (run > 1 && run < 10 && level < 6) 
	{
		length = coeff_tab6[run-2][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab6[run-2][level-1][0],length);
	}
	else if (run > 9 && run < 15 && level == 1) 
	{
		length = coeff_tab7[run-10][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab7[run-10][0],length);
	}
	return length;
}

int PutCoeff_IntraL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run == 0 && level < 9) 
	{
		length = coeff_tab8[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab8[level-1][0],length);

	}
	else if (run > 0 && run < 7 && level < 4) 
	{
		length = coeff_tab9[run-1][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab9[run-1][level-1][0],length);

	}
	else if (run > 6 && run < 21 && level == 1) 
	{
		length = coeff_tab10[run-7][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab10[run-7][0],length);
	}
	return length;
}

  
int PutCoeff_Inter_RVLCL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run == 0 && level < 20 ) 
	{
		length = coeff_RVLCtab14[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab14[level-1][0],length);
	}
	else if (run == 1 && level < 11) 
	{
		length = coeff_RVLCtab15[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab15[level-1][0],length);
	}
	else if (run > 1 && run < 4 && level < 8) 
	{
		length = coeff_RVLCtab16[run-2][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab16[run-2][level-1][0],length);
	}
	else if (run == 4 && level < 6) 
	{
		length = coeff_RVLCtab17[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab17[level-1][0],length);
	}
	else if (run > 4 && run < 8 && level < 5)
	{
		length = coeff_RVLCtab18[run-5][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab18[run-5][level-1][0],length);
	}
	else if (run > 7 && run < 10 && level < 4) 
	{
		length = coeff_RVLCtab19[run-8][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab19[run-8][level-1][0],length);
	}
	else if (run > 9 && run < 18 && level < 3) 
	{
		length = coeff_RVLCtab20[run-10][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab20[run-10][level-1][0],length);
	}
	else if (run > 17 && run < 39 && level == 1) 
	{
		length = coeff_RVLCtab21[run-18][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab21[run-18][0],length);
	}   
	return length;
}

int PutCoeff_Inter_RVLCL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run >= 0 && run < 2 && level < 6) 
	{
		length = coeff_RVLCtab22[run][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab22[run][level-1][0],length);
	}
	else if (run == 2 && level < 4)
	{
		length = coeff_RVLCtab23[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab23[level-1][0],length);
	}
	else if (run > 2 && run < 14 && level < 3) 
	{
		length = coeff_RVLCtab24[run-3][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab24[run-3][level-1][0],length);
	}
	else if (run > 13 && run < 46 && level == 1)
	{
		length = coeff_RVLCtab25[run-14][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab25[run-14][0],length);
	}
	return length;
}

int PutCoeff_Intra_RVLCL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run == 0 && level < 28 )
	{
		length = coeff_RVLCtab1[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab1[level-1][0],length);
	}
	else if (run == 1 && level < 14) 
	{
		length = coeff_RVLCtab2[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab2[level-1][0],length);
	}
	else if (run == 2 && level < 12) 
	{
		length = coeff_RVLCtab3[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab3[level-1][0],length);
	}
	else if (run == 3 && level < 10) 
	{
		length = coeff_RVLCtab4[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab4[level-1][0],length);
	}
	else if (run > 3 && run < 6 && level < 7) 
	{
		length = coeff_RVLCtab5[run-4][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab5[run-4][level-1][0],length);
	}
	else if (run > 5 && run < 8 && level < 6) 
	{
		length = coeff_RVLCtab6[run-6][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab6[run-6][level-1][0],length);
	}
	else if (run > 7 && run < 10 && level < 5) 
	{
		length = coeff_RVLCtab7[run-8][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab7[run-8][level-1][0],length);
	}
	else if (run > 9 && run < 13 && level < 3) 
	{
		length = coeff_RVLCtab8[run-10][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab8[run-10][level-1][0],length);
	}
	else if (run > 12 && run < 20 && level == 1) 
	{
		length = coeff_RVLCtab9[run-13][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab9[run-13][0],length);

	}  
	return length;
}

int PutCoeff_Intra_RVLCL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run >= 0 && run < 2 && level < 6) 
	{
		length = coeff_RVLCtab10[run][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab10[run][level-1][0],length);
	}
	else if (run == 2 && level < 4) 
	{
		length = coeff_RVLCtab11[level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab11[level-1][0],length);
	}
	else if (run > 2 && run < 14 && level < 3) 
	{
		length = coeff_RVLCtab12[run-3][level-1][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab12[run-3][level-1][0],length);
	}
	else if (run > 13 && run < 46 && level == 1) 
	{
		length = coeff_RVLCtab13[run-14][1];
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_RVLCtab13[run-14][0],length);
	}    
	return length;
}

/* The following is for 3-mode VLC */
int PutRunCoeff_InterL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run < 2 && level < 13 )
	{
		length = coeff_tab0[run][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab0[run][level-1][0],length);
			length += 9; /* boon */
		}
	}
	else if (run > 1 && run < 27 && level < 5) 
	{
		length = coeff_tab1[run-2][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab1[run-2][level-1][0],length);
			length += 9; /* boon */
		}
	}
	return length;
}

int PutRunCoeff_InterL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run < 2 && level < 4)
	{
		length = coeff_tab2[run][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab2[run][level-1][0],length);
			length += 9; /* boon */
		}
	}
	else if (run > 1 && run < 42 && level == 1) 
	{
		length = coeff_tab3[run-2][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab3[run-2][0],length);
			length += 9; /* boon */
		}
	}
	return length;
}

int PutRunCoeff_IntraL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run == 0 && level < 28 ) 
	{
		length = coeff_tab4[level-1][1];
		if (length) {
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab4[level-1][0],length);
			length += 9; /* boon */
		}
	}
	else if (run == 1 && level < 11) 
	{
		length = coeff_tab5[level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab5[level-1][0],length);
			length += 9; /* boon */
		}      
	}
	else if (run > 1 && run < 10 && level < 6)
	{
		length = coeff_tab6[run-2][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab6[run-2][level-1][0],length);
			length += 9; /* boon */
		}
	}
	else if (run > 9 && run < 15 && level == 1) 
	{
		length = coeff_tab7[run-10][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab7[run-10][0],length);
			length += 9; /* boon */
		}
	}
	return length;
}

int PutRunCoeff_IntraL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	if (run == 0 && level < 9) 
	{
		length = coeff_tab8[level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab8[level-1][0],length);
			length += 9; /* boon */
		}
	}
	else if (run > 0 && run < 7 && level < 4)
	{
		length = coeff_tab9[run-1][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab9[run-1][level-1][0],length);
			length += 9; /* boon */
		}
	}
	else if (run > 6 && run < 21 && level == 1)
	{
		length = coeff_tab10[run-7][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, 2, 2);  /* boon 120697 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab10[run-7][0],length);
			length += 9; /* boon */
		}
	}
	return length;
}

int PutLevelCoeff_InterL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run < 2 && level < 13 ) 
	{
		length = coeff_tab0[run][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab0[run][level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run > 1 && run < 27 && level < 5) 
	{
		length = coeff_tab1[run-2][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab1[run-2][level-1][0],length);
			length += 8; /* boon */
		}
	}
	return length;
}

int PutLevelCoeff_InterL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run < 2 && level < 4) 
	{
		length = coeff_tab2[run][level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab2[run][level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run > 1 && run < 42 && level == 1)
	{
		length = coeff_tab3[run-2][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits (ppBitStream,pBitOffset, (Ipp16u)coeff_tab3[run-2][0],length);
			length += 8; /* boon */
		}
	}
	return length;
}

int PutLevelCoeff_IntraL0(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;

	//MOMCHECK (last >= 0 && last < 2);
	//MOMCHECK (run >= 0 && run < 64);
	//MOMCHECK (level > 0 && level < 128);
	if (run == 0 && level < 28 ) 
	{
		length = coeff_tab4[level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab4[level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run == 1 && level < 11)
	{
		length = coeff_tab5[level-1][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab5[level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run > 1 && run < 10 && level < 6) 
	{
		length = coeff_tab6[run-2][level-1][1];
		if (length)
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab6[run-2][level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run > 9 && run < 15 && level == 1) 
	{
		length = coeff_tab7[run-10][1];
		if (length) 
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab7[run-10][0],length);
			length += 8; /* boon */
		}
	}
	return length;
}

int PutLevelCoeff_IntraL1(Ipp8u** ppBitStream, int* pBitOffset,int run, int level)
{
	int length = 0;
	if (run == 0 && level < 9) 
	{
		length = coeff_tab8[level-1][1];
		if (length)
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab8[level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run > 0 && run < 7 && level < 4)
	{
		length = coeff_tab9[run-1][level-1][1];
		if (length)
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab9[run-1][level-1][0],length);
			length += 8; /* boon */
		}
	}
	else if (run > 6 && run < 21 && level == 1)
	{
		length = coeff_tab10[run-7][1];
		if (length)
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1); /* boon19970701 */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)coeff_tab10[run-7][0],length);
			length += 8; /* boon */
		}
	}
	return length;
}



IppStatus __STDCALL ippiEncodeDCIntra_H263_16s1u_c(Ipp16s  qDC, Ipp8u** ppBitStream, int* pBitOffset)
{
#if 1
	if (!ppBitStream)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	if (qDC != 128)
		k_mp4_PutBits(ppBitStream, pBitOffset,(Ipp16u)qDC,8);
	else
		k_mp4_PutBits(ppBitStream, pBitOffset,(Ipp16u)255,8);

#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeDCIntra_H263_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeDCIntra_H263_16s1u(qDC, ppBitStream, pBitOffset);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiEncodeCoeffsIntra_H263_16s1u_c(
	Ipp16s* pQCoef, Ipp8u** ppBitStream, int*    pBitOffset, int     countNonZero,
	int     advIntraFlag, int     modQuantFlag, int     scan)
{
#if 1
	int j, nzc, length,first;
	int prev_run, run, prev_level, level, prev_s, s;
	Ipp16s coeff[64];
	const Ipp32s *pZigzag = zigzag[scan+1];

	first = 1;
	nzc =  prev_run = run = prev_level = level =  prev_s = s = 0;

	if (!ppBitStream || !pQCoef)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	if (countNonZero < 1 || countNonZero > 64)
		return ippStsOutOfRangeErr;

	/* in this Intel sample, 
	the modQuantFlag, advIntraFlag are both set with 0 obviously.
	I think modQuantFlag and advIntraFlag are used in H.263, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag || advIntraFlag)
	{
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	for (j = 1; j < 64; j++)
	{
		coeff[j] = pQCoef[pZigzag[j]];
	}

	for (j = 1; nzc < countNonZero; j++)
	{
		s = 0;
		/* Increment run if coeff is zero */
		if ((level = coeff[j]) == 0)
		{
			run++;
		}
		else
		{
			nzc++;
			length = 0;
			/* code run & level and count bits */
			if (level < 0)
			{
				s = 1;
				level = -level;
			}

			if (!first)
			{
				/* Encode the previous ind */
				if ((prev_run < 64) && (prev_level < 13))
				{
					length = PutCoeff_InterL0(ppBitStream,pBitOffset, prev_run, prev_level);
				}
				//else
				//	length = 0;

				if (length == 0)
				{  /* short_video_header */
					if (prev_s == 1)
					{
						prev_level = (prev_level^0xfff)+1;
					}
					k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 
					k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6); /* run */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
				}
				else
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
				}
			}

			prev_run = run; 
			prev_level = level; 
			prev_s = s;
			run = first = 0;
		}
	}

	/* Encode the last coeff */
	if (!first)
	{
		length = 0;
		if ((prev_run < 64) && (prev_level < 4)) 
		{
			length = PutCoeff_InterL1(ppBitStream,pBitOffset,prev_run, prev_level);
		}
		//else
		//	length = 0;

		if (length == 0)
		{  /* short_video_header */
			if (prev_s == 1)
			{
				prev_level = (prev_level^0xfff)+1;
			}
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 
			k_mp4_PutBit1(ppBitStream,pBitOffset);	 /* last */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
		}
		else
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeCoeffsIntra_H263_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeCoeffsIntra_H263_16s1u(pQCoef, ppBitStream, pBitOffset,countNonZero,advIntraFlag, modQuantFlag, scan);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiEncodeCoeffsIntra_H263_16s1u_flv_c(
	Ipp16s* pQCoef, Ipp8u** ppBitStream, int*    pBitOffset, int     countNonZero_in,
	int     advIntraFlag, int     modQuantFlag, int     scan)
{
	int i, nzc, length,first;
	int prev_run, run, prev_level, level, prev_s, s;
	const Ipp32s *pZigzag = zigzag[scan+1];
	int last_index = countNonZero_in>>16;
	int countNonZero = countNonZero_in&0x0000ffff;
	int is_flv = 1;

	first = 1;
	nzc =  prev_run = run = prev_level = level =  prev_s = s = 0;

	if (!ppBitStream || !pQCoef)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	if (countNonZero < 1 || countNonZero > 64)
		return ippStsOutOfRangeErr;

	/* in this Intel sample, 
	the modQuantFlag, advIntraFlag are both set with 0 obviously.
	I think modQuantFlag and advIntraFlag are used in H.263, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag || advIntraFlag)
	{
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	for (i = 1; nzc < countNonZero; i++)
	{
		level = pQCoef[pZigzag[i]];
		s = 0;
		/* Increment run if coeff is zero */
		if (level == 0)
		{
			run++;
		}
		else
		{
			nzc++;
			length = 0;
			/* code run & level and count bits */
			if (level < 0)
			{
				s = 1;
				level = -level;
			}

			if (!first)
			{
				/* Encode the previous ind */
				if ((prev_run < 64) && (prev_level < 13))
				{
					length = PutCoeff_InterL0(ppBitStream,pBitOffset, prev_run, prev_level);
				}
				//else
				//	length = 0;

				if (length == 0)
				{  /* short_video_header */
					if (prev_s == 1)
					{
						prev_level = (prev_level^0xfff)+1;
					}
					k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); //???bnie: not sure about this

					if( is_flv )
					{
						if( prev_level < 64 )//7-bit level
						{
							k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1);
							k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 7);
						}				//11-bit level
						else
						{
							k_mp4_PutBits(ppBitStream,pBitOffset, 1, 1);
							k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 11);
						}
					}
					else
					{
						k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6); /* run */
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
					}
				}
				else
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
				}
			}

			prev_run = run; 
			prev_level = level; 
			prev_s = s;
			run = first = 0;
		}
	}

	/* Encode the last coeff */
	if (!first)
	{
		length = 0;
		if ((prev_run < 64) && (prev_level < 4)) 
		{
			length = PutCoeff_InterL1(ppBitStream,pBitOffset,prev_run, prev_level);
		}
		//else
		//	length = 0;

		if (length == 0)
		{  /* short_video_header */
			if (prev_s == 1)
			{
				prev_level = (prev_level^0xfff)+1;
			}
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); //???bnie: not sure about this

			if( is_flv )
			{
				if( prev_level < 64 )//7-bit level
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1);
					k_mp4_PutBit1(ppBitStream,pBitOffset); /* last */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 7);
				}				//11-bit level
				else
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, 1, 1);
					k_mp4_PutBit1(ppBitStream,pBitOffset); /* last */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 11);
				}
			}
			else
			{
				k_mp4_PutBit1(ppBitStream,pBitOffset); /* last */
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6); /* run */
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
			}
		}
		else
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
		}
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippiEncodeCoeffsInter_H263_16s1u_c(
	Ipp16s* pQCoef, Ipp8u** ppBitStream, int*    pBitOffset,
	int     countNonZero, int     modQuantFlag, int     scan)
{
#if 1
	int j, nzc, length,first;
	int prev_run, run, prev_level, level, prev_s, s;
	Ipp16s coeff[64];
	const Ipp32s        *pZigzag = zigzag[scan+1];

	first = 1;
	nzc =  prev_run = run = prev_level = level =  prev_s = s = 0;

	if (!ppBitStream || !pQCoef)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	if (countNonZero < 1 || countNonZero > 64)
		return ippStsOutOfRangeErr;

	/* in this Intel sample, 
	the modQuantFlag is set with 0 obviously.
	I think modQuantFlag are used in H.263, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag)
	{
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	for (j = 0; j < 64; j++)
	{
		coeff[j] = pQCoef[pZigzag[j]];
	}

	for (j = 0; nzc < countNonZero; j++)
	{
		s = 0;
		/* Increment run if coeff is zero */
		if ((level = coeff[j]) == 0)
		{
			run++;
		}
		else
		{
			nzc++;
			length = 0;
			/* code run & level and count bits */
			if (level < 0)
			{
				s = 1;
				level = -level;
			}

			if (!first)
			{
				/* Encode the previous ind */
				if ((prev_run < 64) && (prev_level < 13))
				{
					length = PutCoeff_InterL0(ppBitStream,pBitOffset, prev_run, prev_level);
				}
				//else
				//	length = 0;

				if (length == 0)
				{  /* short_video_header */
					if (prev_s == 1)
					{
						prev_level = (prev_level^0xfff)+1;
					}
					k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 
					k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6); /* run */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
				}
				else
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
				}
			}

			prev_run = run; 
			prev_level = level; 
			prev_s = s;
			run = first = 0;
		}
	}

	/* Encode the last coeff */
	if (!first)
	{
		length = 0;
		if ((prev_run < 64) && (prev_level < 4)) 
		{
			length = PutCoeff_InterL1(ppBitStream,pBitOffset,prev_run, prev_level);
		}
		//else
		//	length = 0;

		if (length == 0)
		{  /* short_video_header */
			if (prev_s == 1)
			{
				prev_level = (prev_level^0xfff)+1;
			}
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 
			k_mp4_PutBit1(ppBitStream,pBitOffset);	 /* last */
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
		}
		else
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeCoeffsInter_H263_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeCoeffsInter_H263_16s1u(pQCoef, ppBitStream, pBitOffset,countNonZero, modQuantFlag,scan);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiEncodeCoeffsInter_H263_16s1u_flv_c(
	Ipp16s* pQCoef, Ipp8u** ppBitStream, int*    pBitOffset,
	int     countNonZero, int     modQuantFlag, int     scan)
{
#if 1
	int j, nzc, length,first;
	int prev_run, run, prev_level, level, prev_s, s;
	Ipp16s coeff[64];
	const Ipp32s        *pZigzag = zigzag[scan+1];
	int is_flv = 1;

	first = 1;
	nzc =  prev_run = run = prev_level = level =  prev_s = s = 0;

	if (!ppBitStream || !pQCoef)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	if (countNonZero < 1 || countNonZero > 64)
		return ippStsOutOfRangeErr;

	/* in this Intel sample, 
	the modQuantFlag is set with 0 obviously.
	I think modQuantFlag are used in H.263, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag)
	{
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	for (j = 0; j < 64; j++)
	{
		coeff[j] = pQCoef[pZigzag[j]];
	}

	for (j = 0; nzc < countNonZero; j++)
	{
		s = 0;
		/* Increment run if coeff is zero */
		if ((level = coeff[j]) == 0)
		{
			run++;
		}
		else
		{
			nzc++;
			length = 0;
			/* code run & level and count bits */
			if (level < 0)
			{
				s = 1;
				level = -level;
			}

			if (!first)
			{
				/* Encode the previous ind */
				if ((prev_run < 64) && (prev_level < 13))
				{
					length = PutCoeff_InterL0(ppBitStream,pBitOffset, prev_run, prev_level);
				}
				//else
				//	length = 0;

				if (length == 0)
				{  /* short_video_header */
					if (prev_s == 1)
					{
						prev_level = (prev_level^0xfff)+1;
					}
					k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 

					if( is_flv )
					{
						if( level < 64 )//7-bit level
						{
							k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1);
							k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 7);
						}				//11-bit level
						else
						{
							k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1);
							k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
							k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 11);
						}
					}
					else
					{
						k_mp4_PutBit0(ppBitStream,pBitOffset); /* last */
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6); /* run */
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
					}
				}
				else
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
				}
			}

			prev_run = run; 
			prev_level = level; 
			prev_s = s;
			run = first = 0;
		}
	}

	/* Encode the last coeff */
	if (!first)
	{
		length = 0;
		if ((prev_run < 64) && (prev_level < 4)) 
		{
			length = PutCoeff_InterL1(ppBitStream,pBitOffset,prev_run, prev_level);
		}
		//else
		//	length = 0;

		if (length == 0)
		{  /* short_video_header */
			if (prev_s == 1)
			{
				prev_level = (prev_level^0xfff)+1;
			}
			k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7);
			if( is_flv )
			{
				if( level < 64 )//7-bit level
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1);
					k_mp4_PutBit1(ppBitStream,pBitOffset); /* last */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 7);
				}				//11-bit level
				else
				{
					k_mp4_PutBits(ppBitStream,pBitOffset, 0, 1);
					k_mp4_PutBit1(ppBitStream,pBitOffset); /* last */
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
					k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 11);
				}
			}
			else
			{
				k_mp4_PutBit1(ppBitStream,pBitOffset);	 /* last */
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 8); /* level */
			}
		}
		else
		{
			k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeCoeffsInter_H263_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeCoeffsInter_H263_16s1u(pQCoef, ppBitStream, pBitOffset,countNonZero, modQuantFlag,scan);
		return sts;
	}
#endif

	return ippStsNoErr;
}




IppStatus __STDCALL ippiEncodeDCIntra_MPEG4_16s1u_c(Ipp16s dcCoeff, Ipp8u**  ppBitStream,int*  pBitOffset, int blockType)
{
#if 1
	Ipp16s absVal;
	int size = 0;

	if (!ppBitStream)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	absVal = abs(dcCoeff);		/*(dcCoeff < 0)? -dcCoeff : dcCoeff; */

	while (absVal)
	{
		absVal >>= 1;
		size++;
	}

	if (IPPVC_BLOCK_LUMA == blockType)	/*put luma DC size */
	{
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)DCtab_lum[size][0], DCtab_lum[size][1]);
	}
	else		/*put chroma DC size */
	{
		k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)DCtab_chrom[size][0], DCtab_chrom[size][1]);
	}

	if (size)
	{
		if (dcCoeff < 0)
		{
			absVal = -dcCoeff;
			dcCoeff = (absVal ^ ((1 << size) - 1));//((int)pow(2.0,(double)size)-1));
		}

		k_mp4_PutBits(ppBitStream, pBitOffset,(Ipp16u)dcCoeff,size);

		if (size > 8)
			k_mp4_PutBit1(ppBitStream, pBitOffset);
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeDCIntra_MPEG4_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeDCIntra_MPEG4_16s1u(dcCoeff, ppBitStream, pBitOffset, blockType);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiEncodeCoeffsIntra_MPEG4_16s1u_c(
	const Ipp16s*  pCoeffs,  Ipp8u** ppBitStream, int* pBitOffset,
	int  countNonZero, int  rvlcFlag,int noDCFlag,int scan)
{
#if 1
	int j, nzc, length,first;
	int prev_run, run, prev_level, level, prev_s, s;
	Ipp16s coeff[64];
	const Ipp32s *pZigzag = zigzag[scan+1];

	if (!ppBitStream || !pCoeffs)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	first = 1;
	nzc =  prev_run = run = prev_level = level =  prev_s = s = 0;

	for (j = noDCFlag; j < 64; j++)
	{
		coeff[j] = pCoeffs[pZigzag[j]];
	}

	if (!rvlcFlag)
	{
		for (j = noDCFlag; nzc < countNonZero; j++)
		{
			s = 0;
			/* Increment run if coeff is zero */
			if ((level = coeff[j]) == 0)
			{
				run++;
			}
			else
			{
				length = 0;
				nzc ++;
				/* code run & level and count bits */
				if (level < 0)
				{
					s = 1;
					level = -level;
				}
				if (!first)
				{
					/* Encode the previous ind */
					if ((prev_run < 64) && (prev_level < 28))
					{
						length = PutCoeff_IntraL0(ppBitStream, pBitOffset,prev_run, prev_level);
					}
					/* First escape mode. Level offset */
					if (length == 0)
					{
						if ( prev_run < 64 ) 
						{
							/* subtraction of Max level, last = 0 */
							int level_minus_max;
							level_minus_max = prev_level - intra_max_level[0][prev_run];
							if (level_minus_max < 28)  
							{
								length = PutLevelCoeff_IntraL0(ppBitStream, pBitOffset,prev_run, level_minus_max);
							} 
						}
					}
					/* Second escape mode. Run offset */
					if (length == 0) 
					{ 
						if (prev_level < 28) 
						{
							/* subtraction of Max Run, last = 0 */
							int run_minus_max;
#ifdef __DEBUG
							if (prev_level == 0) 
							{
								printf ("ERROR(CodeCoeff-second esc): level is %d\n", prev_level);
								exit(-1);
							}
#endif
							run_minus_max = prev_run - (intra_max_run0[prev_level]+1);
							if (run_minus_max < 64)  /* boon 120697 */
							{
								length = PutRunCoeff_IntraL0(ppBitStream, pBitOffset,run_minus_max, prev_level);
							} 
						} 
					}
					/* Third escape mode. FLC */
					if (length == 0)
					{  /* Escape coding */
						if (prev_s == 1)
						{
							/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
							/* prev_level = (prev_level^0xff)+1; */
							prev_level = (prev_level^0xfff)+1;
						}
						k_mp4_PutBits(ppBitStream, pBitOffset, 3, 7); 
						k_mp4_PutBits(ppBitStream, pBitOffset, 3, 2); /* boon */

						k_mp4_PutBit0(ppBitStream, pBitOffset); /* last */
						k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_run), 6); /* run */

						/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
						k_mp4_PutBit1(ppBitStream, pBitOffset);	/*marker_bit*/
						/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
						k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_level), 12); /* level */
						/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
						k_mp4_PutBit1(ppBitStream, pBitOffset);	/*marker_bit*/
					}
					else
					{
						k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_s), 1);
					}
				}
				prev_run = run; 
				prev_level = level;
				prev_s = s;
				run = first = 0;
			}
		}

		/* Encode the last coeff */
		if (!first)
		{
			length = 0;
			if ((prev_run < 64) && (prev_level < 9))
			{
				/* Separate tables for Intra luminance blocks */
				length = PutCoeff_IntraL1(ppBitStream, pBitOffset,prev_run, prev_level);
			}
			/* First escape mode. Level offset */
			if (length == 0)
			{
				if ( prev_run < 64 )
				{
					/* subtraction of Max level, last = 0 */
					int level_minus_max;
					level_minus_max = prev_level - intra_max_level[1][prev_run];
					if (level_minus_max < 9)
					{
						length = PutLevelCoeff_IntraL1(ppBitStream, pBitOffset,prev_run, level_minus_max);
					} 
				}
			}
			/* Second escape mode. Run offset */
			if (length == 0) { 
				if (prev_level < 9) 
				{
					/* subtraction of Max Run, last = 1 */
					int run_minus_max;
#ifdef __DEBUG
					if (prev_level == 0) 
					{
						printf ( "ERROR(CodeCoeff-second esc): level is %d\n", prev_level);
						exit(-1);
					}
#endif
					run_minus_max = prev_run - (intra_max_run1[prev_level]+1);

					if (run_minus_max < 64)  /* boon 120697 */
					{
						length = PutRunCoeff_IntraL1(ppBitStream, pBitOffset,run_minus_max, prev_level);
					} 
				} 
			}
			/* Third escape mode. FLC */
			if (length == 0)
			{  /* Escape coding */
				if (prev_s == 1)
				{
					/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
					/* prev_level = (prev_level^0xff)+1; */
					prev_level = (prev_level^0xfff)+1;
				}
				k_mp4_PutBits(ppBitStream, pBitOffset, 3, 7); 
				k_mp4_PutBits(ppBitStream, pBitOffset, 3, 2); /* boon */

				k_mp4_PutBit1(ppBitStream, pBitOffset); /* last */
				k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_run), 6);

				/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_PutBit1(ppBitStream, pBitOffset);
				/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
				k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_level), 12); /* level */
				/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_PutBit1(ppBitStream, pBitOffset);
			}
			else
			{
				k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_s), 1);
			}
		}
	}
	else	/* rvlc */
	{
		for (j = noDCFlag; nzc < countNonZero; j++)
		{
			s = 0;
			/* Increment run if coeff is zero */
			if ((level = coeff[j]) == 0)
			{
				run++;
			}
			else
			{
				length = 0;
				nzc++;
				/* code run & level and count bits */
				if (level < 0)
				{
					s = 1;
					level = -level;
				}
				if (!first)
				{
					/* Encode the previous ind */
					if (prev_level  < 28 && prev_run < 39) 
					{
						length = PutCoeff_Intra_RVLCL0(ppBitStream, pBitOffset,prev_run, prev_level);

					}
					if (length == 0)
					{  /* Escape coding */
						k_mp4_PutBits(ppBitStream, pBitOffset, 1, 5);   	/* ESCAPE */
						k_mp4_PutBit0(ppBitStream, pBitOffset);   	/* LAST   */
						k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_run), 6); /* RUN 	  */

						/* 11.08.98 Sven Brandau:  "changed length for LEVEL (11 bit)" due to N2339, Clause 2.1.21 */
						/* 11.08.98 Sven Brandau:  "insert marker_bit befor and after LEVEL" due to N2339, Clause 2.1.21 */
						k_mp4_PutBit1(ppBitStream, pBitOffset);	/*marker_bit*/
						k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_level), 11); /* LEVEL */
						k_mp4_PutBit1(ppBitStream, pBitOffset); /*marker_bit*/

						k_mp4_PutBits(ppBitStream, pBitOffset, 0, 4);   	/* ESCAPE */

						k_mp4_PutBits(ppBitStream, pBitOffset,(Ipp16u)(prev_s),1);	/* ESCAPE's */
					}
					else
					{
						k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_s), 1);
					}
				}

				prev_run = run; 
				prev_level = level; 
				prev_s = s;
				run = first = 0;
			}
		}

		/* Encode the last coeff */
		if (!first)
		{
			length = 0;
			/* if (prev_level  < 5 && prev_run < 45) {  Bugfix provided by Luis Ducla-Soares 10/22/99 */
			if (prev_level  < 6 && prev_run < 45) 
			{
				length = PutCoeff_Intra_RVLCL1(ppBitStream, pBitOffset,prev_run, prev_level);
			}

			if (length == 0)
			{  /* Escape coding */
				k_mp4_PutBits(ppBitStream, pBitOffset, 1, 5); 			/* ESCAPE	*/
				k_mp4_PutBit1(ppBitStream, pBitOffset);			/* LAST		*/
				k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_run), 6);		/* RUN		*/

				/* 11.08.98 Sven Brandau:  "changed length for LEVEL (11 bit)" due to N2339, Clause 2.1.21	     */
				/* 11.08.98 Sven Brandau:  "insert marker_bit befor and after LEVEL" due to N2339, Clause 2.1.21 */
				k_mp4_PutBit1( ppBitStream, pBitOffset);	/*marker_bit*/
				k_mp4_PutBits( ppBitStream, pBitOffset, (Ipp16u)(prev_level), 11); 	/* LEVEL 	*/
				k_mp4_PutBit1( ppBitStream, pBitOffset);	/*marker_bit*/

				k_mp4_PutBits(ppBitStream, pBitOffset, 0, 4);  			/* ESCAPE 	*/

				k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_s), 1);		/* ESCAPE's 	*/
			}
			else
			{
				k_mp4_PutBits(ppBitStream, pBitOffset, (Ipp16u)(prev_s), 1);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeCoeffsIntra_MPEG4_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeCoeffsIntra_MPEG4_16s1u(pCoeffs, ppBitStream, pBitOffset,countNonZero, rvlcFlag,noDCFlag,scan);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiEncodeCoeffsInter_MPEG4_16s1u_c(
	const Ipp16s*  pCoeffs,  Ipp8u**  ppBitStream, int* pBitOffset,
	int  countNonZero,int rvlcFlag,int scan)
{
#if 1
	int j, nzc, length,first;
	int prev_run, run, prev_level, level, prev_s, s;
	Ipp16s coeff[64];
	const Ipp32s        *pZigzag = zigzag[scan+1];

	if (!ppBitStream || !pCoeffs)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	first = 1;
	nzc =  prev_run = run = prev_level = level =  prev_s = s = 0;

	for (j = 0; j < 64; j++)
	{
		coeff[j] = pCoeffs[pZigzag[j]];
	}

	if (!rvlcFlag)
	{
		for (j = 0; nzc < countNonZero; j++)
		{
			/* encode AC coeff */
			s = 0;
			/* Increment run if coeff is zero */
			if ((level = coeff[j]) == 0)
			{
				run++;
			}
			else
			{
				length = 0;
				nzc++;
				/* code run & level and count bits */
				if (level < 0)
				{
					s = 1;
					level = -level;
				}
				if (!first)
				{
					/* Encode the previous ind */
					if ((prev_run < 64) && (prev_level < 13))
					{
						length = PutCoeff_InterL0(ppBitStream,pBitOffset, prev_run, prev_level);
					}
					/* First escape mode. Level offset */
					if (length == 0)
					{
						if ( prev_run < 64 ) 
						{
							/* subtraction of Max level, last = 0 */
							int level_minus_max;
							level_minus_max = prev_level - inter_max_level[0][prev_run];
							if (level_minus_max < 13)
							{
								length = PutLevelCoeff_InterL0(ppBitStream,pBitOffset, prev_run, level_minus_max);
							} 
						}
					}
					/* Second escape mode. Run offset */
					if (length == 0) 
					{ 
						if (prev_level < 13)
						{
							/* subtraction of Max Run, last = 0 */
							int run_minus_max;
#ifdef __DEBUG
							if (prev_level == 0) 
							{
								printf ("ERROR(CodeInterCoeff-second esc): level is %d\n", prev_level);
								exit(-1);
							}
#endif
							run_minus_max = prev_run - (inter_max_run0[prev_level]+1);
							if (run_minus_max < 64)  /* boon 120697 */
							{
								length = PutRunCoeff_InterL0(ppBitStream,pBitOffset, run_minus_max, prev_level);
							}
						} 
					}
					/* Third escape mode. FLC */
					if (length == 0)
					{  /* Escape coding */
						if (prev_s == 1)
						{
							/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
							/* prev_level = (prev_level^0xff)+1; */
							prev_level = (prev_level^0xfff)+1;
						}
						k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 
						k_mp4_PutBits(ppBitStream,pBitOffset, 3, 2); /* boon */

						k_mp4_PutBit0(ppBitStream,pBitOffset);	 /* last */
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6); /* run */

						/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
						k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/
						/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 12); /* level */
						/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
						k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/
					}
					else
					{
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
					}
				}
				prev_run = run; 
				prev_level = level;
				prev_s = s;
				run = first = 0;
			}
		}

		/* Encode the last coeff */
		if (!first)
		{
			length = 0;
			if ((prev_run < 64) && (prev_level < 4)) 
			{
				length = PutCoeff_InterL1(ppBitStream,pBitOffset,prev_run, prev_level);
			}
			/* First escape mode. Level offset */
			if (length == 0) {
				if ( prev_run < 64 ) 
				{
					/* subtraction of Max level, last = 0 */
					int level_minus_max;
					level_minus_max = prev_level - inter_max_level[1][prev_run];
					if (level_minus_max < 4)
					{
						length = PutLevelCoeff_InterL1(ppBitStream,pBitOffset,prev_run, level_minus_max);
					} 
				}
			}
			/* Second escape mode. Run offset */
			if (length == 0) 
			{ 
				if (prev_level < 4) 
				{
					/* subtraction of Max Run, last = 1 */
					int run_minus_max;
#ifdef __DEBUG
					if (prev_level == 0) 
					{
						printf ("ERROR(CodeInterCoeff-second esc): level is %d\n", prev_level);
						exit(-1);
					}
#endif
					run_minus_max = prev_run - (inter_max_run1[prev_level]+1);
					if (run_minus_max < 64)  /* boon 120697 */
					{
						length = PutRunCoeff_InterL1(ppBitStream,pBitOffset,run_minus_max, prev_level);
					}
				} 
			}
			/* Third escape mode. FLC */
			if (length == 0)
			{  /* Escape coding */
				if (prev_s == 1)
				{
					/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
					/* prev_level = (prev_level^0xff)+1; */
					prev_level = (prev_level^0xfff)+1;
				}
				k_mp4_PutBits(ppBitStream,pBitOffset, 3, 7); 
				k_mp4_PutBits(ppBitStream,pBitOffset, 3, 2); /* boon */

				k_mp4_PutBit1(ppBitStream,pBitOffset);	 /* last */

				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);

				/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/
				/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_level), 12); /* level */
				/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/
			}
			else
			{
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
			}
		}
	}
	else	/* rvlc */
	{
		for (j = 0; nzc < countNonZero; j++)
		{
			s = 0;

			/* Increment run if coeff is zero */
			if ((level = coeff[j]) == 0)
			{
				run++;
			}
			else
			{
				length = 0;
				nzc++;
				/* code run & level and count bits */
				if (level < 0)
				{
					s = 1;
					level = -level;
				}
				if (!first)
				{
					/* Encode the previous ind */
					if (prev_level  < 28 && prev_run < 39) 
					{
						length = PutCoeff_Inter_RVLCL0(ppBitStream,pBitOffset,prev_run, prev_level);
					} 
					if (length == 0)
					{  /* Escape coding */
						k_mp4_PutBits(ppBitStream,pBitOffset, 1, 5);   	/* ESCAPE */
						k_mp4_PutBit0(ppBitStream,pBitOffset);   	/* LAST   */
						k_mp4_PutBits(ppBitStream,pBitOffset,(Ipp16u)(prev_run), 6); /* RUN 	  */

						/* 11.08.98 Sven Brandau:  "changed length for LEVEL (11 bit)" due to N2339, Clause 2.1.21 */
						/* 11.08.98 Sven Brandau:  "insert marker_bit befor and after LEVEL" due to N2339, Clause 2.1.21 */
						k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/
						k_mp4_PutBits( ppBitStream,pBitOffset, (Ipp16u)(prev_level), 11); /* LEVEL */
						k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/

						k_mp4_PutBits(ppBitStream,pBitOffset, 0, 4);   	/* ESCAPE */
						k_mp4_PutBits(ppBitStream,pBitOffset,(Ipp16u)(prev_s),1);	/* ESCAPE's */
					}
					else
					{
						k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
					}
				}
				prev_run = run;
				prev_level = level; 
				prev_s = s;
				run = first = 0;
			}
		}

		/* Encode the last coeff */
		if (!first)
		{
			length = 0;
			/* if (prev_level  < 5 && prev_run < 45) {  Bugfix provided by Luis Ducla-Soares 10/22/99 */
			if (prev_level  < 6 && prev_run < 45) 
			{
				length = PutCoeff_Inter_RVLCL1(ppBitStream,pBitOffset,prev_run, (Ipp16u)prev_level);
			}

			if (length == 0)
			{  /* Escape coding */
				k_mp4_PutBits(ppBitStream,pBitOffset, 1, 5); 			/* ESCAPE	*/

				k_mp4_PutBit1(ppBitStream,pBitOffset);		/* LAST		*/
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_run), 6);		/* RUN		*/

				/* 11.08.98 Sven Brandau:  "changed length for LEVEL (11 bit)" due to N2339, Clause 2.1.21	     */
				/* 11.08.98 Sven Brandau:  "insert marker_bit befor and after LEVEL" due to N2339, Clause 2.1.21 */
				k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/
				k_mp4_PutBits( ppBitStream,pBitOffset, (Ipp16u)(prev_level), 11); 	/* LEVEL 	*/
				k_mp4_PutBit1(ppBitStream,pBitOffset);	/*marker_bit*/

				k_mp4_PutBits(ppBitStream,pBitOffset, 0, 4);  			/* ESCAPE 	*/

				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);		/* ESCAPE's 	*/
			}
			else
			{
				k_mp4_PutBits(ppBitStream,pBitOffset, (Ipp16u)(prev_s), 1);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiEncodeCoeffsInter_MPEG4_16s1u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiEncodeCoeffsInter_MPEG4_16s1u(pCoeffs,  ppBitStream, pBitOffset,countNonZero,rvlcFlag,scan);
		return sts;
	}
#endif

	return ippStsNoErr;
}
