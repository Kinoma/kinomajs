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
//***
#include "kinoma_ipp_lib.h"

#include "ipps.h"
#include "sbrdec_own_int.h"

/********************************************************************/

#ifndef UMC_MIN
#define UMC_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

/********************************************************************/
// SBR_TABLE_INVERT[i] = 2^0 / (i + 1), Q31

const int SBR_TABLE_INVERT[64] = {
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

int CLZ(int maxVal)
{

  int     scalef = 0;
  Ipp32u  max = maxVal;

  if (max == 0)
    return 32;

  if (maxVal < 0 )
    return 0;

  while (max <= ((1 << 30) - 1)) {
    max *= 2;
    scalef++;
  }

  return scalef + 1;
}

/********************************************************************/
/*
 * wrapper for function y = isqrt(x)
 */
int sbrSqrtWrap_64s_Sfs(int inData, int inScaleFactor, int *outScaleFactor)
{
  int     isOddNumber = 0;
  int     nGB;
  int     outData;

  if (inData <= 0) {
    *outScaleFactor = inScaleFactor;
    return 0;
  }

/*
 * y = sqrt( x ), x = M*Q(p)
 * y = Q(p/2)*sqrt(M),            if p is     even number
 * y = Q[(p+1)/2] * sqrt(0.5 * M) if p is not even number (odd number), TERRIBLE!!! :)
 */
  isOddNumber = inScaleFactor & 0x01;
  if (isOddNumber) {
    inData >>= 1;
    inScaleFactor--;
  }

/*
 * normalize
 */
  nGB = CLZ(inData) - 1;      // remove "sign"
  nGB >>= 1;
  inData <<= (nGB << 1);

  {
    Ipp64s  data = inData;

    ippsSqrt_64s_ISfs_x(&data, 1, 0);
    outData = (int)data;

  }

  *outScaleFactor = (inScaleFactor >> 1) + nGB;

  return outData;
}

/********************************************************************/

#define Y0      0X15555555  /* 4/3, Q28 */

#define TWO_Q28 0X20000000 /* 2.0, Q28 */

#define NUM_ITER  5

/*
 *  y = 1/x - target function
 *  y[i+1] = y[i] * (2 - x*y[i]) - classical newton's iteration, delta[i+1] = delta[i]^2
 *  y[0] = 4/3
 *  Q28
 */

static int ownInv_32s(int x)
{
  int     i, yi, y;

  yi = Y0; //starting value

  for (i = 1; i <= NUM_ITER; i++) {
    y = MUL32_SBR_32S(x, yi);      /* Q(31) * Q(29) * Q(-32) = Q(28) */
    y = TWO_Q28 - y;                     /* Q(28) - Q(28) */
    yi = MUL32_SBR_32S(yi, y) << 4;/* Q(29) * Q(28) * Q(-32) * Q(4) = Q(29) */
  }

  return yi;
}

/********************************************************************/

/*
 * wrapper for function y = 1/x
 */
int sbrInvWrap_32s_Sf(int x, int *outScaleFactor)
{
  int     nGB;
  int     y;

  nGB = CLZ(x) - 1;   // remove "sign"
  y = ownInv_32s(x << nGB);

  *outScaleFactor = nGB;

  return y;
}

/********************************************************************/

static int SHIFTUP_CLIP30(int inData, int shift)
{
  Ipp64s  data;
  int     outData;

  shift = UMC_MAX(0, shift);
  shift = UMC_MIN(31, shift);

  data = (Ipp64s)inData << shift;

  if (data > IPP_MAX_32S) {
    outData = IPP_MAX_32S;
  } else if (data < IPP_MIN_32S) {
    outData = IPP_MIN_32S;
  } else
    outData = (int)data;

  return outData;
}

/********************************************************************/

int sbrChangeScaleFactor(int inData, int inScaleFactor, int outScaleFactor)
{
  int     shift = outScaleFactor - inScaleFactor;
  int     outData;

  if (shift > 0) {
    outData = SHIFTUP_CLIP30(inData, shift);
  } else {
    outData = inData >> (-shift);
  }

  return outData;
}

/********************************************************************/
