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

#include "ipps.h"
#include "sbrdec_element.h"
//#include "sbrdec_own_int.h"
//#include "sbrdec_tables_int.h"

/********************************************************************/
#ifndef MUL32_SBR_32S
#define MUL32_SBR_32S(x, y) \
  (Ipp32s)(((Ipp64s)((Ipp64s)(x) * (Ipp64s)(y)))>>32)
#endif

/********************************************************************/

static const int ROUND_UP = 0x800000;   /* 0.5 Q(24) */

/********************************************************************/

/* SBR_TABLE_LN[ i ] = ln[i] , range = [1, 65], Q(28) */
static const int SBR_TABLE_LN[] = {
  0, 0, 0xb17217f, 0x1193ea7a, 0x162e42fe,
  0x19c041f7, 0x1cab0bfa, 0x1f2272ae, 0x2145647e, 0x2327d4f5,
  0x24d76377, 0x265dc76e, 0x27c22d79, 0x290a0856, 0x2a39942d,
  0x2b542c72, 0x2c5c85fd, 0x2d54d783, 0x2e3ef674, 0x2f1c6c0c,
  0x2fee84f6, 0x30b65d28, 0x3174e8ee, 0x322afbfb, 0x32d94ef9,
  0x338083ef, 0x342129d5, 0x34bbbf70, 0x3550b5ad, 0x35e07197,
  0x366b4df2, 0x36f19c91, 0x3773a77d, 0x37f1b1e9, 0x386bf903,
  0x38e2b4a6, 0x395617f4, 0x39c651dc, 0x3a338d8b, 0x3a9df2d1,
  0x3b05a676, 0x3b6aca8b, 0x3bcd7ea8, 0x3c2de02c, 0x3c8c0a6d,
  0x3ce816ed, 0x3d421d7b, 0x3d9a345f, 0x3df07078, 0x3e44e55c,
  0x3e97a56f, 0x3ee8c1fe, 0x3f384b55, 0x3f8650d0, 0x3fd2e0ef,
  0x401e0966, 0x4067d72c, 0x40b05686, 0x40f79317, 0x413d97e7,
  0x41826f71, 0x41c623ac, 0x4208be10, 0x424a47a3, 0x428ac8fc,
};

/********************************************************************/

static const int SBR_TABLE_INVERT[64] = {
  0x7fffffff, 0x40000000, 0x2aaaaaab, 0x20000000,
  0x1999999a, 0x15555555, 0x12492492, 0x10000000,
  0x0e38e38e, 0x0ccccccd, 0x0ba2e8ba, 0x0aaaaaab,
  0x09d89d8a, 0x09249249, 0x08888889, 0x08000000,
  0x07878788, 0x071c71c7, 0x06bca1af, 0x06666666,
  0x06186186, 0x05d1745d, 0x0590b216, 0x05555555,
  0x051eb852, 0x04ec4ec5, 0x04bda12f, 0x04924925,
  0x0469ee58, 0x04444444, 0x04210842, 0x04000000,
  0x03e0f83e, 0x03c3c3c4, 0x03a83a84, 0x038e38e4,
  0x03759f23, 0x035e50d8, 0x03483483, 0x03333333,
  0x031f3832, 0x030c30c3, 0x02fa0be8, 0x02e8ba2f,
  0x02d82d83, 0x02c8590b, 0x02b93105, 0x02aaaaab,
  0x029cbc15, 0x028f5c29, 0x02828283, 0x02762762,
  0x026a439f, 0x025ed098, 0x0253c825, 0x02492492,
  0x023ee090, 0x0234f72c, 0x022b63cc, 0x02222222,
  0x02192e2a, 0x02108421, 0x02082082, 0x02000000,
};

/********************************************************************/

/*********************************************************************************
 * y = ( a / b )^ (1 / c)
 *
 * y = exp( (1 / c ) * ( ln(a) - ln(b) )
 *
 * y(x) = exp(x) = 1 + x / 1! + x^2 / 2! + x^3 / 3! + x^4 / 4! + x^5 / 5! + ...
 * hold 5 item only
 *********************************************************************************/
static int sbrGetInitVal(int a, int b, int c)
{
  int     i, x, yi, y;

  a = SBR_TABLE_LN[a];  // Q(28)
  b = SBR_TABLE_LN[b];  // Q(28)

/*
 * Q(28) / Q(0) = Q(28)
 */
  x = (a - b) / c;

  y = (1 << 24);        /* y(x) = 1, Q(24) */
  yi = x >> 4;  /* yi = x^1 * 1/1!, Q(24) */
  y += yi;

  for (i = 2; i <= 5; i++) {
    yi = MUL32_SBR_32S(yi, SBR_TABLE_INVERT[i - 1]) << 1;
    yi = MUL32_SBR_32S(x, yi) << 4;     /* yi = x^i * 1/i! (Q24) */
    y += yi;
  }

  return y;
}

 // fixed-point version
/* **************************************************************
 *
 * q = (k1 / k0) ^ (1 / numBands0);
 * pow_vec[0] = k0;
 *
 * for(k = 1; k<=numBands0; k++)
 * {
 *   pow_vec[k] = NINT[ k0 * q ^ k ];
 * }
 *
 * **************************************************************/

int sbrGetPowerVector(int numBands0, int k1, int k0, int *pow_vec)
{
  int     k;
  int     q;
  int     accMul;

  if (numBands0 <= 0)
    return -1;  // error ?

  if ((numBands0 < 1 || numBands0 > 64) || (k1 < 1 || k1 > 64) ||
      (k0 < 1 || k0 > 64))
    q = 0;
  else
    q = sbrGetInitVal(k1, k0, numBands0);       /* Q(24) */

  pow_vec[0] = k0;
  accMul = k0 << 24;

  for (k = 1; k <= numBands0; k++) {
    accMul = MUL32_SBR_32S(accMul, q) << 8;     /* Q(24) */
    pow_vec[k] = (accMul + ROUND_UP) >> 24;
  }

  return 0;     // OK
}

/* EOF */
