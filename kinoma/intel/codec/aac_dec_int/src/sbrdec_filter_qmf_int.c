/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
/********************************************************************/
//**
#include "kinoma_ipp_lib.h"

#include<ipps.h>
#include<ippac.h>
#include<math.h>
#include<string.h>

#include "aaccmn_chmap.h"
#include "sbrdec.h"
#include "sbrdec_api_int.h"
#include "sbrdec_tables_int.h"
#include "sbrdec_own_int.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifdef KINOMA_DEBUG
extern int g_kinoma_debug_on;
#endif


/*******************************************************************/

/* Q30 */
int tblPreProcCosSinDCT4_64[] = {

  0x3ffec42d, 0xff36f171,
  0x3fe12acb, 0xfc135231,
  0x3f9c2bfa, 0xf8f21e8f,
  0x3f2ff249, 0xf5d544a8,
  0x3e9cc076, 0xf2beafee,
  0x3de2f147, 0xefb047f2,
  0x3d02f756, 0xecabef3e,
  0x3bfd5cc4, 0xe9b38223,
  0x3ad2c2e7, 0xe6c8d59d,
  0x3983e1e7, 0xe3edb628,
  0x3811884c, 0xe123e6ae,
  0x367c9a7d, 0xde6d1f66,
  0x34c61236, 0xdbcb0cce,
  0x32eefde9, 0xd93f4e9e,
  0x30f8801f, 0xd6cb76c9,
  0x2ee3cebe, 0xd4710884,
  0x2cb2324b, 0xd2317757,
  0x2a650525, 0xd00e263a,
  0x27fdb2a6, 0xce0866b9,
  0x257db64b, 0xcc217822,
  0x22e69ac7, 0xca5a86c4,
  0x2039f90e, 0xc8b4ab32,
  0x1d79775b, 0xc730e997,
  0x1aa6c82b, 0xc5d03118,
  0x17c3a931, 0xc4935b3c,
  0x14d1e242, 0xc37b2b6a,
  0x11d3443f, 0xc2884e6f,
  0xec9a7f2 , 0xc1bb5a12,
  0xbb6ecef , 0xc114ccb9,
  0x89cf867 , 0xc0950d1d,
  0x57db402 , 0xc03c6a07,
  0x25b0cae , 0xc00b1a21

};

int tblPostProcCosSinDCT4_64[] =
{

  0x40000000, 0         ,
  0x3fec43c0, 0xfcdc1340,
  0x3fb11b40, 0xf9ba1650,
  0x3f4eab00, 0xf69bf7d0,
  0x3ec52f80, 0xf383a3e0,
  0x3e14fe00, 0xf0730340,
  0x3d3e82c0, 0xed6bf9e0,
  0x3c424200, 0xea706580,
  0x3b20d780, 0xe7821d60,
  0x39daf600, 0xe4a2f000,
  0x38716600, 0xe1d4a2c0,
  0x36e50680, 0xdf18f0c0,
  0x3536cc40, 0xdc718980,
  0x3367c080, 0xd9e01000,
  0x317900c0, 0xd76619c0,
  0x2f6bbe40, 0xd5052d80,
  0x2d413cc0, 0xd2bec340,
  0x2afad280, 0xd09441c0,
  0x2899e640, 0xce86ff40,
  0x261ff000, 0xcc983f80,
  0x238e7680, 0xcac933c0,
  0x20e70f40, 0xc91af980,
  0x1e2b5d40, 0xc78e9a00,
  0x1b5d1000, 0xc6250a00,
  0x187de2a0, 0xc4df2880,
  0x158f9a80, 0xc3bdbe00,
  0x12940620, 0xc2c17d40,
  0xf8cfcc0 , 0xc1eb0200,
  0xc7c5c20 , 0xc13ad080,
  0x9640830 , 0xc0b15500,
  0x645e9b0 , 0xc04ee4c0,
  0x323ecc0 , 0xc013bc40

};

Ipp32sc tblPreQMF_SD_HQ[] = {

{0x40000000, 0         },
{0x3ffb10c1, 0x192155f },
{0x3fec43c6, 0x323ecbe },
{0x3fd39b5a, 0x4b54824 },
{0x3fb11b47, 0x645e9af },
{0x3f84c8e1, 0x7d59395 },
{0x3f4eaafe, 0x9640837 },
{0x3f0ec9f4, 0xaf10a22 },
{0x3ec52f9f, 0xc7c5c1e },
{0x3e71e758, 0xe05c135 },
{0x3e14fdf7, 0xf8cfcbd },
{0x3dae81ce, 0x1111d262},
{0x3d3e82ad, 0x1294062e},
{0x3cc511d8, 0x14135c94},
{0x3c424209, 0x158f9a75},
{0x3bb6276d, 0x17088530},
{0x3b20d79e, 0x187de2a6},
{0x3a8269a2, 0x19ef7943},
{0x39daf5e8, 0x1b5d1009},
{0x392a9642, 0x1cc66e99},
{0x387165e3, 0x1e2b5d38},
{0x37af8158, 0x1f8ba4db},
{0x36e5068a, 0x20e70f32},
{0x361214b0, 0x223d66a8},
{0x3536cc52, 0x238e7673},
{0x34534f40, 0x24da0a99},
{0x3367c08f, 0x261feff9},
{0x32744493, 0x275ff452},
{0x317900d6, 0x2899e64a},
{0x30761c17, 0x29cd9577},
{0x2f6bbe44, 0x2afad269},
{0x2e5a106f, 0x2c216eaa}
};

Ipp32sc tblPostQMF_SD_HQ[] = {

{0xc0013bd3, 0xff36f171},
{0xc01ed535, 0xfc135231},
{0xc063d406, 0xf8f21e8f},
{0xc0d00db7, 0xf5d544a8},
{0xc1633f8a, 0xf2beafee},
{0xc21d0eb9, 0xefb047f2},
{0xc2fd08aa, 0xecabef3e},
{0xc402a33c, 0xe9b38223},
{0xc52d3d19, 0xe6c8d59d},
{0xc67c1e19, 0xe3edb628},
{0xc7ee77b4, 0xe123e6ae},
{0xc9836583, 0xde6d1f66},
{0xcb39edca, 0xdbcb0cce},
{0xcd110217, 0xd93f4e9e},
{0xcf077fe1, 0xd6cb76c9},
{0xd11c3142, 0xd4710884},
{0xd34dcdb5, 0xd2317757},
{0xd59afadb, 0xd00e263a},
{0xd8024d5a, 0xce0866b9},
{0xda8249b5, 0xcc217822},
{0xdd196539, 0xca5a86c4},
{0xdfc606f2, 0xc8b4ab32},
{0xe28688a5, 0xc730e997},
{0xe55937d5, 0xc5d03118},
{0xe83c56cf, 0xc4935b3c},
{0xeb2e1dbe, 0xc37b2b6a},
{0xee2cbbc1, 0xc2884e6f},
{0xf136580e, 0xc1bb5a12},
{0xf4491311, 0xc114ccb9},
{0xf7630799, 0xc0950d1d},
{0xfa824bfe, 0xc03c6a07},
{0xfda4f352, 0xc00b1a21},
{0xc90e8f  , 0xc0013bd3},
{0x3ecadcf , 0xc01ed535},
{0x70de171 , 0xc063d406},
{0xa2abb58 , 0xc0d00db7},
{0xd415012 , 0xc1633f8a},
{0x104fb80e, 0xc21d0eb9},
{0x135410c2, 0xc2fd08aa},
{0x164c7ddd, 0xc402a33c},
{0x19372a63, 0xc52d3d19},
{0x1c1249d8, 0xc67c1e19},
{0x1edc1952, 0xc7ee77b4},
{0x2192e09a, 0xc9836583},
{0x2434f332, 0xcb39edca},
{0x26c0b162, 0xcd110217},
{0x29348937, 0xcf077fe1},
{0x2b8ef77c, 0xd11c3142},
{0x2dce88a9, 0xd34dcdb5},
{0x2ff1d9c6, 0xd59afadb},
{0x31f79947, 0xd8024d5a},
{0x33de87de, 0xda8249b5},
{0x35a5793c, 0xdd196539},
{0x374b54ce, 0xdfc606f2},
{0x38cf1669, 0xe28688a5},
{0x3a2fcee8, 0xe55937d5},
{0x3b6ca4c4, 0xe83c56cf},
{0x3c84d496, 0xeb2e1dbe},
{0x3d77b191, 0xee2cbbc1},
{0x3e44a5ee, 0xf136580e},
{0x3eeb3347, 0xf4491311},
{0x3f6af2e3, 0xf7630799},
{0x3fc395f9, 0xfa824bfe},
{0x3ff4e5df, 0xfda4f352}

};

