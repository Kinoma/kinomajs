/* TomsFastMath, a fast ISO C bignum library.
 * 
 * This project is meant to fill in where LibTomMath
 * falls short.  That is speed ;-)
 *
 * This project is public domain and free for all purposes.
 * 
 * Tom St Denis, tomstdenis@gmail.com
 */
#include <tfm.h>

void fp_read_unsigned_bin(fp_int *a, unsigned char *b, int c)
{
  /* zero the int */
  fp_zero (a);

  /* read the bytes in */
  for (; c > 0; c--) {
    fp_mul_2d (a, 8, a);
    a->dp[0] |= *b++;
    a->used += 1;
  }
  fp_clamp (a);
}

/* $Source$ */
/* $Revision$ */
/* $Date$ */
