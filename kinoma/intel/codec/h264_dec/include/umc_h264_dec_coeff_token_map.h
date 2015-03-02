/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/
#ifndef __UMC_H264_DEC_COEFF_TOKEN_MAP_H__
#define __UMC_H264_DEC_COEFF_TOKEN_MAP_H__

namespace UMC
{

//#undef OFF
//
//#undef SZCF
//
//#undef SHIFT1
//
//#undef SHIFT2
//
//#undef TABLE_TYPE
//
//#if defined (ARM) || defined (_ARM_)
//
//#define OFF 60//120//
//#define SHIFT1 11//16//
//#define SHIFT2 5//8//
//#define SZCF 2//4//
//#define TABLE_TYPE unsigned short //int //
//
//#else   //  defined (ARM) || defined (_ARM_)
//
//#define OFF 120//60//
//#define SHIFT1 16//11//
//#define SHIFT2 8//5//
//#define SZCF 4//2//
//#define TABLE_TYPE int //unsigned short
//
//#endif   //  defined (ARM) || defined (_ARM_)
//
//#if !defined (ARM) && !defined (_ARM_)
#ifdef __INTEL_IPP__
//Table 9 5 - coeff_token mapping to TotalCoeff( coeff_token )
// and TrailingOnes( coeff_token ), 0  <=  nC  <  2, sorted by code len

/*
0    0    1

1    1    01

2    2    001

3    3    0001 1

3    4    0000 11
1    2    0001 00
0    1    0001 01

3    5    0000 100
2    3    0000 101

3    6    0000 0100
2    4    0000 0101
1    3    0000 0110
0    2    0000 0111

3    7    0000 0010 0
2    5    0000 0010 1
1    4    0000 0011 0
0    3    0000 0011 1

3    8    0000 0001 00
2    6    0000 0001 01
1    5    0000 0001 10
0    4    0000 0001 11

3    9    0000 0000 100
2    7    0000 0000 101
1    6    0000 0000 110
0    5    0000 0000 111

0    8    0000 0000 0100 0
2    9    0000 0000 0100 1
1    8    0000 0000 0101 0
0    7    0000 0000 0101 1
3    10    0000 0000 0110 0
2    8    0000 0000 0110 1
1    7    0000 0000 0111 0
0    6    0000 0000 0111 1

3    12    0000 0000 0010 00
2    11    0000 0000 0010 01
1    10    0000 0000 0010 10
0    10    0000 0000 0010 11
3    11    0000 0000 0011 00
2    10    0000 0000 0011 01
1    9    0000 0000 0011 10
0    9    0000 0000 0011 11

1    13    0000 0000 0000 001
3    14    0000 0000 0001 000
2    13    0000 0000 0001 001
1    12    0000 0000 0001 010
0    12    0000 0000 0001 011
3    13    0000 0000 0001 100
2    12    0000 0000 0001 101
1    11    0000 0000 0001 110
0    11    0000 0000 0001 111

0    16    0000 0000 0000 0100
2    16    0000 0000 0000 0101
1    16    0000 0000 0000 0110
0    15    0000 0000 0000 0111
3    16    0000 0000 0000 1000
2    15    0000 0000 0000 1001
1    15    0000 0000 0000 1010
0    14    0000 0000 0000 1011
3    15    0000 0000 0000 1100
2    14    0000 0000 0000 1101
1    14    0000 0000 0000 1110
0    13    0000 0000 0000 1111

*/

static
Ipp32s coeff_token_map_02[] =
{
16, /* max bits */
4,  /* total subtables */
3,3,3,7,/* subtable sizes */


1, /* 1-bit codes */
0x0001, 0, 0,

1, /* 2-bit codes */
0x0001, 1, 1,

1, /* 3-bit codes */
0x0001, 2, 2,

0, /* 4-bit codes */

1, /* 5-bit codes */
0x0003, 3, 3,

3, /* 6-bit codes */
0x0003, 3, 4, 0x0004, 1, 2, 0x0005, 0, 1,

2, /* 7-bit codes */
0x0004, 3, 5, 0x0005, 2, 3,

4, /* 8-bit codes */
0x0004, 3, 6, 0x0005, 2, 4, 0x0006, 1, 3, 0x0007, 0, 2,

4, /* 9-bit codes */
0x0004, 3, 7, 0x0005, 2, 5, 0x0006, 1, 4, 0x0007, 0, 3,

4, /* 10-bit codes */
0x0004, 3, 8, 0x0005, 2, 6, 0x0006, 1, 5, 0x0007, 0, 4,

4, /* 11-bit codes */
0x0004, 3, 9, 0x0005, 2, 7, 0x0006, 1, 6, 0x0007, 0, 5,

0, /* 12-bit codes */

8, /* 13-bit codes */
0x0008, 0, 8, 0x0009, 2, 9, 0x000a, 1, 8, 0x000b, 0, 7,
0x000c, 3, 10,0x000d, 2, 8, 0x000e, 1, 7, 0x000f, 0, 6,

8, /* 14-bit codes */
0x0008, 3, 12, 0x0009, 2, 11, 0x000a, 1, 10, 0x000b, 0, 10,
0x000c, 3, 11, 0x000d, 2, 10, 0x000e, 1, 9, 0x000f, 0, 9,

9, /* 15-bit codes */
0x0001, 1, 13, 0x0008, 3, 14, 0x0009, 2, 13, 0x000a, 1, 12,
0x000b, 0, 12, 0x000c, 3, 13, 0x000d, 2, 12, 0x000e, 1, 11,
0x000f, 0, 11,

12, /* 16-bit codes */
0x0004, 0, 16, 0x0005, 2, 16, 0x0006, 1, 16, 0x0007, 0, 15,
0x0008, 3, 16, 0x0009, 2, 15, 0x000a, 1, 15, 0x000b, 0, 14,
0x000c, 3, 15, 0x000d, 2, 14, 0x000e, 1, 14, 0x000f, 0, 13,

-1
};
/*
#undef OFF
#define OFF 120//60//

#undef SZCF
#define SZCF 4//2//

#undef SHIFT1
#define SHIFT1 16//11//

#undef SHIFT2
#define SHIFT2 8//5//

#undef TABLE_TYPE
#define TABLE_TYPE int //unsigned short
*/



//Table 9 5 - coeff_token mapping to TotalCoeff( coeff_token )
// and TrailingOnes( coeff_token ), 2  <=  nC  <  4, sorted by code len
/*
1    1    10
0    0    11

2    2    011

3    4    0100
3    3    0101

3    5    0011 0
1    2    0011 1

3    7    0001 00
2    4    0001 01
1    4    0001 10
0    2    0001 11
3    6    0010 00
2    3    0010 01
1    3    0010 10
0    1    0010 11

3    8    0000 100
2    5    0000 101
1    5    0000 110
0    3    0000 111

0    5    0000 0100
2    6    0000 0101
1    6    0000 0110
0    4    0000 0111

3    9    0000 0010 0
2    7    0000 0010 1
1    7    0000 0011 0
0    6    0000 0011 1

3    11    0000 0001 000
2    9    0000 0001 001
1    9    0000 0001 010
0    8    0000 0001 011
3    10    0000 0001 100
2    8    0000 0001 101
1    8    0000 0001 110
0    7    0000 0001 111

0    11    0000 0000 1000
2    11    0000 0000 1001
1    11    0000 0000 1010
0    10    0000 0000 1011
3    12    0000 0000 1100
2    10    0000 0000 1101
1    10    0000 0000 1110
0    9    0000 0000 1111

3    15    0000 0000 0000 1
2    14    0000 0000 0011 0
0    14    0000 0000 0011 1
3    14    0000 0000 0100 0
2    13    0000 0000 0100 1
1    13    0000 0000 0101 0
0    13    0000 0000 0101 1
3    13    0000 0000 0110 0
2    12    0000 0000 0110 1
1    12    0000 0000 0111 0
0    12    0000 0000 0111 1

3    16    0000 0000 0001 00
2    16    0000 0000 0001 01
1    16    0000 0000 0001 10
0    16    0000 0000 0001 11
1    15    0000 0000 0010 00
0    15    0000 0000 0010 01
2    15    0000 0000 0010 10
1    14    0000 0000 0010 11
*/

static
Ipp32s coeff_token_map_24[] =
{

14, /* max bits */
3,  /* total subtables */
3,4,7,/* subtable sizes */

0, /* 1-bit codes */

2, /* 2-bit codes */
0x0002, 1, 1, 0x0003, 0, 0,

1, /* 3-bit codes */
0x0003, 2, 2,

2, /* 4-bit codes */
0x0004, 3, 4, 0x0005, 3, 3,

2, /* 5-bit codes */
0x0006, 3, 5, 0x0007, 1, 2,

8, /* 6-bit codes */
0x0004, 3, 7, 0x0005, 2, 4, 0x0006, 1, 4, 0x0007, 0, 2,
0x0008, 3, 6, 0x0009, 2, 3, 0x000a, 1, 3, 0x000b, 0, 1,

4, /* 7-bit codes */
0x0004, 3, 8, 0x0005, 2, 5, 0x0006, 1, 5, 0x0007, 0, 3,

4, /* 8-bit codes */
0x0004, 0, 5, 0x0005, 2, 6, 0x0006, 1, 6, 0x0007, 0, 4,

4, /* 9-bit codes */
0x0004, 3, 9, 0x0005, 2, 7, 0x0006, 1, 7, 0x0007, 0, 6,

0, /* 10-bit codes */

8, /* 11-bit codes */
0x0008, 3, 11, 0x0009, 2, 9, 0x000a, 1, 9,  0x000b, 0, 8,
0x000c, 3, 10, 0x000d, 2, 8, 0x000e, 1, 8,  0x000f, 0, 7,

8, /* 12-bit codes */
0x0008, 0, 11, 0x0009, 2, 11, 0x000a, 1, 11, 0x000b, 0, 10,
0x000c, 3, 12, 0x000d, 2, 10, 0x000e, 1, 10, 0x000f, 0, 9,

11, /* 13-bit codes */
0x0001, 3, 15, 0x0006, 2, 14, 0x0007, 0, 14, 0x0008, 3, 14,
0x0009, 2, 13, 0x000a, 1, 13, 0x000b, 0, 13, 0x000c, 3, 13,
0x000d, 2, 12, 0x000e, 1, 12, 0x000f, 0, 12,

8, /* 14-bit codes */
0x0004, 3, 16, 0x0005, 2, 16, 0x0006, 1, 16, 0x0007, 0, 16,
0x0008, 1, 15, 0x0009, 0, 15, 0x000a, 2, 15, 0x000b, 1, 14,

-1
};


//Table 9 5 - coeff_token mapping to TotalCoeff( coeff_token )
// and TrailingOnes( coeff_token ), 4  <=  nC  <  8, sorted by code len
/*
3    7    1000
3    6    1001
3    5    1010
3    4    1011
3    3    1100
2    2    1101
1    1    1110
0    0    1111

1    5    0100 0
2    5    0100 1
1    4    0101 0
2    4    0101 1
1    3    0110 0
3    8    0110 1
2    3    0111 0
1    2    0111 1

0    3    0010 00
2    7    0010 01
1    7    0010 10
0    2    0010 11
3    9    0011 00
2    6    0011 01
1    6    0011 10
0    1    0011 11

0    7    0001 000
0    6    0001 001
2    9    0001 010
0    5    0001 011
3    10    0001 100
2    8    0001 101
1    8    0001 110
0    4    0001 111

3    12    0000 1000
2    11    0000 1001
1    10    0000 1010
0    9    0000 1011
3    11    0000 1100
2    10    0000 1101
1    9    0000 1110
0    8    0000 1111

1    13    0000 0011 1
0    12    0000 0100 0
2    13    0000 0100 1
1    12    0000 0101 0
0    11    0000 0101 1
3    13    0000 0110 0
2    12    0000 0110 1
1    11    0000 0111 0
0    10    0000 0111 1

0    16    0000 0000 01
3    16    0000 0000 10
2    16    0000 0000 11
1    16    0000 0001 00
0    15    0000 0001 01
3    15    0000 0001 10
2    15    0000 0001 11
1    15    0000 0010 00
0    14    0000 0010 01
3    14    0000 0010 10
2    14    0000 0010 11
1    14    0000 0011 00
0    13    0000 0011 01
*/

static
Ipp32s coeff_token_map_48[] =
{

10, /* max bits */
3,  /* total subtables */
4,3,3,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */
0, /* 3-bit codes */
8, /* 4-bit codes */
0x0008, 3, 7, 0x0009, 3, 6, 0x000a, 3, 5, 0x000b, 3, 4,
0x000c, 3, 3, 0x000d, 2, 2, 0x000e, 1, 1, 0x000f, 0, 0,

8, /* 5-bit codes */
0x0008, 1, 5, 0x0009, 2, 5, 0x000a, 1, 4, 0x000b, 2, 4,
0x000c, 1, 3, 0x000d, 3, 8, 0x000e, 2, 3, 0x000f, 1, 2,

8, /* 6-bit codes */
0x0008, 0, 3, 0x0009, 2, 7, 0x000a, 1, 7, 0x000b, 0, 2,
0x000c, 3, 9, 0x000d, 2, 6, 0x000e, 1, 6, 0x000f, 0, 1,

8, /* 7-bit codes */
0x0008, 0, 7, 0x0009, 0, 6, 0x000a, 2, 9, 0x000b, 0, 5,
0x000c, 3, 10,0x000d, 2, 8, 0x000e, 1, 8, 0x000f, 0, 4,

8, /* 8-bit codes */
0x0008, 3, 12, 0x0009, 2, 11, 0x000a, 1, 10, 0x000b, 0, 9,
0x000c, 3, 11, 0x000d, 2, 10, 0x000e, 1, 9,  0x000f, 0, 8,

9, /* 9-bit codes */
0x0007, 1, 13, 0x0008, 0, 12, 0x0009, 2, 13, 0x000a, 1, 12,
0x000b, 0, 11, 0x000c, 3, 13, 0x000d, 2, 12, 0x000e, 1, 11,
0x000f, 0, 10,

13, /* 10-bit codes */
0x0001, 0, 16, 0x0002, 3, 16, 0x0003, 2, 16, 0x0004, 1, 16,
0x0005, 0, 15, 0x0006, 3, 15, 0x0007, 2, 15, 0x0008, 1, 15,
0x0009, 0, 14, 0x000a, 3, 14, 0x000b, 2, 14, 0x000c, 1, 14,
0x000d, 0, 13,

-1
};


//Table 9 5 - coeff_token mapping to TotalCoeff( coeff_token )
// and TrailingOnes( coeff_token ),  nC  = -1, sorted by code len
/*
1    1    1

0    0    01

2    2    001

0    4    0000 10
0    3    0000 11
0    2    0001 00
3    3    0001 01
1    2    0001 10
0    1    0001 11

3    4    0000 000
2    3    0000 010
1    3    0000 011

2    4    0000 0010
1    4    0000 0011
*/

static
Ipp32s coeff_token_map_cr[] =
{

8, /* max bits */
2,  /* total subtables */
6,2,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 1, 1,

1, /* 2-bit codes */
0x0001, 0, 0,

1, /* 3-bit codes */
0x0001, 2, 2,

0, /* 4-bit codes */
0, /* 5-bit codes */

6, /* 6-bit codes */
0x0002, 0, 4, 0x0003, 0, 3, 0x0004, 0, 2, 0x0005, 3, 3,
0x0006, 1, 2, 0x0007, 0, 1,

3, /* 7-bit codes */
0x0000, 3, 4, 0x0002, 2, 3, 0x0003, 1, 3,

2, /* 8-bit codes */
0x0002, 2, 4, 0x0003, 1, 4,

-1
};

static
Ipp32s coeff_token_map_cr2[] =
{

    13, /* max bits */
    2,  /* total subtables */
    7,6,/* subtable sizes */

    1, /* 1-bit codes */
    0x0001, 0, 0,

    1, /* 2-bit codes */
    0x0001, 1, 1,

    1, /* 3-bit codes */
    0x0001, 2, 2,

    0, /* 4-bit codes */
    1, /* 5-bit codes */
    0x0001, 3, 3,

    1, /* 6-bit codes */
    0x0001, 3, 4,

    8, /* 7-bit codes */
    0x0008, 3, 6, 0x0009, 3, 5, 0x000A, 2, 4,
    0x000B, 2, 3, 0x000C, 1, 3, 0x000D, 1, 2,
    0x000E, 0, 2, 0x000F, 0, 1,

    0, /* 8-bit codes */
    4, /* 9-bit codes */
    0x0004, 2, 5, 0x0005, 1, 4, 0x0006, 0, 4,
    0x0007, 0, 3,
    4, /* 10-bit codes */
    0x0004, 3, 7, 0x0005, 2, 6, 0x0006, 1, 5,
    0x0007, 0, 5,
    4, /* 11-bit codes */
    0x0004, 3, 8, 0x0005, 2, 7, 0x0006, 1, 6,
    0x0007, 0, 6,
    4, /* 12-bit codes */
    0x0004, 2, 8, 0x0005, 1, 8, 0x0006, 1, 7,
    0x0007, 0, 7,
    1, /* 13-bit codes */
    0x0007, 0, 8,

    -1
};
#endif
//#else
//
////The table coeff_token_map_cr for XScale
//static TABLE_TYPE coeff_token_map_crs2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|32,//1
//    (OFF+2*SZCF  << 8)|32,//2
//    (OFF+3*SZCF  << 8)|30,//3
//    (OFF+7*SZCF  << 8)|31,//4
//    (OFF+9*SZCF  << 8)|31,//5
//    (OFF+11*SZCF  << 8)|31,//6
//    (OFF+13*SZCF  << 8)|32,//7
//    (OFF+13*SZCF  << 8)|32,//8
//    (OFF+13*SZCF  << 8)|32,//9
//    (OFF+13*SZCF  << 8)|32,//10
//    (OFF+13*SZCF  << 8)|32,//11
//    (OFF+13*SZCF  << 8)|32,//12
//    (OFF+13*SZCF  << 8)|32,//13
//    (OFF+13*SZCF  << 8)|32,//14
//    (OFF+13*SZCF  << 8)|32,//15
//    (OFF+13*SZCF  << 8)|32,//16
//    (OFF+13*SZCF  << 8)|32,//17
//    (OFF+13*SZCF  << 8)|32,//18
//    (OFF+13*SZCF  << 8)|32,//19
//    (OFF+13*SZCF  << 8)|32,//20
//    (OFF+13*SZCF  << 8)|32,//21
//    (OFF+13*SZCF  << 8)|32,//22
//    (OFF+13*SZCF  << 8)|32,//23
//    (OFF+13*SZCF  << 8)|32,//24
//    (OFF+13*SZCF  << 8)|32,//25
//    (OFF+13*SZCF  << 8)|32,//26
//    (OFF+13*SZCF  << 8)|32,//27
//    (OFF+13*SZCF  << 8)|32,//28
//    (OFF+13*SZCF  << 8)|32,//29
//
//    ( 1 << SHIFT1)|(1 << SHIFT2)| 31,     //0 //80b
//
//    ( 0 << SHIFT1)|(0 << SHIFT2)| 30,     //1 //84
//
//    ( 2 << SHIFT1)|(2 << SHIFT2)| 29,     //2 //88
//
//    ( 0 << SHIFT1)|(2 << SHIFT2)| 26, ( 3 << SHIFT1)|(3 << SHIFT2)| 26, ( 1 << SHIFT1)|(2 << SHIFT2)| 26, ( 0 << SHIFT1)|(1 << SHIFT2)| 26,    //3 //92
//
//    ( 0 << SHIFT1)|(4 << SHIFT2)| 26, ( 0 << SHIFT1)|(3 << SHIFT2)| 26,     //4 //108
//
//    ( 2 << SHIFT1)|(3 << SHIFT2)| 25, ( 1 << SHIFT1)|(3 << SHIFT2)| 25,     //5 //116
//
//    ( 2 << SHIFT1)|(4 << SHIFT2)| 24, ( 1 << SHIFT1)|(4 << SHIFT2)| 24,     //6 //124
//
//    ( 3 << SHIFT1)|(4 << SHIFT2)| 25,     //7..19 //132
//
//};
//
////The table coeff_token_map_02 for XScale
//static TABLE_TYPE coeff_token_map_02s2[] =
//{
//    (OFF+0          << 8)|32,//0
//    (OFF+1*SZCF   << 8)|32,//1
//    (OFF+2*SZCF   << 8)|32,//2
//    (OFF+3*SZCF   << 8)|30,//3
//    (OFF+7*SZCF   << 8)|30,//4
//    (OFF+11*SZCF  << 8)|30,//5
//    (OFF+15*SZCF  << 8)|30,//6
//    (OFF+19*SZCF  << 8)|30,//7
//    (OFF+23*SZCF  << 8)|30,//8
//    (OFF+27*SZCF  << 8)|29,//9
//    (OFF+35*SZCF  << 8)|29,//10
//    (OFF+43*SZCF  << 8)|29,//11
//    (OFF+51*SZCF  << 8)|29,//12
//    (OFF+59*SZCF  << 8)|30,//13
//    (OFF+63*SZCF  << 8)|32,//14
//    (OFF+64*SZCF  << 8)|32,//15
//    (OFF+64*SZCF  << 8)|32,//16
//    (OFF+64*SZCF  << 8)|32,//17
//    (OFF+64*SZCF  << 8)|32,//18
//    (OFF+64*SZCF  << 8)|32,//19
//    (OFF+64*SZCF  << 8)|32,//20
//    (OFF+64*SZCF  << 8)|32,//21
//    (OFF+64*SZCF  << 8)|32,//22
//    (OFF+64*SZCF  << 8)|32,//23
//    (OFF+64*SZCF  << 8)|32,//24
//    (OFF+64*SZCF  << 8)|32,//25
//    (OFF+64*SZCF  << 8)|32,//26
//    (OFF+64*SZCF  << 8)|32,//27
//    (OFF+64*SZCF  << 8)|32,//28
//    (OFF+64*SZCF  << 8)|32,//29
//
//    ( 0 << SHIFT1)|(0 << SHIFT2)| 31,  //0    //80b offset
//    ( 1 << SHIFT1)|(1 << SHIFT2)| 30,  //1    //84b
//    ( 2 << SHIFT1)|(2 << SHIFT2)| 29,  //2    //88b
//    ( 1 << SHIFT1)|(2 << SHIFT2)| 26, ( 0 << SHIFT1)|(1 << SHIFT2)| 26, ( 3 << SHIFT1)|(3 << SHIFT2)| 27, ( 3 << SHIFT1)|(3 << SHIFT2)| 27,//3    //92b
//    ( 3 << SHIFT1)|(5 << SHIFT2)| 25, ( 2 << SHIFT1)|(3 << SHIFT2)| 25, ( 3 << SHIFT1)|(4 << SHIFT2)| 26, ( 3 << SHIFT1)|(4 << SHIFT2)| 26,//4    //108b
//    ( 3 << SHIFT1)|(6 << SHIFT2)| 24, ( 2 << SHIFT1)|(4 << SHIFT2)| 24, ( 1 << SHIFT1)|(3 << SHIFT2)| 24, ( 0 << SHIFT1)|(2 << SHIFT2)| 24,//5    //124b
//    ( 3 << SHIFT1)|(7 << SHIFT2)| 23, ( 2 << SHIFT1)|(5 << SHIFT2)| 23, ( 1 << SHIFT1)|(4 << SHIFT2)| 23, ( 0 << SHIFT1)|(3 << SHIFT2)| 23,//6    //140b
//    ( 3 << SHIFT1)|(8 << SHIFT2)| 22, ( 2 << SHIFT1)|(6 << SHIFT2)| 22, ( 1 << SHIFT1)|(5 << SHIFT2)| 22, ( 0 << SHIFT1)|(4 << SHIFT2)| 22,//7    //156b
//    ( 3 << SHIFT1)|(9 << SHIFT2)| 21, ( 2 << SHIFT1)|(7 << SHIFT2)| 21, ( 1 << SHIFT1)|(6 << SHIFT2)| 21, ( 0 << SHIFT1)|(5 << SHIFT2)| 21,//8    //172
//    ( 0 << SHIFT1)|(8 << SHIFT2)| 19, ( 2 << SHIFT1)|(9 << SHIFT2)| 19, ( 1 << SHIFT1)|(8 << SHIFT2)| 19, ( 0 << SHIFT1)|(7 << SHIFT2)| 19,//9    //188b
//    ( 3 << SHIFT1)|(10<< SHIFT2)| 19, ( 2 << SHIFT1)|(8 << SHIFT2)| 19, ( 1 << SHIFT1)|(7 << SHIFT2)| 19, ( 0 << SHIFT1)|(6 << SHIFT2)| 19,
//    ( 3 << SHIFT1)|(12<< SHIFT2)| 18, ( 2 << SHIFT1)|(11<< SHIFT2)| 18, ( 1 << SHIFT1)|(10<< SHIFT2)| 18, ( 0 << SHIFT1)|(10<< SHIFT2)| 18,//10    //220b
//    ( 3 << SHIFT1)|(11<< SHIFT2)| 18, ( 2 << SHIFT1)|(10<< SHIFT2)| 18, ( 1 << SHIFT1)|(9 << SHIFT2)| 18, ( 0 << SHIFT1)|(9 << SHIFT2)| 18,
//    ( 3 << SHIFT1)|(14<< SHIFT2)| 17, ( 2 << SHIFT1)|(13<< SHIFT2)| 17, ( 1 << SHIFT1)|(12<< SHIFT2)| 17, ( 0 << SHIFT1)|(12<< SHIFT2)| 17,//11    //252b
//    ( 3 << SHIFT1)|(13<< SHIFT2)| 17, ( 2 << SHIFT1)|(12<< SHIFT2)| 17, ( 1 << SHIFT1)|(11<< SHIFT2)| 17, ( 0 << SHIFT1)|(11<< SHIFT2)| 17,
//    ( 3 << SHIFT1)|(16<< SHIFT2)| 16, ( 2 << SHIFT1)|(15<< SHIFT2)| 16, ( 1 << SHIFT1)|(15<< SHIFT2)| 16, ( 0 << SHIFT1)|(14<< SHIFT2)| 16,//12    //284b
//    ( 3 << SHIFT1)|(15<< SHIFT2)| 16, ( 2 << SHIFT1)|(14<< SHIFT2)| 16, ( 1 << SHIFT1)|(14<< SHIFT2)| 16, ( 0 << SHIFT1)|(13<< SHIFT2)| 16,
//    ( 0 << SHIFT1)|(16<< SHIFT2)| 16, ( 2 << SHIFT1)|(16<< SHIFT2)| 16, ( 1 << SHIFT1)|(16<< SHIFT2)| 16, ( 0 << SHIFT1)|(15<< SHIFT2)| 16,//13    //316b
//    ( 1 << SHIFT1)|(13<< SHIFT2)| 17, //14        //332b
//    IPPVC_VLC_FORBIDDEN         //15..19    //336b
//};
//
////The table coeff_token_map_24 for XScale
//static TABLE_TYPE coeff_token_map_24s2[] =
//{
//    (OFF+0*SZCF   << 8)|31,//0
//    (OFF+2*SZCF   << 8)|30,//1
//    (OFF+6*SZCF   << 8)|29,//2
//    (OFF+14*SZCF  << 8)|30,//3
//    (OFF+18*SZCF  << 8)|30,//4
//    (OFF+22*SZCF  << 8)|30,//5
//    (OFF+26*SZCF  << 8)|30,//6
//    (OFF+30*SZCF  << 8)|29,//7
//    (OFF+38*SZCF  << 8)|29,//8
//    (OFF+46*SZCF  << 8)|29,//9
//    (OFF+54*SZCF  << 8)|29,//10
//    (OFF+62*SZCF  << 8)|30,//11
//    (OFF+66*SZCF  << 8)|32,//12
//    (OFF+67*SZCF  << 8)|32,//13
//    (OFF+67*SZCF  << 8)|32,//14
//    (OFF+67*SZCF  << 8)|32,//15
//    (OFF+67*SZCF  << 8)|32,//16
//    (OFF+67*SZCF  << 8)|32,//17
//    (OFF+67*SZCF  << 8)|32,//18
//    (OFF+67*SZCF  << 8)|32,//19
//    (OFF+67*SZCF  << 8)|32,//20
//    (OFF+67*SZCF  << 8)|32,//21
//    (OFF+67*SZCF  << 8)|32,//22
//    (OFF+67*SZCF  << 8)|32,//23
//    (OFF+67*SZCF  << 8)|32,//24
//    (OFF+67*SZCF  << 8)|32,//25
//    (OFF+67*SZCF  << 8)|32,//26
//    (OFF+67*SZCF  << 8)|32,//27
//    (OFF+67*SZCF  << 8)|32,//28
//    (OFF+67*SZCF  << 8)|32,//29
//
//    ( 1 << SHIFT1)|(1 << SHIFT2)| 30, ( 0 << SHIFT1)|(0 << SHIFT2)| 30,                                                    //0    //80b
//
//    ( 3 << SHIFT1)|(4 << SHIFT2)| 28, ( 3 << SHIFT1)|(3 << SHIFT2)| 28, ( 2 << SHIFT1)|(2 << SHIFT2)| 29, ( 2 << SHIFT1)|(2 << SHIFT2)| 29, //1    //88b
//
//    ( 3 << SHIFT1)|(6 << SHIFT2)| 26, ( 2 << SHIFT1)|(3 << SHIFT2)| 26, ( 1 << SHIFT1)|(3 << SHIFT2)| 26, ( 0 << SHIFT1)|(1 << SHIFT2)| 26,    //2    //104b
//    ( 3 << SHIFT1)|(5 << SHIFT2)| 27, ( 3 << SHIFT1)|(5 << SHIFT2)| 27, ( 1 << SHIFT1)|(2 << SHIFT2)| 27, ( 1 << SHIFT1)|(2 << SHIFT2)| 27,
//
//    ( 3 << SHIFT1)|(7 << SHIFT2)| 26, ( 2 << SHIFT1)|(4 << SHIFT2)| 26, ( 1 << SHIFT1)|(4 << SHIFT2)| 26, ( 0 << SHIFT1)|(2 << SHIFT2)| 26,    //3    //136b
//
//    ( 3 << SHIFT1)|(8 << SHIFT2)| 25, ( 2 << SHIFT1)|(5 << SHIFT2)| 25, ( 1 << SHIFT1)|(5 << SHIFT2)| 25, ( 0 << SHIFT1)|(3 << SHIFT2)| 25,    //4    //152b
//
//    ( 0 << SHIFT1)|(5 << SHIFT2)| 24, ( 2 << SHIFT1)|(6 << SHIFT2)| 24, ( 1 << SHIFT1)|(6 << SHIFT2)| 24, ( 0 << SHIFT1)|(4 << SHIFT2)| 24,    //5    //168b
//
//    ( 3 << SHIFT1)|(9 << SHIFT2)| 23, ( 2 << SHIFT1)|(7 << SHIFT2)| 23, ( 1 << SHIFT1)|(7 << SHIFT2)| 23, ( 0 << SHIFT1)|(6 << SHIFT2)| 23,    //6    //184b
//
//    ( 3 << SHIFT1)|(11<< SHIFT2)| 21, ( 2 << SHIFT1)|(9 << SHIFT2)| 21, ( 1 << SHIFT1)|(9 << SHIFT2)| 21, ( 0 << SHIFT1)|(8 << SHIFT2)| 21,    //7    //200b
//    ( 3 << SHIFT1)|(10<< SHIFT2)| 21, ( 2 << SHIFT1)|(8 << SHIFT2)| 21, ( 1 << SHIFT1)|(8 << SHIFT2)| 21, ( 0 << SHIFT1)|(7 << SHIFT2)| 21,
//
//    ( 0 << SHIFT1)|(11<< SHIFT2)| 20, ( 2 << SHIFT1)|(11<< SHIFT2)| 20, ( 1 << SHIFT1)|(11<< SHIFT2)| 20, ( 0 << SHIFT1)|(10<< SHIFT2)| 20,    //8    //232b
//    ( 3 << SHIFT1)|(12<< SHIFT2)| 20, ( 2 << SHIFT1)|(10<< SHIFT2)| 20, ( 1 << SHIFT1)|(10<< SHIFT2)| 20, ( 0 << SHIFT1)|(9 << SHIFT2)| 20,
//
//    ( 3 << SHIFT1)|(14<< SHIFT2)| 19, ( 2 << SHIFT1)|(13<< SHIFT2)| 19, ( 1 << SHIFT1)|(13<< SHIFT2)| 19, ( 0 << SHIFT1)|(13<< SHIFT2)| 19,    //9    //264b
//    ( 3 << SHIFT1)|(13<< SHIFT2)| 19, ( 2 << SHIFT1)|(12<< SHIFT2)| 19, ( 1 << SHIFT1)|(12<< SHIFT2)| 19, ( 0 << SHIFT1)|(12<< SHIFT2)| 19,
//
//    ( 1 << SHIFT1)|(15<< SHIFT2)| 18, ( 0 << SHIFT1)|(15<< SHIFT2)| 18, ( 2 << SHIFT1)|(15<< SHIFT2)| 18, ( 1 << SHIFT1)|(14<< SHIFT2)| 18,    //10 //296b
//    ( 2 << SHIFT1)|(14<< SHIFT2)| 19, ( 2 << SHIFT1)|(14<< SHIFT2)| 19, ( 0 << SHIFT1)|(14<< SHIFT2)| 19, ( 0 << SHIFT1)|(14<< SHIFT2)| 19,
//
//    ( 3 << SHIFT1)|(16<< SHIFT2)| 18, ( 2 << SHIFT1)|(16<< SHIFT2)| 18, ( 1 << SHIFT1)|(16<< SHIFT2)| 18, ( 0 << SHIFT1)|(16<< SHIFT2)| 18,    //11 //328b
//
//    ( 3 << SHIFT1)|(15<< SHIFT2)| 19,     //12        //344b
//
//    IPPVC_VLC_FORBIDDEN            //13..19    //348b
//};
//
//
////The table coeff_token_map_48 for XScale
//static TABLE_TYPE coeff_token_map_48s2[] =
//{
//    (OFF+0*SZCF   << 8)|29,//0
//    (OFF+8*SZCF   << 8)|29,//1
//    (OFF+16*SZCF  << 8)|29,//2
//    (OFF+24*SZCF  << 8)|29,//3
//    (OFF+32*SZCF  << 8)|29,//4
//    (OFF+40*SZCF  << 8)|29,//5
//    (OFF+48*SZCF  << 8)|29,//6
//    (OFF+56*SZCF  << 8)|30,//7
//    (OFF+60*SZCF  << 8)|31,//8
//    (OFF+62*SZCF  << 8)|32,//9
//    (OFF+63*SZCF  << 8)|32,//10
//    (OFF+63*SZCF  << 8)|32,//11
//    (OFF+63*SZCF  << 8)|32,//12
//    (OFF+63*SZCF  << 8)|32,//13
//    (OFF+63*SZCF  << 8)|32,//14
//    (OFF+63*SZCF  << 8)|32,//15
//    (OFF+63*SZCF  << 8)|32,//16
//    (OFF+63*SZCF  << 8)|32,//17
//    (OFF+63*SZCF  << 8)|32,//18
//    (OFF+63*SZCF  << 8)|32,//19
//    (OFF+63*SZCF  << 8)|32,//20
//    (OFF+63*SZCF  << 8)|32,//21
//    (OFF+63*SZCF  << 8)|32,//22
//    (OFF+63*SZCF  << 8)|32,//23
//    (OFF+63*SZCF  << 8)|32,//24
//    (OFF+63*SZCF  << 8)|32,//25
//    (OFF+63*SZCF  << 8)|32,//26
//    (OFF+63*SZCF  << 8)|32,//27
//    (OFF+63*SZCF  << 8)|32,//28
//    (OFF+63*SZCF  << 8)|32,//29
//
//    ( 3 << SHIFT1)|(7 << SHIFT2)| 28, ( 3 << SHIFT1)|(6 << SHIFT2)| 28, ( 3 << SHIFT1)|(5 << SHIFT2)| 28, ( 3 << SHIFT1)|(4 << SHIFT2)| 28,    //0 //80b
//    ( 3 << SHIFT1)|(3 << SHIFT2)| 28, ( 2 << SHIFT1)|(2 << SHIFT2)| 28, ( 1 << SHIFT1)|(1 << SHIFT2)| 28, ( 0 << SHIFT1)|(0 << SHIFT2)| 28,
//
//    ( 1 << SHIFT1)|(5 << SHIFT2)| 27, ( 2 << SHIFT1)|(5 << SHIFT2)| 27, ( 1 << SHIFT1)|(4 << SHIFT2)| 27, ( 2 << SHIFT1)|(4 << SHIFT2)| 27,    //1 //112
//    ( 1 << SHIFT1)|(3 << SHIFT2)| 27, ( 3 << SHIFT1)|(8 << SHIFT2)| 27, ( 2 << SHIFT1)|(3 << SHIFT2)| 27, ( 1 << SHIFT1)|(2 << SHIFT2)| 27,
//
//    ( 0 << SHIFT1)|(3 << SHIFT2)| 26, ( 2 << SHIFT1)|(7 << SHIFT2)| 26, ( 1 << SHIFT1)|(7 << SHIFT2)| 26, ( 0 << SHIFT1)|(2 << SHIFT2)| 26,    //2 //144
//    ( 3 << SHIFT1)|(9 << SHIFT2)| 26, ( 2 << SHIFT1)|(6 << SHIFT2)| 26, ( 1 << SHIFT1)|(6 << SHIFT2)| 26, ( 0 << SHIFT1)|(1 << SHIFT2)| 26,
//
//    ( 0 << SHIFT1)|(7 << SHIFT2)| 25, ( 0 << SHIFT1)|(6 << SHIFT2)| 25, ( 2 << SHIFT1)|(9 << SHIFT2)| 25, ( 0 << SHIFT1)|(5 << SHIFT2)| 25,    //3 //176
//    ( 3 << SHIFT1)|(10<< SHIFT2)| 25, ( 2 << SHIFT1)|(8 << SHIFT2)| 25, ( 1 << SHIFT1)|(8 << SHIFT2)| 25, ( 0 << SHIFT1)|(4 << SHIFT2)| 25,
//
//    ( 3 << SHIFT1)|(12<< SHIFT2)| 24, ( 2 << SHIFT1)|(11<< SHIFT2)| 24, ( 1 << SHIFT1)|(10<< SHIFT2)| 24, ( 0 << SHIFT1)|(9 << SHIFT2)| 24,    //4 //208
//    ( 3 << SHIFT1)|(11<< SHIFT2)| 24, ( 2 << SHIFT1)|(10<< SHIFT2)| 24, ( 1 << SHIFT1)|(9 << SHIFT2)| 24, ( 0 << SHIFT1)|(8 << SHIFT2)| 24,
//
//    ( 0 << SHIFT1)|(12<< SHIFT2)| 23, ( 2 << SHIFT1)|(13<< SHIFT2)| 23, ( 1 << SHIFT1)|(12<< SHIFT2)| 23, ( 0 << SHIFT1)|(11<< SHIFT2)| 23,    //5 //240
//    ( 3 << SHIFT1)|(13<< SHIFT2)| 23, ( 2 << SHIFT1)|(12<< SHIFT2)| 23, ( 1 << SHIFT1)|(11<< SHIFT2)| 23, ( 0 << SHIFT1)|(10<< SHIFT2)| 23,
//
//    ( 1 << SHIFT1)|(15<< SHIFT2)| 22, ( 0 << SHIFT1)|(14<< SHIFT2)| 22, ( 3 << SHIFT1)|(14<< SHIFT2)| 22, ( 2 << SHIFT1)|(14<< SHIFT2)| 22,    //6 //272
//    ( 1 << SHIFT1)|(14<< SHIFT2)| 22, ( 0 << SHIFT1)|(13<< SHIFT2)| 22, ( 1 << SHIFT1)|(13<< SHIFT2)| 23, ( 1 << SHIFT1)|(13<< SHIFT2)| 23,
//
//    ( 1 << SHIFT1)|(16<< SHIFT2)| 22, ( 0 << SHIFT1)|(15<< SHIFT2)| 22, ( 3 << SHIFT1)|(15<< SHIFT2)| 22, ( 2 << SHIFT1)|(15<< SHIFT2)| 22,    //7 //304
//
//    ( 3 << SHIFT1)|(16<< SHIFT2)| 22, ( 2 << SHIFT1)|(16<< SHIFT2)| 22,     //8            //320
//
//    ( 0 << SHIFT1)|(16<< SHIFT2)| 22,                             //9            //328
//    IPPVC_VLC_FORBIDDEN                                    //10..19    //332b
//};
//#endif
//
//#undef OFF

} // end namespace UMC

#endif //__UMC_H264_DEC_COEFF_TOKEN_MAP_H__