Ipp32sc tblPreQMF_A_LP[] = {

{0xd2bec334, 0xd2bec334},
{0xd5052d97, 0xd09441bc},
{0xd76619b6, 0xce86ff2a},
{0xd9e01007, 0xcc983f71},
{0xdc71898d, 0xcac933ae},
{0xdf18f0ce, 0xc91af976},
{0xe1d4a2c8, 0xc78e9a1d},
{0xe4a2eff7, 0xc6250a18},
{0xe7821d5a, 0xc4df2862},
{0xea70658b, 0xc3bdbdf7},
{0xed6bf9d2, 0xc2c17d53},
{0xf0730343, 0xc1eb0209},
{0xf383a3e2, 0xc13ad061},
{0xf69bf7c9, 0xc0b15502},
{0xf9ba1651, 0xc04ee4b9},
{0xfcdc1342, 0xc013bc3a},
{0         , 0xc0000000},
{0x323ecbe , 0xc013bc3a},
{0x645e9af , 0xc04ee4b9},
{0x9640837 , 0xc0b15502},
{0xc7c5c1e , 0xc13ad061},
{0xf8cfcbd , 0xc1eb0209},
{0x1294062e, 0xc2c17d53},
{0x158f9a75, 0xc3bdbdf7},
{0x187de2a6, 0xc4df2862},
{0x1b5d1009, 0xc6250a18},
{0x1e2b5d38, 0xc78e9a1d},
{0x20e70f32, 0xc91af976},
{0x238e7673, 0xcac933ae},
{0x261feff9, 0xcc983f71},
{0x2899e64a, 0xce86ff2a},
{0x2afad269, 0xd09441bc},
{0x2d413ccc, 0xd2bec334},
{0x2f6bbe44, 0xd5052d97},
{0x317900d6, 0xd76619b6},
{0x3367c08f, 0xd9e01007},
{0x3536cc52, 0xdc71898d},
{0x36e5068a, 0xdf18f0ce},
{0x387165e3, 0xe1d4a2c8},
{0x39daf5e8, 0xe4a2eff7},
{0x3b20d79e, 0xe7821d5a},
{0x3c424209, 0xea70658b},
{0x3d3e82ad, 0xed6bf9d2},
{0x3e14fdf7, 0xf0730343},
{0x3ec52f9f, 0xf383a3e2},
{0x3f4eaafe, 0xf69bf7c9},
{0x3fb11b47, 0xf9ba1651},
{0x3fec43c6, 0xfcdc1342},
{0x40000000, 0         },
{0x3fec43c6, 0x323ecbe },
{0x3fb11b47, 0x645e9af },
{0x3f4eaafe, 0x9640837 },
{0x3ec52f9f, 0xc7c5c1e },
{0x3e14fdf7, 0xf8cfcbd },
{0x3d3e82ad, 0x1294062e},
{0x3c424209, 0x158f9a75},
{0x3b20d79e, 0x187de2a6},
{0x39daf5e8, 0x1b5d1009},
{0x387165e3, 0x1e2b5d38},
{0x36e5068a, 0x20e70f32},
{0x3536cc52, 0x238e7673},
{0x3367c08f, 0x261feff9},
{0x317900d6, 0x2899e64a},
{0x2f6bbe44, 0x2afad269}

};

Ipp32sc tblPostQMF_A_LP[] = {

{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000},
{0x40000000, 0         },
{0         , 0x40000000},
{0xc0000000, 0         },
{0         , 0xc0000000}

};

Ipp32sc tblPreQMF_S_LP[] = {

{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000}

};

