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
#ifndef __UMC_H264_DEC_RUN_BEFORE_H
#define __UMC_H264_DEC_RUN_BEFORE_H

namespace UMC
{

//#if !defined (ARM) && !defined (_ARM_)
#ifdef __INTEL_IPP__
// Table 9-10 - Tables for run_before
// zerosLeft = 1
/*
0 1
1 0
*/

static
Ipp32s run_before_map_1[] =
{
1, /* max bits */
1,  /* total subtables */
1,/* subtable sizes */

2, /* 1-bit codes */
0x0001, 0, 0x0000, 1,

-1
};

// Table 9-10 - Tables for run_before
// zerosLeft = 2
/*
0    1

1    01
2    00
*/

static
Ipp32s run_before_map_2[] =
{
2, /* max bits */
1,  /* total subtables */
2,/* subtable sizes */

1, /* 1-bit codes */
0x0001, 0,

2, /* 2-bit codes */
0x0001, 1, 0x0000, 2,

-1
};


// Table 9-10 - Tables for run_before
// zerosLeft = 3
/*
3    00
2    01
1    10
0    11
*/

static
Ipp32s run_before_map_3[] =
{
2, /* max bits */
1,  /* total subtables */
2,/* subtable sizes */

0, /* 1-bit codes */

4, /* 2-bit codes */
0x0000, 3, 0x0001, 2, 0x0002, 1, 0x0003, 0,

-1
};

// Table 9-10 - Tables for run_before
// zerosLeft = 4
/*
2    01
1    10
0    11

4    000
3    001
*/

static
Ipp32s run_before_map_4[] =
{
3, /* max bits */
1,  /* total subtables */
3,/* subtable sizes */

0, /* 1-bit codes */

3, /* 2-bit codes */
0x0001, 2, 0x0002, 1, 0x0003, 0,

2, /* 3-bit codes */
0x0000, 4, 0x0001, 3,

-1
};

// Table 9-10 - Tables for run_before
// zerosLeft = 5
/*
1    10
0    11

5    000
4    001
3    010
2    011
*/

static
Ipp32s run_before_map_5[] =
{
3, /* max bits */
1,  /* total subtables */
3,/* subtable sizes */

0, /* 1-bit codes */

2, /* 2-bit codes */
0x0002, 1, 0x0003, 0,

4, /* 3-bit codes */
0x0000, 5, 0x0001, 4, 0x0002, 3, 0x0003, 2,

-1
};

// Table 9-10 - Tables for run_before
// zerosLeft = 6
/*
0    11

1    000
2    001
4    010
3    011
6    100
5    101

*/

static
Ipp32s run_before_map_6[] =
{
3, /* max bits */
1,  /* total subtables */
3,/* subtable sizes */

0, /* 1-bit codes */

1, /* 2-bit codes */
0x0003, 0,

6, /* 3-bit codes */
0x0000, 1, 0x0001, 2, 0x0002, 4, 0x0003, 3,
0x0004, 6, 0x0005, 5,

-1
};


// Table 9-10 - Tables for run_before
// zerosLeft > 6
/*
6    001
5    010
4    011
3    100
2    101
1    110
0    111

7    0001

8    0000 1

9    0000 01

10    0000 001

11    0000 0001

12    0000 0000 1

13    0000 0000 01

14    0000 0000 001

*/

static
Ipp32s run_before_map_6p[] =
{
11, /* max bits */
2,  /* total subtables */
6,5,/* subtable sizes */

0, /* 1-bit codes */
0, /* 2-bit codes */

7, /* 3-bit codes */
0x0001, 6, 0x0002, 5, 0x0003, 4, 0x0004, 3,
0x0005, 2, 0x0006, 1, 0x0007, 0,

1, /* 4-bit codes */
0x0001, 7,

1, /* 5-bit codes */
0x0001, 8,

1, /* 6-bit codes */
0x0001, 9,

1, /* 7-bit codes */
0x0001, 10,

1, /* 8-bit codes */
0x0001, 11,

1, /* 9-bit codes */
0x0001, 12,

1, /* 10-bit codes */
0x0001, 13,

1, /* 11-bit codes */
0x0001, 14,

-1
};
#endif
/*
#undef OFF
#define OFF 120//60//

#undef SZCF
#define SZCF 4//2//

#undef SHIFT1
#define SHIFT1 8

#undef TABLE_TYPE
#define TABLE_TYPE int //unsigned short
*/

//#else
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
////The table run_before_map_1 for XScale
//static TABLE_TYPE run_before_map_1s2[] =
//{
//    (OFF+0  << 8)|32,//0
//    (OFF+SZCF  << 8)|32,//1
//    (OFF+SZCF  << 8)|32,//2
//    (OFF+SZCF  << 8)|32,//3
//    (OFF+SZCF  << 8)|32,//4
//    (OFF+SZCF  << 8)|32,//5
//    (OFF+SZCF  << 8)|32,//6
//    (OFF+SZCF  << 8)|32,//7
//    (OFF+SZCF  << 8)|32,//8
//    (OFF+SZCF  << 8)|32,//9
//    (OFF+SZCF  << 8)|32,//10
//    (OFF+SZCF  << 8)|32,//11
//    (OFF+SZCF  << 8)|32,//12
//    (OFF+SZCF  << 8)|32,//13
//    (OFF+SZCF  << 8)|32,//14
//    (OFF+SZCF  << 8)|32,//15
//    (OFF+SZCF  << 8)|32,//16
//    (OFF+SZCF  << 8)|32,//17
//    (OFF+SZCF  << 8)|32,//18
//    (OFF+SZCF  << 8)|32,//19
//    (OFF+SZCF  << 8)|32,//20
//    (OFF+SZCF  << 8)|32,//21
//    (OFF+SZCF  << 8)|32,//22
//    (OFF+SZCF  << 8)|32,//23
//    (OFF+SZCF  << 8)|32,//24
//    (OFF+SZCF  << 8)|32,//25
//    (OFF+SZCF  << 8)|32,//26
//    (OFF+SZCF  << 8)|32,//27
//    (OFF+SZCF  << 8)|32,//28
//    (OFF+SZCF  << 8)|32,//29
//
//    (0<< SHIFT1)| 31,    //0
//    (1<< SHIFT1)| 31, //0..16
//
//};
//
////The table run_before_map_4 for XScale
//static TABLE_TYPE run_before_map_4s2[] =
//{
//    (OFF+0*SZCF  << 8)|31,//0
//    (OFF+2*SZCF  << 8)|32,//1
//    (OFF+3*SZCF  << 8)|32,//2
//    (OFF+4*SZCF  << 8)|32,//3
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
//    (OFF+4*SZCF  << 8)|32,//20
//    (OFF+4*SZCF  << 8)|32,//21
//    (OFF+4*SZCF  << 8)|32,//22
//    (OFF+4*SZCF  << 8)|32,//23
//    (OFF+4*SZCF  << 8)|32,//24
//    (OFF+4*SZCF  << 8)|32,//25
//    (OFF+4*SZCF  << 8)|32,//26
//    (OFF+4*SZCF  << 8)|32,//27
//    (OFF+4*SZCF  << 8)|32,//28
//    (OFF+4*SZCF  << 8)|32,//29
//
//    (1<< SHIFT1)| 30, (0<< SHIFT1)| 30,    //0
//    (2<< SHIFT1)| 30,                //1
//    (3<< SHIFT1)| 29,                //2
//    (4<< SHIFT1)| 29,                //3..16
//
//};
////The table run_before_map_3 for XScale
//static TABLE_TYPE run_before_map_3s2[] =
//{
//    (OFF+0*SZCF  << 8)|31,//0
//    (OFF+2*SZCF  << 8)|32,//1
//    (OFF+3*SZCF  << 8)|32,//2
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
//    (OFF+3*SZCF  << 8)|32,//20
//    (OFF+3*SZCF  << 8)|32,//21
//    (OFF+3*SZCF  << 8)|32,//22
//    (OFF+3*SZCF  << 8)|32,//23
//    (OFF+3*SZCF  << 8)|32,//24
//    (OFF+3*SZCF  << 8)|32,//25
//    (OFF+3*SZCF  << 8)|32,//26
//    (OFF+3*SZCF  << 8)|32,//27
//    (OFF+3*SZCF  << 8)|32,//28
//    (OFF+3*SZCF  << 8)|32,//29
//
//    (1<< SHIFT1)| 30, (0<< SHIFT1)| 30,    //0
//    (2<< SHIFT1)| 30,                //1
//    (3<< SHIFT1)| 30,                //2..16
//
//};
////The table run_before_map_2 for XScale
//static TABLE_TYPE run_before_map_2s2[] =
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
//    (OFF+2*SZCF  << 8)|32,//20
//    (OFF+2*SZCF  << 8)|32,//21
//    (OFF+2*SZCF  << 8)|32,//22
//    (OFF+2*SZCF  << 8)|32,//23
//    (OFF+2*SZCF  << 8)|32,//24
//    (OFF+2*SZCF  << 8)|32,//25
//    (OFF+2*SZCF  << 8)|32,//26
//    (OFF+2*SZCF  << 8)|32,//27
//    (OFF+2*SZCF  << 8)|32,//28
//    (OFF+2*SZCF  << 8)|32,//29
//
//    (0<< SHIFT1)| 31,    //0
//    (1<< SHIFT1)| 30, //1
//    (2<< SHIFT1)| 30, //2..16
//
//};
//
////The table run_before_map_6 for XScale
//static TABLE_TYPE run_before_map_6s2[] =
//{
//    (OFF+0*SZCF   << 8)|30,//0
//    (OFF+4*SZCF   << 8)|31,//1
//    (OFF+6*SZCF  << 8)|32,//2
//    (OFF+7*SZCF  << 8)|32,//3
//    (OFF+7*SZCF  << 8)|32,//4
//    (OFF+7*SZCF  << 8)|32,//5
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
//    (OFF+7*SZCF  << 8)|32,//20
//    (OFF+7*SZCF  << 8)|32,//21
//    (OFF+7*SZCF  << 8)|32,//22
//    (OFF+7*SZCF  << 8)|32,//23
//    (OFF+7*SZCF  << 8)|32,//24
//    (OFF+7*SZCF  << 8)|32,//25
//    (OFF+7*SZCF  << 8)|32,//26
//    (OFF+7*SZCF  << 8)|32,//27
//    (OFF+7*SZCF  << 8)|32,//28
//    (OFF+7*SZCF  << 8)|32,//29
//
//    (6<< SHIFT1)| 29, (5<< SHIFT1)| 29, (0<< SHIFT1)| 30, (0<< SHIFT1)| 30,    //0
//    (4<< SHIFT1)| 29, (3<< SHIFT1)| 29,                            //1
//    (2<< SHIFT1)| 29,                                        //2
//    (1<< SHIFT1)| 29,                                        //3..16
//
//};
////The table run_before_map_5 for XScale
//static TABLE_TYPE run_before_map_5s2[] =
//{
//    (OFF+0*SZCF   << 8)|31,//0
//    (OFF+2*SZCF   << 8)|31,//1
//    (OFF+4*SZCF   << 8)|32,//2
//    (OFF+5*SZCF  << 8)|32,//3
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
//    (OFF+5*SZCF  << 8)|32,//20
//    (OFF+5*SZCF  << 8)|32,//21
//    (OFF+5*SZCF  << 8)|32,//22
//    (OFF+5*SZCF  << 8)|32,//23
//    (OFF+5*SZCF  << 8)|32,//24
//    (OFF+5*SZCF  << 8)|32,//25
//    (OFF+5*SZCF  << 8)|32,//26
//    (OFF+5*SZCF  << 8)|32,//27
//    (OFF+5*SZCF  << 8)|32,//28
//    (OFF+5*SZCF  << 8)|32,//29
//
//    (1<< SHIFT1)| 30, (0<< SHIFT1)| 30,    //0
//    (3<< SHIFT1)| 29,    (2<< SHIFT1)| 29,    //1
//    (4<< SHIFT1)| 29,                //2
//    (5<< SHIFT1)| 29,                //3..16
//
//};
////The table run_before_map_6 for XScale
//static TABLE_TYPE run_before_map_6ps2[] =
//{
//    (OFF+0*SZCF  << 8)|30,//0
//    (OFF+4*SZCF  << 8)|31,//1
//    (OFF+6*SZCF  << 8)|32,//2
//    (OFF+7*SZCF  << 8)|32,//3
//    (OFF+8*SZCF  << 8)|32,//4
//    (OFF+9*SZCF  << 8)|32,//5
//    (OFF+10*SZCF << 8)|32,//6
//    (OFF+11*SZCF << 8)|32,//7
//    (OFF+12*SZCF << 8)|32,//8
//    (OFF+13*SZCF << 8)|32,//9
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
//    (OFF+14*SZCF << 8)|32,//20
//    (OFF+14*SZCF << 8)|32,//21
//    (OFF+14*SZCF << 8)|32,//22
//    (OFF+14*SZCF << 8)|32,//23
//    (OFF+14*SZCF << 8)|32,//24
//    (OFF+14*SZCF << 8)|32,//25
//    (OFF+14*SZCF << 8)|32,//26
//    (OFF+14*SZCF << 8)|32,//27
//    (OFF+14*SZCF << 8)|32,//28
//    (OFF+14*SZCF << 8)|32,//29
//
//    (3<< SHIFT1)| 29, (2<< SHIFT1)| 29, (1<< SHIFT1)| 29, (0<< SHIFT1)| 29,    //0
//    (5<< SHIFT1)| 29,    (4<< SHIFT1)| 29,                            //1
//    (6<< SHIFT1)| 29,                                        //2
//    (7<< SHIFT1)| 28,                                        //3
//    (8<< SHIFT1)| 27,                                        //4
//    (9<< SHIFT1)| 26,                                        //5
//    (10<< SHIFT1)|25,                                        //6
//    (11<< SHIFT1)|24,                                        //7
//    (12<< SHIFT1)|23,                                        //8
//    (13<< SHIFT1)|22,                                        //9
//    (14<< SHIFT1)|21,                                        //10
//    IPPVC_VLC_FORBIDDEN
//
//};
//
//#endif

#undef OFF

} // end namespace UMC

#endif //__UMC_H264_DEC_RUN_BEFORE_H
