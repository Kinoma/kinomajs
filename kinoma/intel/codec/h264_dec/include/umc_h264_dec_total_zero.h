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
#ifndef __UMC_H264_DEC_TOTAL_ZEROS_H__
#define __UMC_H264_DEC_TOTAL_ZEROS_H__

namespace UMC
{

//#if !defined (ARM) && !defined (_ARM_)

#ifdef __INTEL_IPP__
//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 1
/*
0    1

2    010
1    011

4    0010
3    0011

6    0001 0
5    0001 1

8    0000 10
7    0000 11

10    0000 010
9    0000 011

12    0000 0010
11    0000 0011

15    0000 0000 1
14    0000 0001 0
13    0000 0001 1
*/

static
Ipp32s total_zeros_map_1[] =
{
9, /* max bits */
2,  /* total subtables */
5,4,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 0,

0, /* 2-bit codes */

2, /* 3-bit codes */
0x0002, 2, 0x0003, 1,

2, /* 4-bit codes */
0x0002, 4, 0x0003, 3,

2, /* 5-bit codes */
0x0002, 6, 0x0003, 5,

2, /* 6-bit codes */
0x0002, 8, 0x0003, 7,

2, /* 7-bit codes */
0x0002, 10, 0x0003, 9,

2, /* 8-bit codes */
0x0002, 12, 0x0003, 11,

3, /* 9-bit codes */
0x0001, 15, 0x0002, 14, 0x0003, 13,

-1
};




//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 2

/*
4    011
3    100
2    101
1    110
0    111

8    0010
7    0011
6    0100
5    0101

10    0001 0
9    0001 1

14    0000 00
13    0000 01
12    0000 10
11    0000 11
*/

static
Ipp32s total_zeros_map_2[] =
{
6, /* max bits */
2,  /* total subtables */
3,3,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */

5, /* 3-bit codes */
0x0003, 4, 0x0004, 3, 0x0005, 2, 0x0006, 1,
0x0007, 0,

4, /* 4-bit codes */
0x0002, 8, 0x0003, 7, 0x0004, 6, 0x0005, 5,

2, /* 5-bit codes */
0x0002, 10, 0x0003, 9,

4, /* 6-bit codes */
0x0000, 14, 0x0001, 13, 0x0002, 12, 0x0003, 11,

-1
};




//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 3

/*
7    011
6    100
3    101
2    110
1    111

8    0010
5    0011
4    0100
0    0101

12    0000 1
10    0001 0
9    0001 1

13    0000 00
11    0000 01
*/

static
Ipp32s total_zeros_map_3[] =
{
6, /* max bits */
2,  /* total subtables */
3,3,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */

5, /* 3-bit codes */
0x0003, 7, 0x0004, 6, 0x0005, 3, 0x0006, 2,
0x0007, 1,

4, /* 4-bit codes */
0x0002, 8, 0x0003, 5, 0x0004, 4, 0x0005, 0,

3, /* 5-bit codes */
0x0001, 12, 0x0002, 10, 0x0003, 9,

2, /* 6-bit codes */
0x0000, 13, 0x0001, 11,

-1
};




//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 4
/*
8  011
6  100
5  101
4  110
1  111

9  0010
7  0011
3  0100
2  0101

12 0000 0
11 0000 1
10 0001 0
0  0001 1
*/

static
Ipp32s total_zeros_map_4[] =
{
5, /* max bits */
2,  /* total subtables */
2,3,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */

5, /* 3-bit codes */
0x0003, 8, 0x0004, 6, 0x0005, 5, 0x0006, 4,
0x0007, 1,

4, /* 4-bit codes */
0x0002, 9, 0x0003, 7, 0x0004, 3, 0x0005, 2,

4, /* 5-bit codes */
0x0000, 12, 0x0001, 11, 0x0002, 10, 0x0003, 0,

-1
};


//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 5
/*
7    011
6    100
5    101
4    110
3    111

10    0001
8    0010
2    0011
1    0100
0    0101

11    0000 0
9    0000 1
*/

static
Ipp32s total_zeros_map_5[] =
{
5, /* max bits */
2,  /* total subtables */
2,3,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */

5, /* 3-bit codes */
0x0003, 7, 0x0004, 6, 0x0005, 5, 0x0006, 4,
0x0007, 3,

5, /* 4-bit codes */
0x0001, 10, 0x0002, 8, 0x0003, 2, 0x0004, 1,
0x0005, 0,

2, /* 5-bit codes */
0x0000, 11, 0x0001, 9,

-1
};


//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 6
/*
9    001
7    010
6    011
5    100
4    101
3    110
2    111

8    0001

1    0000 1

10    0000 00
0    0000 01
*/

static
Ipp32s total_zeros_map_6[] =
{
6, /* max bits */
2,  /* total subtables */
3,3,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */

7, /* 3-bit codes */
0x0001, 9, 0x0002, 7, 0x0003, 6, 0x0004, 5,
0x0005, 4, 0x0006, 3, 0x0007, 2,

1, /* 4-bit codes */
0x0001, 8,

1, /* 5-bit codes */
0x0001, 1,

2, /* 6-bit codes */
0x0000, 10, 0x0001, 0,

-1
};


//Table 9 7 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 7
/*
5    11

8    001
6    010
4    011
3    100
2    101

7    0001

1    0000 1

9    0000 00
0    0000 01

*/

static
Ipp32s total_zeros_map_7[] =
{
6, /* max bits */
2,  /* total subtables */
3,3,/* subtable sizes */

0, /* 1-bit codes */

1, /* 2-bit codes */
0x0003, 5,

5, /* 3-bit codes */
0x0001, 8, 0x0002, 6, 0x0003, 4, 0x0004, 3,
0x0005, 2,

1, /* 4-bit codes */
0x0001, 7,

1, /* 5-bit codes */
0x0001, 1,

2, /* 6-bit codes */
0x0000, 9, 0x0001, 0,

-1
};



//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 8
/*
5    10
4    11

7    001
6    010
3    011

1    0001

2    0000 1

8    0000 00
0    0000 01

*/

static
Ipp32s total_zeros_map_8[] =
{
6, /* max bits */
2,  /* total subtables */
3,3,/* subtable sizes */

0, /* 1-bit codes */

2, /* 2-bit codes */
0x0002, 5,0x0003, 4,

3, /* 3-bit codes */
0x0001, 7, 0x0002, 6, 0x0003, 3,

1, /* 4-bit codes */
0x0001, 1,

1, /* 5-bit codes */
0x0001, 2,

2, /* 6-bit codes */
0x0000, 8, 0x0001, 0,

-1
};


//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 9
/*
6    01
4    10
3    11

5    001

2    0001

7    0000 1

1    0000 00
0    0000 01
*/

static
Ipp32s total_zeros_map_9[] =
{
6, /* max bits */
2,  /* total subtables */
3,3,/* subtable sizes */

0, /* 1-bit codes */

3, /* 2-bit codes */
0x0001, 6, 0x0002, 4, 0x0003, 3,

1, /* 3-bit codes */
0x0001, 5,

1, /* 4-bit codes */
0x0001, 2,

1, /* 5-bit codes */
0x0001, 7,

2, /* 6-bit codes */
0x0000, 1, 0x0001, 0,

-1
};


//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 10
/*
5    01
4    10
3    11

2    001

6    0001

1    0000 0
0    0000 1
*/

static
Ipp32s total_zeros_map_10[] =
{
5, /* max bits */
2,  /* total subtables */
2,3,/* subtable sizes */

0, /* 1-bit codes */

3, /* 2-bit codes */
0x0001, 5, 0x0002, 4, 0x0003, 3,

1, /* 3-bit codes */
0x0001, 2,

1, /* 4-bit codes */
0x0001, 6,

2, /* 5-bit codes */
0x0000, 1, 0x0001, 0,

-1
};



//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 11
/*

4    1

2    001
3    010
5    011

0    0000
1    0001
*/

static
Ipp32s total_zeros_map_11[] =
{
4, /* max bits */
1,  /* total subtables */
4,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 4,

0, /* 2-bit codes */

3, /* 3-bit codes */
0x0001, 2, 0x0002, 3, 0x0003, 5,

2, /* 4-bit codes */
0x0000, 0, 0x0001, 1,


-1
};


//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 12
/*
3    1

2    01

4    001

0    0000
1    0001
*/

static
Ipp32s total_zeros_map_12[] =
{
4, /* max bits */
1,  /* total subtables */
4,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 3,

1, /* 2-bit codes */
0x0001, 2,

1, /* 3-bit codes */
0x0001, 4,

2, /* 4-bit codes */
0x0000, 0, 0x0001, 1,


-1
};


//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 13
/*
2    1

3    01

0    000
1    001

*/

static
Ipp32s total_zeros_map_13[] =
{
3, /* max bits */
1,  /* total subtables */
3,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 2,

1, /* 2-bit codes */
0x0001, 3,

2, /* 3-bit codes */
0x0000, 0, 0x0001, 1,

-1
};



//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 14
/*
2    1

0    00
1    01

*/

static
Ipp32s total_zeros_map_14[] =
{
2, /* max bits */
1,  /* total subtables */
2,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 2,

2, /* 2-bit codes */
0x0000, 0, 0x0001, 1,

-1
};


//Table 9-8 - total_zeros tables for 4x4 blocks with
//TotalCoeff( coeff_token ) == 15
/*
0    0
1    1

*/

static
Ipp32s total_zeros_map_15[] =
{
1, /* max bits */
1,  /* total subtables */
1,/* subtable sizes */

2, /* 1-bit codes */
0x0000, 0, 0x0001, 1,

-1
};


//Table 9-9 - total_zeros tables for chroma DC 2x2 blocks
//TotalCoeff( coeff_token ) == 1
/*
0    1
1    01
2    001
3    000
*/

static
Ipp32s total_zeros_map_cr1[] =
{
3, /* max bits */
1, /* total subtables */
3,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 0,

1, /* 2-bit codes */
0x0001, 1,

2, /* 3-bit codes */
0x0000, 3, 0x0001, 2,

-1
};


//Table 9-9 - total_zeros tables for chroma DC 2x2 blocks
//TotalCoeff( coeff_token ) == 2
/*
0    1
1    01
2    00
*/

static
Ipp32s total_zeros_map_cr2[] =
{
2, /* max bits */
1, /* total subtables */
2,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 0,

2, /* 2-bit codes */
0x0000, 2, 0x0001, 1,

-1
};



//Table 9-9 - total_zeros tables for chroma DC 2x2 blocks
//TotalCoeff( coeff_token ) == 3
/*
0    1
1    0
*/

static
Ipp32s total_zeros_map_cr3[] =
{
1, /* max bits */
1, /* total subtables */
1,/* subtable sizes */

2, /* 1-bit codes */
0x0001, 0, 0x0000, 1,

-1
};

static
Ipp32s total_zeros_map_cr422_1[] =
{
    5, /* max bits */
    1, /* total subtables */
    5,/* subtable sizes */

    1, /* 1-bit codes */
    0x0001, 0,

    0, /* 2-bit codes */
    2, /* 3-bit codes */
    0x0002, 1,0x0003, 2,
    3, /* 4-bit codes */
    0x0001, 5,
    0x0002, 3,0x0003, 4,
    2, /* 5-bit codes */
    0x0000, 7,0x0001, 6,

    -1
};

static
Ipp32s total_zeros_map_cr422_2[] =
{
    3, /* max bits */
    1, /* total subtables */
    3,/* subtable sizes */

    0, /* 1-bit codes */

    1, /* 2-bit codes */
    0x0001, 1,
    6, /* 3-bit codes */
    0x0000, 0,0x0001, 2,
    0x0004, 3,0x0005, 4,
    0x0006, 5,0x0007, 6,

    -1
};

static
Ipp32s total_zeros_map_cr422_3[] =
{
    3, /* max bits */
    1, /* total subtables */
    3,/* subtable sizes */

    0, /* 1-bit codes */

    2, /* 2-bit codes */
    0x0001, 2,0x0002, 3,
    4, /* 3-bit codes */
    0x0000, 0,0x0001, 1,
    0x0006, 4,0x0007, 5,

    -1
};

static
Ipp32s total_zeros_map_cr422_4[] =
{
    3, /* max bits */
    1, /* total subtables */
    3,/* subtable sizes */

    0, /* 1-bit codes */

    3, /* 2-bit codes */
    0x0000, 1,0x0001, 2,
    0x0002, 3,
    2, /* 3-bit codes */
    0x0006, 0,0x0007, 4,

    -1
};

static
Ipp32s total_zeros_map_cr422_5[] =
{
    2, /* max bits */
    1, /* total subtables */
    2,/* subtable sizes */

    0, /* 1-bit codes */

    4, /* 2-bit codes */
    0x0000, 0,0x0001, 1,
    0x0002, 2,0x0003, 3,


    -1
};

static
Ipp32s total_zeros_map_cr422_6[] =
{
    2, /* max bits */
    1, /* total subtables */
    2,/* subtable sizes */

    1, /* 1-bit codes */
    0x0001, 2,

    2, /* 2-bit codes */
    0x0000, 0, 0x0001, 1,

    -1
};

static
Ipp32s total_zeros_map_cr422_7[] =
{
    1, /* max bits */
    1, /* total subtables */
    1,/* subtable sizes */

    2, /* 1-bit codes */
    0x0000, 0, 0x0001, 1,

    -1
};
#endif
//#else
///*
//#undef OFF
//#define OFF 120//60//
//
//#undef SZCF
//#define SZCF 4//2//
//
//#undef SHIFT1
//#define SHIFT1 8
//
//#undef TABLE_TYPE
//#define TABLE_TYPE int //unsigned short
//*/
//
//#undef OFF
//#undef SZCF
//
//#undef SHIFT1
//#define SHIFT1 8
//
//#undef TABLE_TYPE
//
//#if defined (ARM) || defined (_ARM_)
//
//#define OFF 60//120//
//#define SZCF 2//4//
//#define TABLE_TYPE unsigned short // int //
//
//#else   //  defined (ARM) || defined (_ARM_)
//
//#define OFF 120//60//
//#define SZCF 4//2//
//#define TABLE_TYPE int //unsigned short
//
//#endif   //  defined (ARM) || defined (_ARM_)
//
//
////The table total_zeros_map_1 for XScale
//
//static TABLE_TYPE total_zeros_map_1s2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|31,//1
//    (OFF+3*SZCF << 8)|31,//2
//    (OFF+5*SZCF << 8)|31,//3
//    (OFF+7*SZCF << 8)|31,//4
//    (OFF+9*SZCF << 8)|31,//5
//    (OFF+11*SZCF << 8)|31,//6
//    (OFF+13*SZCF << 8)|31,//7
//    (OFF+15*SZCF << 8)|32,//8
//    (OFF+16*SZCF << 8)|32,//9
//    (OFF+16*SZCF << 8)|32,//10
//    (OFF+16*SZCF << 8)|32,//11
//    (OFF+16*SZCF << 8)|32,//12
//    (OFF+16*SZCF << 8)|32,//13
//    (OFF+16*SZCF << 8)|32,//14
//    (OFF+16*SZCF << 8)|32,//15
//    (OFF+16*SZCF << 8)|32,//16
//    (OFF+16*SZCF << 8)|32,//17
//    (OFF+16*SZCF << 8)|32,//18
//    (OFF+16*SZCF << 8)|32,//19
//    (OFF+16*SZCF << 8)|32,//20
//    (OFF+16*SZCF << 8)|32,//21
//    (OFF+16*SZCF << 8)|32,//22
//    (OFF+16*SZCF << 8)|32,//23
//    (OFF+16*SZCF << 8)|32,//24
//    (OFF+16*SZCF << 8)|32,//25
//    (OFF+16*SZCF << 8)|32,//26
//    (OFF+16*SZCF << 8)|32,//27
//    (OFF+16*SZCF << 8)|32,//28
//    (OFF+16*SZCF << 8)|32,//29
//
//    (0<<8)| 31,                 //0    //80
//    (2<<8)| 29,    (1<<8)| 29,        //1    //84
//    (4<<8)| 28,    (3<<8)| 28,        //2    //92
//    (6<<8)| 27,    (5<<8)| 27,        //3    //100
//    (8<<8)| 26,    (7<<8)| 26,        //4    //108
//    (10<<8)|25,    (9<<8)| 25,        //5    //116
//    (12<<8)|24,    (11<<8)|24,        //6    //124
//    (14<<8)|23,    (13<<8)|23,        //7    //132
//    (15<<8)|23,                    //8    //140
//    IPPVC_VLC_FORBIDDEN            //9    //144
//
//};
//
////The table total_zeros_map_2 for XScale
//static TABLE_TYPE total_zeros_map_2s2[] =
//{
//    (OFF+0*SZCF  << 8)|30,//0
//    (OFF+4*SZCF << 8)|30,//1
//    (OFF+8*SZCF << 8)|31,//2
//    (OFF+10*SZCF << 8)|31,//3
//    (OFF+12*SZCF << 8)|31,//4
//    (OFF+14*SZCF << 8)|32,//5
//    (OFF+15*SZCF << 8)|32,//6
//    (OFF+15*SZCF << 8)|32,//7
//    (OFF+15*SZCF << 8)|32,//8
//    (OFF+15*SZCF << 8)|32,//9
//    (OFF+15*SZCF << 8)|32,//10
//    (OFF+15*SZCF << 8)|32,//11
//    (OFF+15*SZCF << 8)|32,//12
//    (OFF+15*SZCF << 8)|32,//13
//    (OFF+15*SZCF << 8)|32,//14
//    (OFF+15*SZCF << 8)|32,//15
//    (OFF+15*SZCF << 8)|32,//16
//    (OFF+15*SZCF << 8)|32,//17
//    (OFF+15*SZCF << 8)|32,//18
//    (OFF+15*SZCF << 8)|32,//19
//    (OFF+15*SZCF << 8)|32,//10
//    (OFF+15*SZCF << 8)|32,//11
//    (OFF+15*SZCF << 8)|32,//12
//    (OFF+15*SZCF << 8)|32,//13
//    (OFF+15*SZCF << 8)|32,//14
//    (OFF+15*SZCF << 8)|32,//15
//    (OFF+15*SZCF << 8)|32,//16
//    (OFF+15*SZCF << 8)|32,//17
//    (OFF+15*SZCF << 8)|32,//18
//    (OFF+15*SZCF << 8)|32,//19
//
//    (3<<8)| 29, (2<<8)| 29,(1<<8)| 29, (0<<8)| 29,    //0    //80
//    (6<<8)| 28, (5<<8)| 28,(4<<8)| 29, (4<<8)| 29,    //1    //96
//    (8<<8)| 28,    (7<<8)| 28,                            //2    //112
//    (10<<8)|27,    (9<<8)| 27,                            //3    //120
//    (12<<8)|26,    (11<<8)|26,                            //4    //128
//    (13<<8)|26,                                        //5    //136
//    (14<<8)|26,                                        //6    //140
//
//};
//
////The table total_zeros_map_6 for XScale
//static TABLE_TYPE total_zeros_map_6s2[] =
//{
//    (OFF+0*SZCF  << 8)|30,//0
//    (OFF+4*SZCF << 8)|31,//1
//    (OFF+6*SZCF << 8)|32,//2
//    (OFF+7*SZCF << 8)|32,//3
//    (OFF+8*SZCF << 8)|32,//4
//    (OFF+9*SZCF << 8)|32,//5
//    (OFF+10*SZCF << 8)|32,//6
//    (OFF+10*SZCF << 8)|32,//7
//    (OFF+10*SZCF << 8)|32,//8
//    (OFF+10*SZCF << 8)|32,//9
//    (OFF+10*SZCF << 8)|32,//10
//    (OFF+10*SZCF << 8)|32,//11
//    (OFF+10*SZCF << 8)|32,//12
//    (OFF+10*SZCF << 8)|32,//13
//    (OFF+10*SZCF << 8)|32,//14
//    (OFF+10*SZCF << 8)|32,//15
//    (OFF+10*SZCF << 8)|32,//16
//    (OFF+10*SZCF << 8)|32,//17
//    (OFF+10*SZCF << 8)|32,//18
//    (OFF+10*SZCF << 8)|32,//19
//    (OFF+10*SZCF << 8)|32,//10
//    (OFF+10*SZCF << 8)|32,//11
//    (OFF+10*SZCF << 8)|32,//12
//    (OFF+10*SZCF << 8)|32,//13
//    (OFF+10*SZCF << 8)|32,//14
//    (OFF+10*SZCF << 8)|32,//15
//    (OFF+10*SZCF << 8)|32,//16
//    (OFF+10*SZCF << 8)|32,//17
//    (OFF+10*SZCF << 8)|32,//18
//    (OFF+10*SZCF << 8)|32,//19
//
//    (5<<8)| 29,(4<<8)| 29,(3<<8)| 29, (2<<8)| 29,    //0        //80
//    (7<<8)| 29,(6<<8)| 29,                            //1        //96
//    (9<<8)| 29,                                        //2        //104
//    (8<<8)| 28,                                        //3        //108
//    (1<<8)| 27,                                        //4        //112
//    (0<<8)| 26,                                        //5        //116
//    (10<<8)|26,                                        //6..15    //120
//
//};
////The table total_zeros_map_5 for XScale
//static TABLE_TYPE total_zeros_map_5s2[] =
//{
//    (OFF+0*SZCF  << 8)|30,//0
//    (OFF+4*SZCF << 8)|30,//1
//    (OFF+8*SZCF << 8)|31,//2
//    (OFF+10*SZCF << 8)|32,//3
//    (OFF+11*SZCF << 8)|32,//4
//    (OFF+12*SZCF << 8)|32,//5
//    (OFF+12*SZCF << 8)|32,//6
//    (OFF+12*SZCF << 8)|32,//7
//    (OFF+12*SZCF << 8)|32,//8
//    (OFF+12*SZCF << 8)|32,//9
//    (OFF+12*SZCF << 8)|32,//10
//    (OFF+12*SZCF << 8)|32,//11
//    (OFF+12*SZCF << 8)|32,//12
//    (OFF+12*SZCF << 8)|32,//13
//    (OFF+12*SZCF << 8)|32,//14
//    (OFF+12*SZCF << 8)|32,//15
//    (OFF+12*SZCF << 8)|32,//16
//    (OFF+12*SZCF << 8)|32,//17
//    (OFF+12*SZCF << 8)|32,//18
//    (OFF+12*SZCF << 8)|32,//19
//    (OFF+12*SZCF << 8)|32,//10
//    (OFF+12*SZCF << 8)|32,//11
//    (OFF+12*SZCF << 8)|32,//12
//    (OFF+12*SZCF << 8)|32,//13
//    (OFF+12*SZCF << 8)|32,//14
//    (OFF+12*SZCF << 8)|32,//15
//    (OFF+12*SZCF << 8)|32,//16
//    (OFF+12*SZCF << 8)|32,//17
//    (OFF+12*SZCF << 8)|32,//18
//    (OFF+12*SZCF << 8)|32,//19
//
//    (6<<8)| 29,(5<<8)| 29,(4<<8)| 29, (3<<8)| 29,    //0        //80
//    (1<<8)| 28,(0<<8)| 28,(7<<8)| 29, (7<<8)| 29,    //1        //96
//    (8<<8)| 28,    (2<<8)| 28,                            //2        //112
//    (10<<8)|28,                                        //3        //120
//    (9 <<8)|27,                                        //4        //124
//    (11<<8)|27,                                        //5..15    //128
//
//};
////The table total_zeros_map_4 for XScale
//static TABLE_TYPE total_zeros_map_4s2[] =
//{
//    (OFF+0*SZCF    << 8)|30,//0
//    (OFF+4*SZCF << 8)|30,//1
//    (OFF+8*SZCF << 8)|31,//2
//    (OFF+10*SZCF << 8)|31,//3
//    (OFF+12*SZCF << 8)|32,//4
//    (OFF+13*SZCF << 8)|32,//5
//    (OFF+13*SZCF << 8)|32,//6
//    (OFF+13*SZCF << 8)|32,//7
//    (OFF+13*SZCF << 8)|32,//8
//    (OFF+13*SZCF << 8)|32,//9
//    (OFF+13*SZCF << 8)|32,//10
//    (OFF+13*SZCF << 8)|32,//11
//    (OFF+13*SZCF << 8)|32,//12
//    (OFF+13*SZCF << 8)|32,//13
//    (OFF+13*SZCF << 8)|32,//14
//    (OFF+13*SZCF << 8)|32,//15
//    (OFF+13*SZCF << 8)|32,//16
//    (OFF+13*SZCF << 8)|32,//17
//    (OFF+13*SZCF << 8)|32,//18
//    (OFF+13*SZCF << 8)|32,//19
//    (OFF+13*SZCF << 8)|32,//10
//    (OFF+13*SZCF << 8)|32,//11
//    (OFF+13*SZCF << 8)|32,//12
//    (OFF+13*SZCF << 8)|32,//13
//    (OFF+13*SZCF << 8)|32,//14
//    (OFF+13*SZCF << 8)|32,//15
//    (OFF+13*SZCF << 8)|32,//16
//    (OFF+13*SZCF << 8)|32,//17
//    (OFF+13*SZCF << 8)|32,//18
//    (OFF+13*SZCF << 8)|32,//19
//
//    (6<<8)| 29,(5<<8)| 29,(4<<8)| 29, (1<<8)| 29,    //0        //80
//    (3<<8)| 28,(2<<8)| 28,(8<<8)| 29, (8<<8)| 29,    //1        //96
//    (9<<8)| 28,    (7<<8)| 28,                            //2        //112
//    (10<<8)|27,    (0<<8)| 27,                            //3        //120
//    (11<<8)|27,                                        //4        //128
//    (12<<8)|27,                                        //5..15    //132
//
//};
////The table total_zeros_map_3 for XScale
//static TABLE_TYPE total_zeros_map_3s2[] =
//{
//    (OFF+0*SZCF  << 8)|30,//0
//    (OFF+4*SZCF << 8)|30,//1
//    (OFF+8*SZCF << 8)|31,//2
//    (OFF+10*SZCF << 8)|31,//3
//    (OFF+12*SZCF << 8)|32,//4
//    (OFF+13*SZCF << 8)|32,//5
//    (OFF+14*SZCF << 8)|32,//6
//    (OFF+14*SZCF << 8)|32,//7
//    (OFF+14*SZCF << 8)|32,//8
//    (OFF+14*SZCF << 8)|32,//9
//    (OFF+14*SZCF << 8)|32,//10
//    (OFF+14*SZCF << 8)|32,//11
//    (OFF+14*SZCF << 8)|32,//12
//    (OFF+14*SZCF << 8)|32,//13
//    (OFF+14*SZCF << 8)|32,//14
//    (OFF+14*SZCF << 8)|32,//15
//    (OFF+14*SZCF << 8)|32,//16
//    (OFF+14*SZCF << 8)|32,//17
//    (OFF+14*SZCF << 8)|32,//18
//    (OFF+14*SZCF << 8)|32,//19
//    (OFF+14*SZCF << 8)|32,//10
//    (OFF+14*SZCF << 8)|32,//11
//    (OFF+14*SZCF << 8)|32,//12
//    (OFF+14*SZCF << 8)|32,//13
//    (OFF+14*SZCF << 8)|32,//14
//    (OFF+14*SZCF << 8)|32,//15
//    (OFF+14*SZCF << 8)|32,//16
//    (OFF+14*SZCF << 8)|32,//17
//    (OFF+14*SZCF << 8)|32,//18
//    (OFF+14*SZCF << 8)|32,//19
//
//    (6<<8)| 29,(3<<8)| 29,(2<<8)| 29, (1<<8)| 29,    //0    //80
//    (4<<8)| 28,(0<<8)| 28,(7<<8)| 29, (7<<8)| 29,    //1    //96
//    (8<<8)| 28,    (5<<8)| 28,                            //2    //112
//    (10<<8)|27,    (9<<8)| 27,                            //3    //120
//    (12<<8)|27,                                        //4    //128
//    (11<<8)|26,                                        //5    //132
//    (13<<8)|26,                                        //6..15    //136
//
//};
////The table total_zeros_map_9 for XScale
//static TABLE_TYPE total_zeros_map_9s2[] =
//{
//    (OFF+0*SZCF   << 8)|31,//0
//    (OFF+2*SZCF   << 8)|32,//1
//    (OFF+3*SZCF  << 8)|32,//2
//    (OFF+4*SZCF  << 8)|32,//3
//    (OFF+5*SZCF  << 8)|32,//4
//    (OFF+6*SZCF  << 8)|32,//5
//    (OFF+7*SZCF  << 8)|32,//6
//    (OFF+7*SZCF  << 8)|32,//7
//    (OFF+7*SZCF  << 8)|32,//8
//    (OFF+7*SZCF  << 8)|32,//9
//    (OFF+7*SZCF  << 8)|32,//10
//    (OFF+7*SZCF  << 8)|32,//11
//    (OFF+7*SZCF  << 8)|32,//12
//    (OFF+7*SZCF  << 8)|32,//13
//    (OFF+7*SZCF  << 8)|32,//14
//    (OFF+7*SZCF  << 8)|32,//15
//    (OFF+7*SZCF  << 8)|32,//16
//    (OFF+7*SZCF  << 8)|32,//17
//    (OFF+7*SZCF  << 8)|32,//18
//    (OFF+7*SZCF  << 8)|32,//19
//    (OFF+7*SZCF  << 8)|32,//10
//    (OFF+7*SZCF  << 8)|32,//11
//    (OFF+7*SZCF  << 8)|32,//12
//    (OFF+7*SZCF  << 8)|32,//13
//    (OFF+7*SZCF  << 8)|32,//14
//    (OFF+7*SZCF  << 8)|32,//15
//    (OFF+7*SZCF  << 8)|32,//16
//    (OFF+7*SZCF  << 8)|32,//17
//    (OFF+7*SZCF  << 8)|32,//18
//    (OFF+7*SZCF  << 8)|32,//19
//
//    (4<<8)| 30, (3<<8)| 30,                            //0
//    (6<<8)| 30,                                     //1
//    (5<<8)| 29,                                        //2
//    (2<<8)| 28,                                        //3
//    (7<<8)| 27,                                        //4
//    (0<<8)| 26,                                        //5
//    (1<<8)| 26,                                        //6..15
//
//};
////The table total_zeros_map_8 for XScale
//static TABLE_TYPE total_zeros_map_8s2[] =
//{
//    (OFF+0*SZCF << 8)|31,//0
//    (OFF+2*SZCF << 8)|31,//1
//    (OFF+4*SZCF << 8)|32,//2
//    (OFF+5*SZCF << 8)|32,//3
//    (OFF+6*SZCF << 8)|32,//4
//    (OFF+7*SZCF << 8)|32,//5
//    (OFF+8*SZCF << 8)|32,//6
//    (OFF+8*SZCF << 8)|32,//7
//    (OFF+8*SZCF << 8)|32,//8
//    (OFF+8*SZCF << 8)|32,//9
//    (OFF+8*SZCF << 8)|32,//10
//    (OFF+8*SZCF << 8)|32,//11
//    (OFF+8*SZCF << 8)|32,//12
//    (OFF+8*SZCF << 8)|32,//13
//    (OFF+8*SZCF << 8)|32,//14
//    (OFF+8*SZCF << 8)|32,//15
//    (OFF+8*SZCF << 8)|32,//16
//    (OFF+8*SZCF << 8)|32,//17
//    (OFF+8*SZCF << 8)|32,//18
//    (OFF+8*SZCF << 8)|32,//19
//    (OFF+8*SZCF << 8)|32,//10
//    (OFF+8*SZCF << 8)|32,//11
//    (OFF+8*SZCF << 8)|32,//12
//    (OFF+8*SZCF << 8)|32,//13
//    (OFF+8*SZCF << 8)|32,//14
//    (OFF+8*SZCF << 8)|32,//15
//    (OFF+8*SZCF << 8)|32,//16
//    (OFF+8*SZCF << 8)|32,//17
//    (OFF+8*SZCF << 8)|32,//18
//    (OFF+8*SZCF << 8)|32,//19
//
//    (5<<8)| 30, (4<<8)| 30,                            //0
//    (6<<8)| 29, (3<<8)| 29,                            //1
//    (7<<8)| 29,                                        //2
//    (1<<8)| 28,                                        //3
//    (2<<8)| 27,                                        //4
//    (0<<8)| 26,                                        //5
//    (8<<8)| 26,                                        //6
//
//};
////The table total_zeros_map_7 for XScale
//static TABLE_TYPE total_zeros_map_7s2[] =
//{
//    (OFF+0*SZCF  << 8)|30,//0
//    (OFF+4*SZCF << 8)|31,//1
//    (OFF+6*SZCF << 8)|32,//2
//    (OFF+7*SZCF << 8)|32,//3
//    (OFF+8*SZCF << 8)|32,//4
//    (OFF+9*SZCF << 8)|32,//5
//    (OFF+10*SZCF << 8)|32,//6
//    (OFF+10*SZCF << 8)|32,//7
//    (OFF+10*SZCF << 8)|32,//8
//    (OFF+10*SZCF << 8)|32,//9
//    (OFF+10*SZCF << 8)|32,//10
//    (OFF+10*SZCF << 8)|32,//11
//    (OFF+10*SZCF << 8)|32,//12
//    (OFF+10*SZCF << 8)|32,//13
//    (OFF+10*SZCF << 8)|32,//14
//    (OFF+10*SZCF << 8)|32,//15
//    (OFF+10*SZCF << 8)|32,//16
//    (OFF+10*SZCF << 8)|32,//17
//    (OFF+10*SZCF << 8)|32,//18
//    (OFF+10*SZCF << 8)|32,//19
//    (OFF+10*SZCF << 8)|32,//10
//    (OFF+10*SZCF << 8)|32,//11
//    (OFF+10*SZCF << 8)|32,//12
//    (OFF+10*SZCF << 8)|32,//13
//    (OFF+10*SZCF << 8)|32,//14
//    (OFF+10*SZCF << 8)|32,//15
//    (OFF+10*SZCF << 8)|32,//16
//    (OFF+10*SZCF << 8)|32,//17
//    (OFF+10*SZCF << 8)|32,//18
//    (OFF+10*SZCF << 8)|32,//19
//
//    (3<<8)| 29,(2<<8)| 29,(5<<8)| 30, (5<<8)| 30,    //0        //80
//    (6<<8)| 29,(4<<8)| 29,                            //1        //96
//    (8<<8)| 29,                                        //2        //104
//    (7<<8)| 28,                                        //3        //108
//    (1<<8)| 27,                                        //4        //112
//    (0<<8)| 26,                                        //5        //116
//    (9<<8)| 26,                                        //6        //120
//
//};
//
////The table total_zeros_map_12 for XScale
//static TABLE_TYPE total_zeros_map_12s2[] =
//{
//    (OFF+0*SZCF   << 8)|32,//0
//    (OFF+1*SZCF   << 8)|32,//1
//    (OFF+2*SZCF   << 8)|32,//2
//    (OFF+3*SZCF  << 8)|32,//3
//    (OFF+4*SZCF  << 8)|32,//4
//    (OFF+4*SZCF  << 8)|32,//5
//    (OFF+4*SZCF  << 8)|32,//6
//    (OFF+4*SZCF  << 8)|32,//7
//    (OFF+4*SZCF  << 8)|32,//8
//    (OFF+4*SZCF  << 8)|32,//9
//    (OFF+4*SZCF  << 8)|32,//10
//    (OFF+4*SZCF  << 8)|32,//11
//    (OFF+4*SZCF  << 8)|32,//12
//    (OFF+4*SZCF  << 8)|32,//13
//    (OFF+4*SZCF  << 8)|32,//14
//    (OFF+4*SZCF  << 8)|32,//15
//    (OFF+4*SZCF  << 8)|32,//16
//    (OFF+4*SZCF  << 8)|32,//17
//    (OFF+4*SZCF  << 8)|32,//18
//    (OFF+4*SZCF  << 8)|32,//19
//    (OFF+4*SZCF  << 8)|32,//10
//    (OFF+4*SZCF  << 8)|32,//11
//    (OFF+4*SZCF  << 8)|32,//12
//    (OFF+4*SZCF  << 8)|32,//13
//    (OFF+4*SZCF  << 8)|32,//14
//    (OFF+4*SZCF  << 8)|32,//15
//    (OFF+4*SZCF  << 8)|32,//16
//    (OFF+4*SZCF  << 8)|32,//17
//    (OFF+4*SZCF  << 8)|32,//18
//    (OFF+4*SZCF  << 8)|32,//19
//
//    (3<<8)| 31,                                        //0
//    (2<<8)| 30,                                      //1
//    (4<<8)| 29,                                        //2
//    (1<<8)| 28,                                        //3
//    (0<<8)| 28,                                        //4..15
//
//};
////The table total_zeros_map_11 for XScale
//static TABLE_TYPE total_zeros_map_11s2[] =
//{
//    (OFF+0*SZCF   << 8)|32,//0
//    (OFF+1*SZCF   << 8)|31,//1
//    (OFF+3*SZCF  << 8)|32,//2
//    (OFF+4*SZCF  << 8)|32,//3
//    (OFF+5*SZCF  << 8)|32,//4
//    (OFF+5*SZCF  << 8)|32,//5
//    (OFF+5*SZCF  << 8)|32,//6
//    (OFF+5*SZCF  << 8)|32,//7
//    (OFF+5*SZCF  << 8)|32,//8
//    (OFF+5*SZCF  << 8)|32,//9
//    (OFF+5*SZCF  << 8)|32,//10
//    (OFF+5*SZCF  << 8)|32,//11
//    (OFF+5*SZCF  << 8)|32,//12
//    (OFF+5*SZCF  << 8)|32,//13
//    (OFF+5*SZCF  << 8)|32,//14
//    (OFF+5*SZCF  << 8)|32,//15
//    (OFF+5*SZCF  << 8)|32,//16
//    (OFF+5*SZCF  << 8)|32,//17
//    (OFF+5*SZCF  << 8)|32,//18
//    (OFF+5*SZCF  << 8)|32,//19
//    (OFF+5*SZCF  << 8)|32,//10
//    (OFF+5*SZCF  << 8)|32,//11
//    (OFF+5*SZCF  << 8)|32,//12
//    (OFF+5*SZCF  << 8)|32,//13
//    (OFF+5*SZCF  << 8)|32,//14
//    (OFF+5*SZCF  << 8)|32,//15
//    (OFF+5*SZCF  << 8)|32,//16
//    (OFF+5*SZCF  << 8)|32,//17
//    (OFF+5*SZCF  << 8)|32,//18
//    (OFF+5*SZCF  << 8)|32,//19
//
//    (4<<8)| 31,                                        //0
//    (3<<8)| 29, (5<<8)| 29,                         //1
//    (2<<8)| 29,                                        //2
//    (1<<8)| 28,                                        //3
//    (0<<8)| 28,                                        //4..15
//
//};
////The table total_zeros_map_10 for XScale
//static TABLE_TYPE total_zeros_map_10s2[] =
//{
//    (OFF+0*SZCF  << 8)|31,//0
//    (OFF+2*SZCF  << 8)|32,//1
//    (OFF+3*SZCF  << 8)|32,//2
//    (OFF+4*SZCF  << 8)|32,//3
//    (OFF+5*SZCF  << 8)|32,//4
//    (OFF+6*SZCF  << 8)|32,//5
//    (OFF+6*SZCF  << 8)|32,//6
//    (OFF+6*SZCF  << 8)|32,//7
//    (OFF+6*SZCF  << 8)|32,//8
//    (OFF+6*SZCF  << 8)|32,//9
//    (OFF+6*SZCF  << 8)|32,//10
//    (OFF+6*SZCF  << 8)|32,//11
//    (OFF+6*SZCF  << 8)|32,//12
//    (OFF+6*SZCF  << 8)|32,//13
//    (OFF+6*SZCF  << 8)|32,//14
//    (OFF+6*SZCF  << 8)|32,//15
//    (OFF+6*SZCF  << 8)|32,//16
//    (OFF+6*SZCF  << 8)|32,//17
//    (OFF+6*SZCF  << 8)|32,//18
//    (OFF+6*SZCF  << 8)|32,//19
//    (OFF+6*SZCF  << 8)|32,//10
//    (OFF+6*SZCF  << 8)|32,//11
//    (OFF+6*SZCF  << 8)|32,//12
//    (OFF+6*SZCF  << 8)|32,//13
//    (OFF+6*SZCF  << 8)|32,//14
//    (OFF+6*SZCF  << 8)|32,//15
//    (OFF+6*SZCF  << 8)|32,//16
//    (OFF+6*SZCF  << 8)|32,//17
//    (OFF+6*SZCF  << 8)|32,//18
//    (OFF+6*SZCF  << 8)|32,//19
//
//    (4<<8)| 30, (3<<8)| 30,                            //0
//    (5<<8)| 30,                                     //1
//    (2<<8)| 29,                                        //2
//    (6<<8)| 28,                                        //3
//    (0<<8)| 27,                                        //4
//    (1<<8)| 27,                                        //5..15
//
//};
////The table total_zeros_map_cr1 for XScale
//static TABLE_TYPE total_zeros_map_cr1s2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|32,//1
//    (OFF+2*SZCF  << 8)|32,//2
//    (OFF+3*SZCF  << 8)|32,//3
//    (OFF+3*SZCF  << 8)|32,//4
//    (OFF+3*SZCF  << 8)|32,//5
//    (OFF+3*SZCF  << 8)|32,//6
//    (OFF+3*SZCF  << 8)|32,//7
//    (OFF+3*SZCF  << 8)|32,//8
//    (OFF+3*SZCF  << 8)|32,//9
//    (OFF+3*SZCF  << 8)|32,//10
//    (OFF+3*SZCF  << 8)|32,//11
//    (OFF+3*SZCF  << 8)|32,//12
//    (OFF+3*SZCF  << 8)|32,//13
//    (OFF+3*SZCF  << 8)|32,//14
//    (OFF+3*SZCF  << 8)|32,//15
//    (OFF+3*SZCF  << 8)|32,//16
//    (OFF+3*SZCF  << 8)|32,//17
//    (OFF+3*SZCF  << 8)|32,//18
//    (OFF+3*SZCF  << 8)|32,//19
//    (OFF+3*SZCF  << 8)|32,//10
//    (OFF+3*SZCF  << 8)|32,//11
//    (OFF+3*SZCF  << 8)|32,//12
//    (OFF+3*SZCF  << 8)|32,//13
//    (OFF+3*SZCF  << 8)|32,//14
//    (OFF+3*SZCF  << 8)|32,//15
//    (OFF+3*SZCF  << 8)|32,//16
//    (OFF+3*SZCF  << 8)|32,//17
//    (OFF+3*SZCF  << 8)|32,//18
//    (OFF+3*SZCF  << 8)|32,//19
//
//    (0<<8)| 31,                                        //0
//    (1<<8)| 30,                                        //1
//    (2<<8)| 29,                                        //2
//    (3<<8)| 29,                                        //3..15
//
//};
////The table total_zeros_map_15 for XScale
//static TABLE_TYPE total_zeros_map_15s2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|32,//1
//    (OFF+1*SZCF  << 8)|32,//2
//    (OFF+1*SZCF  << 8)|32,//3
//    (OFF+1*SZCF  << 8)|32,//4
//    (OFF+1*SZCF  << 8)|32,//5
//    (OFF+1*SZCF  << 8)|32,//6
//    (OFF+1*SZCF  << 8)|32,//7
//    (OFF+1*SZCF  << 8)|32,//8
//    (OFF+1*SZCF  << 8)|32,//9
//    (OFF+1*SZCF  << 8)|32,//10
//    (OFF+1*SZCF  << 8)|32,//11
//    (OFF+1*SZCF  << 8)|32,//12
//    (OFF+1*SZCF  << 8)|32,//13
//    (OFF+1*SZCF  << 8)|32,//14
//    (OFF+1*SZCF  << 8)|32,//15
//    (OFF+1*SZCF  << 8)|32,//16
//    (OFF+1*SZCF  << 8)|32,//17
//    (OFF+1*SZCF  << 8)|32,//18
//    (OFF+1*SZCF  << 8)|32,//19
//    (OFF+1*SZCF  << 8)|32,//20
//    (OFF+1*SZCF  << 8)|32,//21
//    (OFF+1*SZCF  << 8)|32,//22
//    (OFF+1*SZCF  << 8)|32,//23
//    (OFF+1*SZCF  << 8)|32,//24
//    (OFF+1*SZCF  << 8)|32,//25
//    (OFF+1*SZCF  << 8)|32,//26
//    (OFF+1*SZCF  << 8)|32,//27
//    (OFF+1*SZCF  << 8)|32,//28
//    (OFF+1*SZCF  << 8)|32,//29
//
//    (1<<8)| 31,                                        //0
//    (0<<8)| 31,                                        //1..15
//
//};
////The table total_zeros_map_14 for XScale
//static TABLE_TYPE total_zeros_map_14s2[] =
//{
//    (OFF+0*SZCF   << 8)|32,//0
//    (OFF+1*SZCF   << 8)|32,//1
//    (OFF+2*SZCF   << 8)|32,//2
//    (OFF+2*SZCF  << 8)|32,//3
//    (OFF+2*SZCF  << 8)|32,//4
//    (OFF+2*SZCF  << 8)|32,//5
//    (OFF+2*SZCF  << 8)|32,//6
//    (OFF+2*SZCF  << 8)|32,//7
//    (OFF+2*SZCF  << 8)|32,//8
//    (OFF+2*SZCF  << 8)|32,//9
//    (OFF+2*SZCF  << 8)|32,//10
//    (OFF+2*SZCF  << 8)|32,//11
//    (OFF+2*SZCF  << 8)|32,//12
//    (OFF+2*SZCF  << 8)|32,//13
//    (OFF+2*SZCF  << 8)|32,//14
//    (OFF+2*SZCF  << 8)|32,//15
//    (OFF+2*SZCF  << 8)|32,//16
//    (OFF+2*SZCF  << 8)|32,//17
//    (OFF+2*SZCF  << 8)|32,//18
//    (OFF+2*SZCF  << 8)|32,//19
//    (OFF+2*SZCF  << 8)|32,//10
//    (OFF+2*SZCF  << 8)|32,//11
//    (OFF+2*SZCF  << 8)|32,//12
//    (OFF+2*SZCF  << 8)|32,//13
//    (OFF+2*SZCF  << 8)|32,//14
//    (OFF+2*SZCF  << 8)|32,//15
//    (OFF+2*SZCF  << 8)|32,//16
//    (OFF+2*SZCF  << 8)|32,//17
//    (OFF+2*SZCF  << 8)|32,//18
//    (OFF+2*SZCF  << 8)|32,//19
//
//    (2<<8)| 31,                                        //0
//    (1<<8)| 30,                                      //1
//    (0<<8)| 30,                                        //2..15
//
//};
////The table total_zeros_map_13 for XScale
//static TABLE_TYPE total_zeros_map_13s2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|32,//1
//    (OFF+2*SZCF  << 8)|32,//2
//    (OFF+3*SZCF  << 8)|32,//3
//    (OFF+3*SZCF  << 8)|32,//4
//    (OFF+3*SZCF  << 8)|32,//5
//    (OFF+3*SZCF  << 8)|32,//6
//    (OFF+3*SZCF  << 8)|32,//7
//    (OFF+3*SZCF  << 8)|32,//8
//    (OFF+3*SZCF  << 8)|32,//9
//    (OFF+3*SZCF  << 8)|32,//10
//    (OFF+3*SZCF  << 8)|32,//11
//    (OFF+3*SZCF  << 8)|32,//12
//    (OFF+3*SZCF  << 8)|32,//13
//    (OFF+3*SZCF  << 8)|32,//14
//    (OFF+3*SZCF  << 8)|32,//15
//    (OFF+3*SZCF  << 8)|32,//16
//    (OFF+3*SZCF  << 8)|32,//17
//    (OFF+3*SZCF  << 8)|32,//18
//    (OFF+3*SZCF  << 8)|32,//19
//    (OFF+3*SZCF  << 8)|32,//10
//    (OFF+3*SZCF  << 8)|32,//11
//    (OFF+3*SZCF  << 8)|32,//12
//    (OFF+3*SZCF  << 8)|32,//13
//    (OFF+3*SZCF  << 8)|32,//14
//    (OFF+3*SZCF  << 8)|32,//15
//    (OFF+3*SZCF  << 8)|32,//16
//    (OFF+3*SZCF  << 8)|32,//17
//    (OFF+3*SZCF  << 8)|32,//18
//    (OFF+3*SZCF  << 8)|32,//19
//
//    (2<<8)| 31,                                        //0
//    (3<<8)| 30,                                      //1
//    (1<<8)| 29,                                        //2
//    (0<<8)| 29,                                        //3..15
//
//};
//
////The table total_zeros_map_cr3 for XScale
//static TABLE_TYPE total_zeros_map_cr3s2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|32,//1
//    (OFF+1*SZCF  << 8)|32,//2
//    (OFF+1*SZCF  << 8)|32,//3
//    (OFF+1*SZCF  << 8)|32,//4
//    (OFF+1*SZCF  << 8)|32,//5
//    (OFF+1*SZCF  << 8)|32,//6
//    (OFF+1*SZCF  << 8)|32,//7
//    (OFF+1*SZCF  << 8)|32,//8
//    (OFF+1*SZCF  << 8)|32,//9
//    (OFF+1*SZCF  << 8)|32,//10
//    (OFF+1*SZCF  << 8)|32,//11
//    (OFF+1*SZCF  << 8)|32,//12
//    (OFF+1*SZCF  << 8)|32,//13
//    (OFF+1*SZCF  << 8)|32,//14
//    (OFF+1*SZCF  << 8)|32,//15
//    (OFF+1*SZCF  << 8)|32,//16
//    (OFF+1*SZCF  << 8)|32,//17
//    (OFF+1*SZCF  << 8)|32,//18
//    (OFF+1*SZCF  << 8)|32,//19
//    (OFF+1*SZCF  << 8)|32,//10
//    (OFF+1*SZCF  << 8)|32,//11
//    (OFF+1*SZCF  << 8)|32,//12
//    (OFF+1*SZCF  << 8)|32,//13
//    (OFF+1*SZCF  << 8)|32,//14
//    (OFF+1*SZCF  << 8)|32,//15
//    (OFF+1*SZCF  << 8)|32,//16
//    (OFF+1*SZCF  << 8)|32,//17
//    (OFF+1*SZCF  << 8)|32,//18
//    (OFF+1*SZCF  << 8)|32,//19
//
//    (0<<8)| 31,                                        //0
//    (1<<8)| 31,                                        //1..15
//
//};
////The table total_zeros_map_cr2 for XScale
//static TABLE_TYPE total_zeros_map_cr2s2[] =
//{
//    (OFF+0*SZCF  << 8)|32,//0
//    (OFF+1*SZCF  << 8)|32,//1
//    (OFF+2*SZCF  << 8)|32,//2
//    (OFF+2*SZCF  << 8)|32,//3
//    (OFF+2*SZCF  << 8)|32,//4
//    (OFF+2*SZCF  << 8)|32,//5
//    (OFF+2*SZCF  << 8)|32,//6
//    (OFF+2*SZCF  << 8)|32,//7
//    (OFF+2*SZCF  << 8)|32,//8
//    (OFF+2*SZCF  << 8)|32,//9
//    (OFF+2*SZCF  << 8)|32,//10
//    (OFF+2*SZCF  << 8)|32,//11
//    (OFF+2*SZCF  << 8)|32,//12
//    (OFF+2*SZCF  << 8)|32,//13
//    (OFF+2*SZCF  << 8)|32,//14
//    (OFF+2*SZCF  << 8)|32,//15
//    (OFF+2*SZCF  << 8)|32,//16
//    (OFF+2*SZCF  << 8)|32,//17
//    (OFF+2*SZCF  << 8)|32,//18
//    (OFF+2*SZCF  << 8)|32,//19
//    (OFF+2*SZCF  << 8)|32,//10
//    (OFF+2*SZCF  << 8)|32,//11
//    (OFF+2*SZCF  << 8)|32,//12
//    (OFF+2*SZCF  << 8)|32,//13
//    (OFF+2*SZCF  << 8)|32,//14
//    (OFF+2*SZCF  << 8)|32,//15
//    (OFF+2*SZCF  << 8)|32,//16
//    (OFF+2*SZCF  << 8)|32,//17
//    (OFF+2*SZCF  << 8)|32,//18
//    (OFF+2*SZCF  << 8)|32,//19
//
//    (0<<8)| 31,                                        //0
//    (1<<8)| 30,                                        //1
//    (2<<8)| 30,                                        //2..15
//
//};
//
//#endif
//
//#undef OFF

} // end namespace UMC

#endif //__UMC_H264_DEC_TOTAL_ZEROS_H__