Ipp32sc tblPostQMF_S_LP[] = {

{0x2d413ccc, 0xd2bec334},
{0x2e5a106f, 0xd3de9156},
{0x2f6bbe44, 0xd5052d97},
{0x30761c17, 0xd6326a89},
{0x317900d6, 0xd76619b6},
{0x32744493, 0xd8a00bae},
{0x3367c08f, 0xd9e01007},
{0x34534f40, 0xdb25f567},
{0x3536cc52, 0xdc71898d},
{0x361214b0, 0xddc29958},
{0x36e5068a, 0xdf18f0ce},
{0x37af8158, 0xe0745b25},
{0x387165e3, 0xe1d4a2c8},
{0x392a9642, 0xe3399167},
{0x39daf5e8, 0xe4a2eff7},
{0x3a8269a2, 0xe61086bd},
{0x3b20d79e, 0xe7821d5a},
{0x3bb6276d, 0xe8f77ad0},
{0x3c424209, 0xea70658b},
{0x3cc511d8, 0xebeca36c},
{0x3d3e82ad, 0xed6bf9d2},
{0x3dae81ce, 0xeeee2d9e},
{0x3e14fdf7, 0xf0730343},
{0x3e71e758, 0xf1fa3ecb},
{0x3ec52f9f, 0xf383a3e2},
{0x3f0ec9f4, 0xf50ef5de},
{0x3f4eaafe, 0xf69bf7c9},
{0x3f84c8e1, 0xf82a6c6b},
{0x3fb11b47, 0xf9ba1651},
{0x3fd39b5a, 0xfb4ab7dc},
{0x3fec43c6, 0xfcdc1342},
{0x3ffb10c1, 0xfe6deaa1},
{0x40000000, 0         },
{0x3ffb10c1, 0x192155f },
{0x3fec43c6, 0x323ecbe },
{0x3fd39b5a, 0x4b54824 },
{0x3fb11b47, 0x645e9af },
{0x3f84c8e1, 0x7d59395 },
{0x3f4eaafe, 0x9640837 },
{0x3f0ec9f4, 0xaf10a22 },
{0x3ec52f9f, 0xc7c5c1e },
{0x3e71e758, 0xe05c135 },
{0x3e14fdf7, 0xf8cfcbd },
{0x3dae81ce, 0x1111d262},
{0x3d3e82ad, 0x1294062e},
{0x3cc511d8, 0x14135c94},
{0x3c424209, 0x158f9a75},
{0x3bb6276d, 0x17088530},
{0x3b20d79e, 0x187de2a6},
{0x3a8269a2, 0x19ef7943},
{0x39daf5e8, 0x1b5d1009},
{0x392a9642, 0x1cc66e99},
{0x387165e3, 0x1e2b5d38},
{0x37af8158, 0x1f8ba4db},
{0x36e5068a, 0x20e70f32},
{0x361214b0, 0x223d66a8},
{0x3536cc52, 0x238e7673},
{0x34534f40, 0x24da0a99},
{0x3367c08f, 0x261feff9},
{0x32744493, 0x275ff452},
{0x317900d6, 0x2899e64a},
{0x30761c17, 0x29cd9577},
{0x2f6bbe44, 0x2afad269},
{0x2e5a106f, 0x2c216eaa},
{0x2d413ccc, 0x2d413ccc},
{0x2c216eaa, 0x2e5a106f},
{0x2afad269, 0x2f6bbe44},
{0x29cd9577, 0x30761c17},
{0x2899e64a, 0x317900d6},
{0x275ff452, 0x32744493},
{0x261feff9, 0x3367c08f},
{0x24da0a99, 0x34534f40},
{0x238e7673, 0x3536cc52},
{0x223d66a8, 0x361214b0},
{0x20e70f32, 0x36e5068a},
{0x1f8ba4db, 0x37af8158},
{0x1e2b5d38, 0x387165e3},
{0x1cc66e99, 0x392a9642},
{0x1b5d1009, 0x39daf5e8},
{0x19ef7943, 0x3a8269a2},
{0x187de2a6, 0x3b20d79e},
{0x17088530, 0x3bb6276d},
{0x158f9a75, 0x3c424209},
{0x14135c94, 0x3cc511d8},
{0x1294062e, 0x3d3e82ad},
{0x1111d262, 0x3dae81ce},
{0xf8cfcbd , 0x3e14fdf7},
{0xe05c135 , 0x3e71e758},
{0xc7c5c1e , 0x3ec52f9f},
{0xaf10a22 , 0x3f0ec9f4},
{0x9640837 , 0x3f4eaafe},
{0x7d59395 , 0x3f84c8e1},
{0x645e9af , 0x3fb11b47},
{0x4b54824 , 0x3fd39b5a},
{0x323ecbe , 0x3fec43c6},
{0x192155f , 0x3ffb10c1},
{0         , 0x40000000},
{0xfe6deaa1, 0x3ffb10c1},
{0xfcdc1342, 0x3fec43c6},
{0xfb4ab7dc, 0x3fd39b5a},
{0xf9ba1651, 0x3fb11b47},
{0xf82a6c6b, 0x3f84c8e1},
{0xf69bf7c9, 0x3f4eaafe},
{0xf50ef5de, 0x3f0ec9f4},
{0xf383a3e2, 0x3ec52f9f},
{0xf1fa3ecb, 0x3e71e758},
{0xf0730343, 0x3e14fdf7},
{0xeeee2d9e, 0x3dae81ce},
{0xed6bf9d2, 0x3d3e82ad},
{0xebeca36c, 0x3cc511d8},
{0xea70658b, 0x3c424209},
{0xe8f77ad0, 0x3bb6276d},
{0xe7821d5a, 0x3b20d79e},
{0xe61086bd, 0x3a8269a2},
{0xe4a2eff7, 0x39daf5e8},
{0xe3399167, 0x392a9642},
{0xe1d4a2c8, 0x387165e3},
{0xe0745b25, 0x37af8158},
{0xdf18f0ce, 0x36e5068a},
{0xddc29958, 0x361214b0},
{0xdc71898d, 0x3536cc52},
{0xdb25f567, 0x34534f40},
{0xd9e01007, 0x3367c08f},
{0xd8a00bae, 0x32744493},
{0xd76619b6, 0x317900d6},
{0xd6326a89, 0x30761c17},
{0xd5052d97, 0x2f6bbe44},
{0xd3de9156, 0x2e5a106f}

};

Ipp32sc tblPreQMF_SD_LP[] = {

{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000},
{0x40000000, 0         },
{0         , 0xc0000000},
{0xc0000000, 0         },
{0         , 0x40000000}

};

Ipp32sc tblPostQMF_SD_LP[] = {

{0x2d413ccc, 0xd2bec334},
{0x2f6bbe44, 0xd5052d97},
{0x317900d6, 0xd76619b6},
{0x3367c08f, 0xd9e01007},
{0x3536cc52, 0xdc71898d},
{0x36e5068a, 0xdf18f0ce},
{0x387165e3, 0xe1d4a2c8},
{0x39daf5e8, 0xe4a2eff7},
{0x3b20d79e, 0xe7821d5a},
{0x3c424209, 0xea70658b},
{0x3d3e82ad, 0xed6bf9d2},
{0x3e14fdf7, 0xf0730343},
{0x3ec52f9f, 0xf383a3e2},
{0x3f4eaafe, 0xf69bf7c9},
{0x3fb11b47, 0xf9ba1651},
{0x3fec43c6, 0xfcdc1342},
{0x40000000, 0         },
{0x3fec43c6, 0x323ecbe },
{0x3fb11b47, 0x645e9af },
{0x3f4eaafe, 0x9640837 },
{0x3ec52f9f, 0xc7c5c1e },
{0x3e14fdf7, 0xf8cfcbd },
{0x3d3e82ad, 0x1294062e},
{0x3c424209, 0x158f9a75},
{0x3b20d79e, 0x187de2a6},
{0x39daf5e8, 0x1b5d1009},
{0x387165e3, 0x1e2b5d38},
{0x36e5068a, 0x20e70f32},
{0x3536cc52, 0x238e7673},
{0x3367c08f, 0x261feff9},
{0x317900d6, 0x2899e64a},
{0x2f6bbe44, 0x2afad269},
{0x2d413ccc, 0x2d413ccc},
{0x2afad269, 0x2f6bbe44},
{0x2899e64a, 0x317900d6},
{0x261feff9, 0x3367c08f},
{0x238e7673, 0x3536cc52},
{0x20e70f32, 0x36e5068a},
{0x1e2b5d38, 0x387165e3},
{0x1b5d1009, 0x39daf5e8},
{0x187de2a6, 0x3b20d79e},
{0x158f9a75, 0x3c424209},
{0x1294062e, 0x3d3e82ad},
{0xf8cfcbd , 0x3e14fdf7},
{0xc7c5c1e , 0x3ec52f9f},
{0x9640837 , 0x3f4eaafe},
{0x645e9af , 0x3fb11b47},
{0x323ecbe , 0x3fec43c6},
{0         , 0x40000000},
{0xfcdc1342, 0x3fec43c6},
{0xf9ba1651, 0x3fb11b47},
{0xf69bf7c9, 0x3f4eaafe},
{0xf383a3e2, 0x3ec52f9f},
{0xf0730343, 0x3e14fdf7},
{0xed6bf9d2, 0x3d3e82ad},
{0xea70658b, 0x3c424209},
{0xe7821d5a, 0x3b20d79e},
{0xe4a2eff7, 0x39daf5e8},
{0xe1d4a2c8, 0x387165e3},
{0xdf18f0ce, 0x36e5068a},
{0xdc71898d, 0x3536cc52},
{0xd9e01007, 0x3367c08f},
{0xd76619b6, 0x317900d6},
{0xd5052d97, 0x2f6bbe44}

};

/********************************************************************/

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

/********************************************************************/

Ipp32s sbrdecInitFilter( sSbrDecFilter** pDC, int* pSizeWorkBuf )
{
  int pSizeSpec = 0, pSizeInit = 0, pSizeBuf = 0;
  Ipp8u* pBufInit;

  *pDC = (sSbrDecFilter*)ippsMalloc_8u_x( sizeof(sSbrDecFilter) );

  /************************************************************
   *                    analysis HQ
   ***********************************************************/
  ippsFFTGetSize_C_32sc_x
                   ( 5, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     &pSizeSpec, &pSizeInit, &pSizeBuf );

  if( pSizeInit )
    pBufInit = ippsMalloc_8u_x(pSizeInit);
  else
    pBufInit = 0;

  if( pSizeSpec )
    (*pDC)->pMemSpecQMFA = ippsMalloc_8u_x(pSizeSpec);
  else
    (*pDC)->pMemSpecQMFA = 0;

  ippsFFTInit_C_32sc_x
    ( &( (*pDC)->pFFTSpecQMFA),
                     5, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     (*pDC)->pMemSpecQMFA, pBufInit );

  if( pBufInit )
    ippsFree_x(pBufInit);

  *pSizeWorkBuf = UMC_MAX( *pSizeWorkBuf, pSizeBuf );

  /************************************************************
   *                    synthesis HQ
   ***********************************************************/

  ippsFFTGetSize_C_32sc_x
                   ( 7, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     &pSizeSpec, &pSizeInit, &pSizeBuf );

  if( pSizeInit )
    pBufInit = ippsMalloc_8u_x(pSizeInit);
  else
    pBufInit = 0;

  if( pSizeSpec )
    (*pDC)->pMemSpecQMFS = ippsMalloc_8u_x(pSizeSpec);
  else
    (*pDC)->pMemSpecQMFS = 0;

  ippsFFTInit_C_32sc_x( &( (*pDC)->pFFTSpecQMFS), 7, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     (*pDC)->pMemSpecQMFS, pBufInit );

  if( pBufInit )
    ippsFree_x(pBufInit);

  *pSizeWorkBuf = UMC_MAX( *pSizeWorkBuf, pSizeBuf );

  /************************************************************
   *                    synthesis down HQ
   ***********************************************************/

  ippsFFTGetSize_C_32sc_x
                   ( 6, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     &pSizeSpec, &pSizeInit, &pSizeBuf );

  if( pSizeInit )
    pBufInit = ippsMalloc_8u_x(pSizeInit);
  else
    pBufInit = 0;

  if( pSizeSpec )
    (*pDC)->pMemSpecQMFSD = ippsMalloc_8u_x(pSizeSpec);
  else
    (*pDC)->pMemSpecQMFSD = 0;

  ippsFFTInit_C_32sc_x( &( (*pDC)->pFFTSpecQMFSD), 6, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     (*pDC)->pMemSpecQMFSD, pBufInit );

  if( pBufInit )
    ippsFree_x(pBufInit);

  *pSizeWorkBuf = UMC_MAX( *pSizeWorkBuf, pSizeBuf );

  /************************************************************
   *                    analysis LP
   ***********************************************************/
  ippsFFTGetSize_C_32sc_x
                   ( 6, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     &pSizeSpec, &pSizeInit, &pSizeBuf );

  if( pSizeInit )
    pBufInit = ippsMalloc_8u_x(pSizeInit);
  else
    pBufInit = 0;

  if( pSizeSpec )
    (*pDC)->pMemSpecQMFA_LPmode = ippsMalloc_8u_x(pSizeSpec);
  else
    (*pDC)->pMemSpecQMFA_LPmode = 0;

  ippsFFTInit_C_32sc_x
    ( &( (*pDC)->pFFTSpecQMFA_LPmode),
                     6, IPP_FFT_NODIV_BY_ANY, ippAlgHintAccurate,
                     (*pDC)->pMemSpecQMFA_LPmode, pBufInit );

  if( pBufInit )
    ippsFree_x(pBufInit);

  *pSizeWorkBuf = UMC_MAX( *pSizeWorkBuf, pSizeBuf );

  /************************************************************
   *                    synthesis LP
   ***********************************************************/

  // the same as synthesis HQ

  return 0;//OK;
}

/********************************************************************/

Ipp32s sbrdecFreeFilter( sSbrDecFilter* pDC )
{
  if( pDC == 0 )
    return 0;

  //if( pDC != 0 ){
  if( pDC->pMemSpecQMFA )
    ippsFree_x( pDC->pMemSpecQMFA );

  if( pDC->pMemSpecQMFA_LPmode )
    ippsFree_x( pDC->pMemSpecQMFA_LPmode );

  if( pDC->pMemSpecQMFS )
    ippsFree_x( pDC->pMemSpecQMFS );

  if( pDC->pMemSpecQMFSD )
    ippsFree_x( pDC->pMemSpecQMFSD );
  //}

  ippsFree_x( pDC );

  return 0;//OK;
}

/********************************************************************
 *
 * <------------------QMF ANALYSIS BLOCK----------------------------->
 *
 ********************************************************************/

static int ownPreProc_AQMF_HQ_32s(int* pSrc, int* pDst)
{
  int i;

  pDst[0] = pSrc[0];
  pDst[1] = pSrc[1];

  for(i=1; i<31; i++)
  {
    pDst[2*i]   = -pSrc[64-i];
    pDst[2*i+1] = pSrc[i+1];
  }

  pDst[63] = pSrc[32];
  pDst[62] = -pSrc[33];

  return 0;
}

/*******************************************************************/

static int ownPostProc_AQMF_HQ_32s(int* pSrc, Ipp32sc* pDst)
{
  int i;

  for(i = 0; i < 32; i++)
  {
    pDst[i].re = pSrc[i];
    pDst[i].im = -pSrc[63 - i];
  }

  return 0;
}

/*******************************************************************/

static int ownPreProc_DCT4_64_32s(Ipp32s* pSrc, Ipp32sc* pDst)
{
  int j;
  //float arg;
  int a, b, c, s;
  //int len = 64;

  for(j=0; j < 32; j++)
  {
    //arg = -(j+0.25f)*M_PI/len;
    c = tblPreProcCosSinDCT4_64[2*j];   //cos(arg);
    s = tblPreProcCosSinDCT4_64[2*j+1]; //sin(arg);

    a = pSrc[2*j];
    b = pSrc[64 - 1 - 2*j];

    //pDstRe[j] = a*c - b*s;
    pDst[j].re = MUL32_SBR_32S(a, c) - MUL32_SBR_32S(b, s);
    //pDstIm[j] = a*s + b*c;
    pDst[j].im = MUL32_SBR_32S(a, s) + MUL32_SBR_32S(b, c);
  }

  return 0;
}

/*******************************************************************/

static int ownPostProc_DCT4_64_32s(Ipp32sc* pSrc, int* pDst)
{
  int u;
  //float arg;
  int a, b, c, s;
  //int len = 64;

  for(u=0; u<32; u++)
  {
    //arg = -M_PI * u / len;
    c = tblPostProcCosSinDCT4_64[2*u];/* cos(arg); */
    s = tblPostProcCosSinDCT4_64[2*u+1];/* sin(arg); */
    a = pSrc[ u ].re;
    b = pSrc[ u ].im;

    pDst[2*u]          = MUL32_SBR_32S(a, c) - MUL32_SBR_32S(b, s);
    pDst[64 - 1 - 2*u] = - ( MUL32_SBR_32S(a, s) + MUL32_SBR_32S(b, c) );
    //pDst[2*u] = a*c - b*s;
    //pDst[64 - 1 - 2*u] = - (a*s + b*c);
  }

  return 0;
}

/********************************************************************/

int  ippsAnalysisFilter_FT_SBR_RToC_32s32s_Sfs(int* pSrc, Ipp32sc* pDst,
                                        IppsFFTSpec_C_32sc* pFFTSpecQMFA,
                                        Ipp8u* pWorkBuffer)
{

  Ipp32s                    bufDCT4[64];

  Ipp32sc                   bufInFft[32];
  Ipp32sc                   bufOutFft[32];

  ownPreProc_AQMF_HQ_32s(pSrc, bufDCT4);

  ///* DCT-IV Inplace */
  ownPreProc_DCT4_64_32s(bufDCT4, bufInFft);
  ippsFFTFwd_CToC_32sc_Sfs_x
                   ( bufInFft,
                     bufOutFft,
                     pFFTSpecQMFA,
                     2,  pWorkBuffer);

  ownPostProc_DCT4_64_32s(bufOutFft, bufDCT4);


  ownPostProc_AQMF_HQ_32s(bufDCT4, pDst);

  return 0;//OK

}

/********************************************************************/

int ippsAnalysisFilter_SBR_RToC_32s_Sfs(Ipp32s* pSrc,
                                        Ipp32sc* pDst,
                                        int kx,
                                        IppsFFTSpec_C_32sc* pFFTSpecQMFA,
                                        int* pDelayCoefs, int scaleFactor,
                                        Ipp8u* pWorkBuf)
{
  int n, k, j;
  int status = 0;
  Ipp32s  z_int[320];
  Ipp32s  u_int[64];

  for (n = 319; n >= 32; n--) {
      pDelayCoefs[n] = pDelayCoefs[n - 32];
    }
    for (n = 31; n >= 0; n--) {
      pDelayCoefs[n] = pSrc[31 - n] << (SCALE_FACTOR_QMFA_IN - scaleFactor);
    }

   ippsMul_32s_Sfs_x(pDelayCoefs, SBR_TABLE_QMF_WINDOW_320_INT_Q30, z_int, 320, 32);

    for (n = 0; n <= 63; n++) {
      u_int[n] = 0;
      for (j = 0; j <= 4; j++) {
        u_int[n] += z_int[n + j * 64];
      }
    }

#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			int *p1 = pSrc;
			int *p2 = u_int;
			fprintf( stderr, "   kinoma debug: o10:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0],p1[1],p1[2],p1[3],p1[4],p1[5] );
			fprintf( stderr, "   kinoma debug: o11:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p2[0],p2[1],p2[2],p2[3],p2[4],p2[5] );
		}
#endif
    ippsAnalysisFilter_FT_SBR_RToC_32s32s_Sfs(u_int,
                                             pDst,
                                             pFFTSpecQMFA,
                                             pWorkBuf);
#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			Ipp32sc *p1 = pDst;
			fprintf( stderr, "   kinoma debug: o12:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0].re,p1[1].re,p1[2].re,p1[3].re,p1[4].re,p1[5].re );
			fprintf( stderr, "   kinoma debug: o13:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0].im,p1[1].im,p1[2].im,p1[3].im,p1[4].im,p1[5].im );
		}
#endif

// set zerro high band
    for (k = kx; k < 32; k++) {
      pDst[k].re =  pDst[k].im = 0;

    }

    return status;
}

/********************************************************************/

int sbrAnalysisFilter_SBR_RToC_32s_D2L_Sfs(Ipp32s* pSrc,
                                    Ipp32sc* pDst[],
                                    int nSubBand, int kx,
                                    IppsFFTSpec_C_32sc* pFFTSpecQMFA,
                                    int* pDelayCoefs, int scaleFactor,
                                    Ipp8u* pWorkBuf)
{
  int l;
  int status = 0;

  for (l = 0; l < nSubBand; l++) {
    ippsAnalysisFilter_SBR_RToC_32s_Sfs(pSrc + 32 * l,
                                        pDst[l],
                                        kx,
                                        pFFTSpecQMFA,
                                        pDelayCoefs,
                                        scaleFactor,
                                        pWorkBuf);
  }


  return status;
}
/********************************************************************
 *
 * <------------------QMF SYNTHESIS BLOCK--------------------------->
 *
 ********************************************************************/

int ownPreProcQMFS_SBR_32sc(Ipp32sc* pSrc,  Ipp32sc* pDst)
{
  int k;

  for(k = 0; k < 64; k++)
  {

    pDst[k].re = MUL32_SBR_32S( pSrc[k].re, SBR_TABLE_PRE_QMFS_COS_INT_Q31[k] )  -
                 MUL32_SBR_32S( pSrc[k].im, SBR_TABLE_PRE_QMFS_SIN_INT_Q31[k] );

    pDst[k].im = MUL32_SBR_32S( pSrc[k].im, SBR_TABLE_PRE_QMFS_COS_INT_Q31[k] ) +
                 MUL32_SBR_32S( pSrc[k].re, SBR_TABLE_PRE_QMFS_SIN_INT_Q31[k] );
  }

  return 0;
}

/********************************************************************/

int ownPostProcQMFS_SBR_32sc(Ipp32sc* pSrc,  Ipp32sc* pDst)
{
  int n;

  for(n = 0; n < 128; n++)
  {
    //angle = SBR_PI / 128 * (n - 126.5f);
    //xCos = cos(angle);
    //ySin = sin(angle);

    pDst[n].re = MUL32_SBR_32S( pSrc[n].re, SBR_TABLE_POST_QMFS_COS_INT_Q31[n] )  -
                 MUL32_SBR_32S( pSrc[n].im, SBR_TABLE_POST_QMFS_SIN_INT_Q31[n] );

    pDst[n].im = MUL32_SBR_32S( pSrc[n].im, SBR_TABLE_POST_QMFS_COS_INT_Q31[n] ) +
                 MUL32_SBR_32S( pSrc[n].re, SBR_TABLE_POST_QMFS_SIN_INT_Q31[n] );

  }

  return 0;
}

/********************************************************************/

int sbrClip_64s32s(Ipp64s inData)
{
  int outData;

  if (inData > 0x7FFFFFFF )
    outData = 0x7FFFFFFF;
  else if (inData < -0x7FFFFFFF)
    outData = -0x7FFFFFFF;
  else
    outData = (int)inData;

  return outData;
}

/********************************************************************/

int ippsRealToCplx_32s(const int* pSrcRe, const int* pSrcIm, Ipp32sc* pSrc, int len)
{
  int k;

  for(k=0; k<len; k++)
  {
    pSrc[k].re = pSrcRe[k];
    pSrc[k].im = pSrcIm[k];
  }

  return 0;
}

/********************************************************************/

int ippsCplxToReal_32sc(const Ipp32sc* pSrc, int* pSrcRe, int* pSrcIm, int len)
{
  int k;

  for(k=0; k<len; k++)
  {
    pSrcRe[k] = pSrc[k].re;
    pSrcIm[k] = pSrc[k].im;
  }

  return 0;
}

/********************************************************************/

int  ippsSynthesisFilter_FT_SBR_RToC_32s32s_Sfs(Ipp32sc* pSrc,
                                               IppsFFTSpec_C_32sc* pFFTSpecQMFS,
                                               int* pDst,
                                               Ipp8u *pWorkBuf)

{
  Ipp32sc vec_128_32sc[128];
  Ipp32s  vec_128_32s[128];
  Ipp32sc fft_out[128];
  Ipp32sc fft_out2[128];
  //Ipp32sc pSrc[64];

  ippsZero_32sc_x(&(vec_128_32sc[64]), 64);

  //ippsRealToCplx_32s(pSrcRe, pSrcIm, pSrc, 64);
  //ippsCopy_32sc(pSrc, vec_128_32sc, 64);

  ownPreProcQMFS_SBR_32sc(pSrc,  vec_128_32sc); //Q(5) * Q(31) * Q(-32) = Q(5-1)

  ippsFFTInv_CToC_32sc_Sfs_x(vec_128_32sc, fft_out, pFFTSpecQMFS, 0, pWorkBuf);//Q(5-1+6)

  ownPostProcQMFS_SBR_32sc(fft_out, fft_out2);//Q(5-1+6) * Q(31) * Q(-32) = Q(9)

  ippsCplxToReal_32sc(fft_out2, pDst, vec_128_32s, 128);

  return 0;
}

/********************************************************************/

int ippsSynthesisFilter_SBR_CToR_32s_Sfs(Ipp32sc* piSrc,
                                     Ipp32s *pDst,

                                     IppsFFTSpec_C_32sc* pFFTSpecQMFS,
                                     int* pDelayCoefs,

                                     int* scaleFactor,
                                     Ipp8u *pWorkBuf)
{
  Ipp32s g[640];
  Ipp32s* v ;
  int n,k;
  int status = 0;

  v = pDelayCoefs;

  ippsMove_32s_x(v, v+128, 1280-128);

  ippsSynthesisFilter_FT_SBR_RToC_32s32s_Sfs(piSrc, pFFTSpecQMFS, v, pWorkBuf);

  for (n=0; n<5; n++) {
    for (k=0; k<=63; k++) {
      g[128*n + k]      = v[256*n + k];
      g[128*n + 64 + k] = v[256*n + 192 + k];
    }
  }

  ippsMul_32s_ISfs_x(SBR_TABLE_QMF_WINDOW_640_INT_Q31, g, 640, 32);//Q(9-1)

  for (k=0; k<=63; k++) {
    Ipp64s sum = 0L;

    for (n=0; n<=9; n++) {
      sum += g[64*n + k];
    }
    pDst[k] = sbrClip_64s32s( sum );//Q(8)
  }


  *scaleFactor =  8 ;

  return status;
}

/********************************************************************
 *
 * <------------------QMF SYNTHESIS DOWN BLOCK---------------------->
 *
 ********************************************************************/

int ownPreProcQMFSD_SBR_32sc(Ipp32sc* pSrc,  Ipp32sc* pDst)
{

  ippsMul_32sc_Sfs_x(pSrc, tblPreQMF_SD_HQ, pDst, 32, 32);

  return 0;
}

/********************************************************************/

int ownPostProcQMFSD_SBR_32sc(Ipp32sc* pSrc,  Ipp32sc* pDst)
{

  ippsMul_32sc_Sfs_x(pSrc, tblPostQMF_SD_HQ, pDst, 64, 32);

  return 0;
}

/********************************************************************/

int  ippsSynthesisDownFilter_FT_SBR_RToC_32s32s_Sfs(Ipp32sc* pSrc,
                                               IppsFFTSpec_C_32sc* pFFTSpecQMFSD,
                                               int* pDst,
                                               Ipp8u *pWorkBuf)

{
  Ipp32sc vec_64_32sc[64];
  Ipp32s  vec_64_32s[64];
  Ipp32sc fft_out[64];
  Ipp32sc fft_out2[64];
  //Ipp32sc pSrc[64];

  ippsZero_32sc_x(&(vec_64_32sc[32]), 32);

  //ippsRealToCplx_32s(pSrcRe, pSrcIm, pSrc, 32);

  ownPreProcQMFSD_SBR_32sc(pSrc,  vec_64_32sc); //Q(5) * Q(31) * Q(-32) = Q(5-1)

  ippsFFTInv_CToC_32sc_Sfs_x(vec_64_32sc, fft_out, pFFTSpecQMFSD, 0, pWorkBuf);//Q(5+2)

  ownPostProcQMFSD_SBR_32sc(fft_out, fft_out2);//Q(5+2) * Q(31) * Q(-32) = Q(5+1)

  ippsCplxToReal_32sc(fft_out2, pDst, vec_64_32s, 64);

  return 0;
}

/********************************************************************/

int ippsSynthesisDownFilter_SBR_CToR_32s_Sfs(Ipp32sc* piSrc,
                                     Ipp32s *pDst,

                                     IppsFFTSpec_C_32sc* pFFTSpecQMFSD,
                                     int* pDelayCoefs,
                                     int* scaleFactor,
                                     Ipp8u *pWorkBuf)
{
  Ipp32s g[640];
  Ipp32s* v ;
  int n,k;
  int status = 0;

  v = pDelayCoefs;

  ippsMove_32s_x(v, v+64, 640-64);

  ippsSynthesisDownFilter_FT_SBR_RToC_32s32s_Sfs(piSrc, pFFTSpecQMFSD, v, pWorkBuf);

  for (n=0; n<5; n++) {
    for (k=0; k<=31; k++) {
      g[64*n + k]      = v[128*n + k];
      g[64*n + 32 + k] = v[128*n + 96 + k];
    }
  }

  ippsMul_32s_ISfs_x(SBR_TABLE_QMF_WINDOW_320_INT_Q31, g, 320, 32);//Q(-1)

  for (k=0; k<=31; k++) {
    Ipp64s sum = 0L;

    for (n=0; n<=9; n++) {
      sum += g[32*n + k];
    }
    pDst[k] = sbrClip_64s32s( sum );//Q(5)
  }

  *scaleFactor =  6 ;

  return status;
}

/********************************************************************
 *
 * <------------------QMF ANALYSIS LP BLOCK---------------------->
 *
 ********************************************************************/
#if 0
int my_IFFT(Ipp32fc* pSrc,Ipp32fc* pDst, int len)
{
  int l;//, len=64;
  int k,n;
  float arg, sumRe, sumIm;

  for(k=0; k<len; k++)
  {
    sumRe = sumIm = 0.0f;

    for( n=0; n<len;n++ )
    {
      //arg = -2.0f * M_PI * k* n / len;
      arg = 2.0f * M_PI * k* n / len;

      sumRe += pSrc[n].re * cos(arg) - pSrc[n].im * sin(arg);
      sumIm += pSrc[n].im * cos(arg) + pSrc[n].re * sin(arg);
    }

    pDst[k].re = sumRe;
    pDst[k].im = sumIm;
  }

  return 0;
}
#endif
/********************************************************************/

int ownAQMF_SBR_LP_PreProcVER2(Ipp32s* pSrc, Ipp32sc* pDst)
{

  ippsMul_32s32sc_Sfs_x(pSrc, tblPreQMF_A_LP, pDst, 64, 32);


  return 0;//OK
}

int ownAQMF_SBR_LP_PostProcVER2(Ipp32sc* pSrc, Ipp32s* pDst)
{
  int n;

  for(n=0; n<32; n++)
  {
    pDst[n] = MUL32_SBR_32S(pSrc[n].re, tblPostQMF_A_LP[n].re) - MUL32_SBR_32S(pSrc[n].im, tblPostQMF_A_LP[n].im);


  }

  return 0;//OK
}

/********************************************************************/

int ippsAnalysisFilter_FT_SBR_RToR_32s32sVER2_Sfs(Ipp32s* pSrc,
                                                   Ipp32s* pDstRe,
                                                   IppsFFTSpec_C_32sc* pFFTSpecQMFA,
                                                   Ipp8u* pWorkBuf)
{
  Ipp32sc bufData[64];
  Ipp32sc bufOutFFT[64];

  ownAQMF_SBR_LP_PreProcVER2(pSrc, bufData);//Q(12) - Q(2)
  //my_IFFT(bufData, bufOutFFT, 64);
  ippsFFTInv_CToC_32sc_Sfs_x(bufData, bufOutFFT, pFFTSpecQMFA, 2, pWorkBuf);//Q(10-2)

  ownAQMF_SBR_LP_PostProcVER2(bufOutFFT, pDstRe);//Q(8-2-1)

  return 0;//OK
}

/********************************************************************/
#if 0
int ownAQMF_SBR_LP_PreProc(float* pSrc, float* pDst)
{
  int n;

  pDst[0] = pSrc[48];

  for(n=1; n<16; n++)
  {
    pDst[n] = pSrc[n+48] + pSrc[48-n];
  }

  for(n=16; n<32; n++)
  {
    pDst[n] = -pSrc[n-16] + pSrc[48-n];
  }

  return 0;//OK
}

/********************************************************************/

int ownAQMF_SBR_LP_PostProc_I(float* pSrcDst)
{
  int n;

  for(n=0; n<32; n++)
  {
    pSrcDst[n] = 2 * pSrcDst[n];
  }

  return 0;//OK
}

/********************************************************************/

int ownDCT3_32f(float* pSrc, float* pDst, int len)
{
  int k,i;
  float sum  = 0.f;
  float My_PI = 3.14159265358979323846f;

  for(k=0; k<len; k++){
    sum = 0.f;

    for(i=0; i<len; i++){
      sum += pSrc[i] * cos( My_PI / (2.f * len) * i * (2*k + 1) );
    }

    pDst[ k ] = sum;
  }
  return 0;
}

/********************************************************************/

 int ippsAnalysisFilter_FT_SBR_RToR_32s32s_Sfs(float* pSrc,
                                               float* pDstRe,
                                               Ipp8u* pWorkBuf)
{
  float bufData[32];

  ownAQMF_SBR_LP_PreProc(pSrc, bufData);
  ownDCT3_32f(bufData, pDstRe, 32);
  ownAQMF_SBR_LP_PostProc_I(pDstRe);

  return 0;//OK
}
#endif
 /********************************************************************/

int ippsAnalysisFilter_SBR_RToR_32s_Sfs(Ipp32s* pSrc,
                                        Ipp32s* pDstRe,
                                        int kx,
                                        IppsFFTSpec_C_32sc* pFFTSpecQMFA,
                                        Ipp32s* pDelayCoefs,
                                        int scaleFactor,
                                        Ipp8u* pWorkBuf)
{
  int n, k, j;
  int status = 0;
  Ipp32s  z_int[320];
  Ipp32s  u_int[64];

  for (n = 319; n >= 32; n--) {
    pDelayCoefs[n] = pDelayCoefs[n - 32];
  }
  for (n = 31; n >= 0; n--) {
    pDelayCoefs[n] = pSrc[31 - n] << (SCALE_FACTOR_QMFA_IN - scaleFactor);
  }

  ippsMul_32s_Sfs_x(pDelayCoefs, SBR_TABLE_QMF_WINDOW_320_INT_Q30, z_int, 320, 32);

    for (n = 0; n <= 63; n++) {
      u_int[n] = 0;
      for (j = 0; j <= 4; j++) {
        u_int[n] += z_int[n + j * 64];
      }
    }

    //ippsAnalysisFilter_FT_SBR_RToR_32s32s_Sfs(u_int, pDstRe, pWorkBuf);
    ippsAnalysisFilter_FT_SBR_RToR_32s32sVER2_Sfs(u_int, pDstRe, pFFTSpecQMFA, pWorkBuf);

// set zerro high band
    for (k = kx; k < 32; k++) {
      pDstRe[k] = 0;
    }

    return status;
}

/********************************************************************/

int sbrAnalysisFilter_SBR_RToR_32s_D2L_Sfs(Ipp32s* pSrc,
                                    Ipp32s* pDstRe[],
                                    int nSubBand, int kx,
                                    IppsFFTSpec_C_32sc* pFFTSpec,
                                    Ipp32s* pDelayCoefs,
                                    int scaleFactor,
                                    Ipp8u* pWorkBuf)
{

  int l;

  for (l=0; l<nSubBand; l++) {

   ippsAnalysisFilter_SBR_RToR_32s_Sfs(pSrc + 32 * l,
                                       pDstRe[l],
                                       kx,
                                       pFFTSpec,
                                       pDelayCoefs,
                                       scaleFactor,
                                       pWorkBuf);
  }

  return 0;
}

/********************************************************************
 *
 * <------------------QMF SYNTHESIS LP BLOCK---------------------->
 *
 ********************************************************************/

/* !!! DRAFT VERSION !!! */
int ownPreProcSQMF_SBR_LP(Ipp32s* pSrc, Ipp32sc* pDst)
{
  //int k;
  //float arg;
  //float cosX, sinX;
  //int iCosX, iSinX;

  //tblPreQMF_S_LP
  ippsMul_32s32sc_Sfs_x(pSrc, tblPreQMF_S_LP, pDst, 128, 32);

  /*for(k = 0; k < 128; k++)
  {
    arg = - 2.f * M_PI * 32 * k / 128.f;
    cosX = cos(arg);
    sinX = sin(arg);

    iCosX = (int)( cosX * (1<<30) );
    iSinX = (int)( sinX * (1<<30) );

    pDst[k].re = MUL32_SBR_32S(pSrc[k], iCosX);
    pDst[k].im = MUL32_SBR_32S(pSrc[k], iSinX);
  }*/

  return 0;//OK
}

/********************************************************************/

int ownPostProcSQMF_SBR_LP(Ipp32sc* pSrc, Ipp32s* pDst)
{
  int k;
 /* float arg;
  float cosX, sinX;
  int iCosX, iSinX;*/


  for(k = 0; k < 128; k++)
  {
    /*arg = 2.f * M_PI / 128.f * (0.5f*k - 16);
    cosX = cos(arg);
    sinX = sin(arg);

    iCosX = (int)( cosX * (1<<30) );
    iSinX = (int)( sinX * (1<<30) );*/

    //pDst[k] = pSrc[k].re * cosX - pSrc[k].im * sinX;
    pDst[k] = MUL32_SBR_32S(pSrc[k].re,  tblPostQMF_S_LP[k].re) - MUL32_SBR_32S(pSrc[k].im,  tblPostQMF_S_LP[k].im);
  }

  return 0;//OK
}

/********************************************************************/

int ippsSynthesisFilter_FT_SBR_RToR_32s32s_Sfs(int* piSrcRe, IppsFFTSpec_C_32sc* pFFTSpecQMFS,
                                               int* iv, Ipp8u* pWorkBuf)
{
  Ipp32s pSrcRe[128];//[64];

  Ipp32sc inFFT[128];
  Ipp32sc outFFT[128];

  ippsCopy_32s_x(piSrcRe, pSrcRe, 64);
  ippsZero_32s_x(pSrcRe + 64, 64);

  ownPreProcSQMF_SBR_LP(pSrcRe, inFFT);// Q(-2)

  ippsFFTInv_CToC_32sc_Sfs_x(inFFT, outFFT, pFFTSpecQMFS, 0, pWorkBuf);//Q( +1 + implicity Q(5) )

  ownPostProcSQMF_SBR_LP(outFFT, iv);//Q(-2)

  return 0;
}

/********************************************************************/

int ippsSynthesisFilter_SBR_RToR_32s_Sfs(Ipp32s* pSrc,
                                     Ipp32s *pDst,
                                     IppsFFTSpec_C_32sc* pFFTSpecQMFS,
                                     Ipp32s* pDelayCoefs,
                                     int* scaleFactor,
                                     Ipp8u *pWorkBuf)
{
  Ipp32s g[640];
  Ipp32s* v ;
  int n,k;
  int status = 0;

  v = pDelayCoefs;

  ippsMove_32s_x(v, v+128, 1280-128);

  ippsSynthesisFilter_FT_SBR_RToR_32s32s_Sfs(pSrc, pFFTSpecQMFS, v, pWorkBuf);

  for (n=0; n<5; n++) {
    for (k=0; k<=63; k++) {
      g[128*n + k]      = v[256*n + k];
      g[128*n + 64 + k] = v[256*n + 192 + k];
    }
  }

  ippsMul_32s_ISfs_x(SBR_TABLE_QMF_WINDOW_640_INT_Q31, g, 640, 32);

  for (k=0; k<=63; k++) {
    Ipp64s sum = 0L;

    for (n=0; n<=9; n++) {
      sum += g[64*n + k];
    }
    pDst[k] = sbrClip_64s32s( sum );//Q(5)
  }

  *scaleFactor =  5 ;

  return status;
}

/********************************************************************/

int sbrSynthesisFilter_SBR_RToR_32s_D2L(Ipp32s** piSrcRe,
                                         Ipp32s *pDst,
                                         int nSubBand,
                                         IppsFFTSpec_C_32sc* pFFTSpecQMFS,
                                         Ipp32s* pDelayCoefs,
                                         int* scaleFactor,
                                         Ipp8u *pWorkBuf)
{
  int l;
  int status = 0;

  for (l=0; l<nSubBand; l++) {

    ippsSynthesisFilter_SBR_RToR_32s_Sfs(piSrcRe[l],
                                     pDst+64*l,
                                     pFFTSpecQMFS,
                                     pDelayCoefs,
                                     scaleFactor,
                                     pWorkBuf);
  }

  *scaleFactor =  5 ;

  return status;
}

/********************************************************************
 *
 * <------------------QMF SYNTHESIS DOWN LP BLOCK------------------->
 *
 ********************************************************************/

int ownPreProcQMFSD_SBR_LP_32sc(Ipp32s* pSrc,  Ipp32sc* pDst)
{

  ippsMul_32s32sc_Sfs_x(pSrc, tblPreQMF_SD_LP, pDst, 32, 32);

  return 0;
}

/********************************************************************/

int ownPostProcQMFSD_SBR_LP_32sc(Ipp32sc* pSrc,  Ipp32s* pDst)
{
  int n;

  for(n = 0; n < 64; n++)
  {
    pDst[n] = MUL32_SBR_32S( pSrc[n].re, tblPostQMF_SD_LP[n].re )  - MUL32_SBR_32S( pSrc[n].im, tblPostQMF_SD_LP[n].im );
  }

  return 0;
}

/********************************************************************/

int  ippsSynthesisDownFilter_FT_SBR_RToR_32s32s_Sfs(Ipp32s* pSrcRe,
                                               IppsFFTSpec_C_32sc* pFFTSpecQMFSD,
                                               int* pDst,
                                               Ipp8u *pWorkBuf)

{
  Ipp32sc vec_64_32sc[64];
  Ipp32sc fft_out[64];

  ippsZero_32sc_x(&(vec_64_32sc[32]), 32);

  ownPreProcQMFSD_SBR_LP_32sc(pSrcRe,  vec_64_32sc); //Q(5) * Q(30) * Q(-32) = Q(5-2)

  ippsFFTInv_CToC_32sc_Sfs_x(vec_64_32sc, fft_out, pFFTSpecQMFSD, 0, pWorkBuf);//Q(3+5)

  ownPostProcQMFSD_SBR_LP_32sc(fft_out, pDst);//Q(5+5) * Q(30) * Q(-32) = Q(6)

  return 0;
}

/********************************************************************/

int ippsSynthesisDownFilter_SBR_RToR_32s_Sfs(Ipp32s* piSrcRe,
                                     Ipp32s *pDst,

                                     IppsFFTSpec_C_32sc* pFFTSpecQMFSD,
                                     int* pDelayCoefs,
                                     int* scaleFactor,
                                     Ipp8u *pWorkBuf)
{
  Ipp32s g[640];
  Ipp32s* v ;
  int n,k;
  int status = 0;

  v = pDelayCoefs;

  ippsMove_32s_x(v, v+64, 640-64);

  ippsSynthesisDownFilter_FT_SBR_RToR_32s32s_Sfs(piSrcRe, pFFTSpecQMFSD, v, pWorkBuf);

  for (n=0; n<5; n++) {
    for (k=0; k<=31; k++) {
      g[64*n + k]      = v[128*n + k];
      g[64*n + 32 + k] = v[128*n + 96 + k];
    }
  }

  ippsMul_32s_ISfs_x(SBR_TABLE_QMF_WINDOW_320_INT_Q31, g, 320, 32);//Q(-1)

  for (k=0; k<=31; k++) {
    Ipp64s sum = 0L;

    for (n=0; n<=9; n++) {
      sum += g[32*n + k];
    }
    pDst[k] = sbrClip_64s32s( sum );//Q(5)
  }

  *scaleFactor =  5 ;

  return status;
}


/***********************************************************************/

int sbrSynthesisFilter_32s(Ipp32s* bufZ, Ipp32s* pDst,
                           Ipp32s* bufDelay, sSbrDecFilter* pQmfBank,
                           Ipp8u* pWorkBuf, int* scaleFactor, int mode)
{
  switch ( mode ) {

   case (HEAAC_HQ_MODE | HEAAC_DWNSMPL_OFF):
      ippsSynthesisFilter_SBR_CToR_32s_Sfs((Ipp32sc*)bufZ, pDst,

                                       pQmfBank->pFFTSpecQMFS,
                                       bufDelay,

                                       scaleFactor, // !!!
                                       pWorkBuf);

        *scaleFactor =  8 ;
        break;

   case (HEAAC_HQ_MODE | HEAAC_DWNSMPL_ON):
        ippsSynthesisDownFilter_SBR_CToR_32s_Sfs((Ipp32sc*)bufZ, pDst,

                                       pQmfBank->pFFTSpecQMFSD,
                                       bufDelay,

                                       scaleFactor, // !!!
                                       pWorkBuf);
        *scaleFactor =  6 ;
        break;

   case (HEAAC_LP_MODE | HEAAC_DWNSMPL_OFF):
      ippsSynthesisFilter_SBR_RToR_32s_Sfs(bufZ, pDst,

                                           pQmfBank->pFFTSpecQMFS,
                                           bufDelay,

                                           scaleFactor, // !!!
                                           pWorkBuf );


      *scaleFactor =  5 ;
      break;

   case (HEAAC_LP_MODE | HEAAC_DWNSMPL_ON):
   default:
       ippsSynthesisDownFilter_SBR_RToR_32s_Sfs(bufZ, pDst,

                                               pQmfBank->pFFTSpecQMFSD,
                                               bufDelay,

                                               scaleFactor, // !!!
                                               pWorkBuf);
      *scaleFactor =  5 ;
      break;
   }

  return 0; //OK
}

/********************************************************************/

/* EOF */
