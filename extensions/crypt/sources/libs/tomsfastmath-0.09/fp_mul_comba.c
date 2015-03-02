/* TomsFastMath, a fast ISO C bignum library.
 * 
 * This project is meant to fill in where LibTomMath
 * falls short.  That is speed ;-)
 *
 * This project is public domain and free for all purposes.
 * 
 * Tom St Denis, tomstdenis@gmail.com
 */

/* About this file...

*/

#include <tfm.h>

#if defined(TFM_PRESCOTT) && defined(TFM_SSE2)
   #undef TFM_SSE2
   #define TFM_X86
#endif

/* these are the combas.  Worship them. */
#if defined(TFM_X86)
/* Generic x86 optimized code */

/* anything you need at the start */
#define COMBA_START

/* clear the chaining variables */
#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;

/* forward the carry to the next digit */
#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);

/* store the first sum */
#define COMBA_STORE(x) \
   x = c0;

/* store the second sum [carry] */
#define COMBA_STORE2(x) \
   x = c1;

/* anything you need at the end */
#define COMBA_FINI

/* this should multiply i and j  */
#define MULADD(i, j)                                      \
asm(                                                      \
     "movl  %6,%%eax     \n\t"                            \
     "mull  %7           \n\t"                            \
     "addl  %%eax,%0     \n\t"                            \
     "adcl  %%edx,%1     \n\t"                            \
     "adcl  $0,%2        \n\t"                            \
     :"=r"(c0), "=r"(c1), "=r"(c2): "0"(c0), "1"(c1), "2"(c2), "m"(i), "m"(j)  :"%eax","%edx","%cc");

#elif defined(TFM_X86_64)
/* x86-64 optimized */

/* anything you need at the start */
#define COMBA_START

/* clear the chaining variables */
#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;

/* forward the carry to the next digit */
#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);

/* store the first sum */
#define COMBA_STORE(x) \
   x = c0;

/* store the second sum [carry] */
#define COMBA_STORE2(x) \
   x = c1;

/* anything you need at the end */
#define COMBA_FINI

/* this should multiply i and j  */
#define MULADD(i, j)                                      \
asm  (                                                    \
     "movq  %6,%%rax     \n\t"                            \
     "mulq  %7           \n\t"                            \
     "addq  %%rax,%0     \n\t"                            \
     "adcq  %%rdx,%1     \n\t"                            \
     "adcq  $0,%2        \n\t"                            \
     :"=r"(c0), "=r"(c1), "=r"(c2): "0"(c0), "1"(c1), "2"(c2), "g"(i), "g"(j)  :"%rax","%rdx","%cc");

#elif defined(TFM_SSE2)
/* use SSE2 optimizations */

/* anything you need at the start */
#define COMBA_START

/* clear the chaining variables */
#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;

/* forward the carry to the next digit */
#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);

/* store the first sum */
#define COMBA_STORE(x) \
   x = c0;

/* store the second sum [carry] */
#define COMBA_STORE2(x) \
   x = c1;

/* anything you need at the end */
#define COMBA_FINI \
   asm("emms");

/* this should multiply i and j  */
#define MULADD(i, j)                                     \
asm(                                                     \
    "movd  %6,%%mm0     \n\t"                            \
    "movd  %7,%%mm1     \n\t"                            \
    "pmuludq %%mm1,%%mm0\n\t"                            \
    "movd  %%mm0,%%eax  \n\t"                            \
    "psrlq $32,%%mm0    \n\t"                            \
    "addl  %%eax,%0     \n\t"                            \
    "movd  %%mm0,%%eax  \n\t"                            \
    "adcl  %%eax,%1     \n\t"                            \
    "adcl  $0,%2        \n\t"                            \
    :"=r"(c0), "=r"(c1), "=r"(c2): "0"(c0), "1"(c1), "2"(c2), "m"(i), "m"(j)  :"%eax","%cc");

#elif defined(TFM_ARM)
/* ARM code */

#define COMBA_START 

#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;

#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);

#define COMBA_STORE(x) \
   x = c0;

#define COMBA_STORE2(x) \
   x = c1;

#define COMBA_FINI

#define MULADD(i, j)                                          \
asm(                                                          \
"  UMULL  r0,r1,%6,%7           \n\t"                         \
"  ADDS   %0,%0,r0              \n\t"                         \
"  ADCS   %1,%1,r1              \n\t"                         \
"  ADC    %2,%2,#0              \n\t"                         \
:"=r"(c0), "=r"(c1), "=r"(c2) : "0"(c0), "1"(c1), "2"(c2), "r"(i), "r"(j) : "r0", "r1", "%cc");

#elif defined(TFM_PPC32)
/* For 32-bit PPC */

#define COMBA_START

#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;

#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);

#define COMBA_STORE(x) \
   x = c0;

#define COMBA_STORE2(x) \
   x = c1;

#define COMBA_FINI 
   
/* untested: will mulhwu change the flags?  Docs say no */
#define MULADD(i, j)              \
asm(                              \
   " mullw  16,%6,%7       \n\t" \
   " addc   %0,%0,16       \n\t" \
   " mulhwu 16,%6,%7       \n\t" \
   " adde   %1,%1,16       \n\t" \
   " addze  %2,%2           \n\t" \
:"=r"(c0), "=r"(c1), "=r"(c2):"0"(c0), "1"(c1), "2"(c2), "r"(i), "r"(j):"16");

#else
/* ISO C code */

#define COMBA_START

#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;

#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);

#define COMBA_STORE(x) \
   x = c0;

#define COMBA_STORE2(x) \
   x = c1;

#define COMBA_FINI 
   
#define MULADD(i, j)                                                              \
   do { fp_word t;                                                                \
   t = (fp_word)c0 + ((fp_word)i) * ((fp_word)j); c0 = t;                         \
   t = (fp_word)c1 + (t >> DIGIT_BIT);            c1 = t; c2 += t >> DIGIT_BIT;   \
   } while (0);

#endif


/* generic PxQ multiplier */
void fp_mul_comba(fp_int *A, fp_int *B, fp_int *C)
{
   int       ix, iy, iz, tx, ty, pa;
   fp_digit  c0, c1, c2, *tmpx, *tmpy;
   fp_int    tmp, *dst;

   COMBA_START;
   COMBA_CLEAR;
   
   /* get size of output and trim */
   pa = A->used + B->used;
   if (pa >= FP_SIZE) {
      pa = FP_SIZE-1;
   }

   if (A == C || B == C) {
      fp_zero(&tmp);
      dst = &tmp;
   } else {
      fp_zero(C);
      dst = C;
   }

   for (ix = 0; ix < pa; ix++) {
      /* get offsets into the two bignums */
      ty = MIN(ix, B->used-1);
      tx = ix - ty;

      /* setup temp aliases */
      tmpx = A->dp + tx;
      tmpy = B->dp + ty;

      /* this is the number of times the loop will iterrate, essentially its 
         while (tx++ < a->used && ty-- >= 0) { ... }
       */
      iy = MIN(A->used-tx, ty+1);

      /* execute loop */
      COMBA_FORWARD;
      for (iz = 0; iz < iy; ++iz) {
          MULADD(*tmpx++, *tmpy--);
      }

      /* store term */
      COMBA_STORE(dst->dp[ix]);
  }
  COMBA_FINI;

  dst->used = pa;
  dst->sign = A->sign ^ B->sign;
  fp_clamp(dst);
  fp_copy(dst, C);
}

#if defined(TFM_SMALL_SET)
void fp_mul_comba_small(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[32];
   switch (MAX(A->used, B->used)) { 

   case 1:
      memcpy(at, A->dp, 1 * sizeof(fp_digit));
      memcpy(at+1, B->dp, 1 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[1]); 
      COMBA_STORE(C->dp[0]);
      COMBA_STORE2(C->dp[1]);
      C->used = 2;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 2:
      memcpy(at, A->dp, 2 * sizeof(fp_digit));
      memcpy(at+2, B->dp, 2 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[2]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[3]);       MULADD(at[1], at[2]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[1], at[3]); 
      COMBA_STORE(C->dp[2]);
      COMBA_STORE2(C->dp[3]);
      C->used = 4;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 3:
      memcpy(at, A->dp, 3 * sizeof(fp_digit));
      memcpy(at+3, B->dp, 3 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[3]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[4]);       MULADD(at[1], at[3]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[5]);       MULADD(at[1], at[4]);       MULADD(at[2], at[3]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[1], at[5]);       MULADD(at[2], at[4]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[2], at[5]); 
      COMBA_STORE(C->dp[4]);
      COMBA_STORE2(C->dp[5]);
      C->used = 6;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 4:
      memcpy(at, A->dp, 4 * sizeof(fp_digit));
      memcpy(at+4, B->dp, 4 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[4]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[5]);       MULADD(at[1], at[4]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[6]);       MULADD(at[1], at[5]);       MULADD(at[2], at[4]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[7]);       MULADD(at[1], at[6]);       MULADD(at[2], at[5]);       MULADD(at[3], at[4]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[1], at[7]);       MULADD(at[2], at[6]);       MULADD(at[3], at[5]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[2], at[7]);       MULADD(at[3], at[6]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[3], at[7]); 
      COMBA_STORE(C->dp[6]);
      COMBA_STORE2(C->dp[7]);
      C->used = 8;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 5:
      memcpy(at, A->dp, 5 * sizeof(fp_digit));
      memcpy(at+5, B->dp, 5 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[5]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[6]);       MULADD(at[1], at[5]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[7]);       MULADD(at[1], at[6]);       MULADD(at[2], at[5]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[8]);       MULADD(at[1], at[7]);       MULADD(at[2], at[6]);       MULADD(at[3], at[5]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[9]);       MULADD(at[1], at[8]);       MULADD(at[2], at[7]);       MULADD(at[3], at[6]);       MULADD(at[4], at[5]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[1], at[9]);       MULADD(at[2], at[8]);       MULADD(at[3], at[7]);       MULADD(at[4], at[6]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[2], at[9]);       MULADD(at[3], at[8]);       MULADD(at[4], at[7]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[3], at[9]);       MULADD(at[4], at[8]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[4], at[9]); 
      COMBA_STORE(C->dp[8]);
      COMBA_STORE2(C->dp[9]);
      C->used = 10;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 6:
      memcpy(at, A->dp, 6 * sizeof(fp_digit));
      memcpy(at+6, B->dp, 6 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[6]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[7]);       MULADD(at[1], at[6]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[8]);       MULADD(at[1], at[7]);       MULADD(at[2], at[6]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[9]);       MULADD(at[1], at[8]);       MULADD(at[2], at[7]);       MULADD(at[3], at[6]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[10]);       MULADD(at[1], at[9]);       MULADD(at[2], at[8]);       MULADD(at[3], at[7]);       MULADD(at[4], at[6]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[11]);       MULADD(at[1], at[10]);       MULADD(at[2], at[9]);       MULADD(at[3], at[8]);       MULADD(at[4], at[7]);       MULADD(at[5], at[6]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[1], at[11]);       MULADD(at[2], at[10]);       MULADD(at[3], at[9]);       MULADD(at[4], at[8]);       MULADD(at[5], at[7]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[2], at[11]);       MULADD(at[3], at[10]);       MULADD(at[4], at[9]);       MULADD(at[5], at[8]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[3], at[11]);       MULADD(at[4], at[10]);       MULADD(at[5], at[9]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[4], at[11]);       MULADD(at[5], at[10]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[5], at[11]); 
      COMBA_STORE(C->dp[10]);
      COMBA_STORE2(C->dp[11]);
      C->used = 12;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 7:
      memcpy(at, A->dp, 7 * sizeof(fp_digit));
      memcpy(at+7, B->dp, 7 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[7]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[8]);       MULADD(at[1], at[7]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[9]);       MULADD(at[1], at[8]);       MULADD(at[2], at[7]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[10]);       MULADD(at[1], at[9]);       MULADD(at[2], at[8]);       MULADD(at[3], at[7]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[11]);       MULADD(at[1], at[10]);       MULADD(at[2], at[9]);       MULADD(at[3], at[8]);       MULADD(at[4], at[7]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[12]);       MULADD(at[1], at[11]);       MULADD(at[2], at[10]);       MULADD(at[3], at[9]);       MULADD(at[4], at[8]);       MULADD(at[5], at[7]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[13]);       MULADD(at[1], at[12]);       MULADD(at[2], at[11]);       MULADD(at[3], at[10]);       MULADD(at[4], at[9]);       MULADD(at[5], at[8]);       MULADD(at[6], at[7]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[1], at[13]);       MULADD(at[2], at[12]);       MULADD(at[3], at[11]);       MULADD(at[4], at[10]);       MULADD(at[5], at[9]);       MULADD(at[6], at[8]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[2], at[13]);       MULADD(at[3], at[12]);       MULADD(at[4], at[11]);       MULADD(at[5], at[10]);       MULADD(at[6], at[9]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[3], at[13]);       MULADD(at[4], at[12]);       MULADD(at[5], at[11]);       MULADD(at[6], at[10]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[4], at[13]);       MULADD(at[5], at[12]);       MULADD(at[6], at[11]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[5], at[13]);       MULADD(at[6], at[12]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[6], at[13]); 
      COMBA_STORE(C->dp[12]);
      COMBA_STORE2(C->dp[13]);
      C->used = 14;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 8:
      memcpy(at, A->dp, 8 * sizeof(fp_digit));
      memcpy(at+8, B->dp, 8 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[8]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[9]);       MULADD(at[1], at[8]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[10]);       MULADD(at[1], at[9]);       MULADD(at[2], at[8]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[11]);       MULADD(at[1], at[10]);       MULADD(at[2], at[9]);       MULADD(at[3], at[8]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[12]);       MULADD(at[1], at[11]);       MULADD(at[2], at[10]);       MULADD(at[3], at[9]);       MULADD(at[4], at[8]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[13]);       MULADD(at[1], at[12]);       MULADD(at[2], at[11]);       MULADD(at[3], at[10]);       MULADD(at[4], at[9]);       MULADD(at[5], at[8]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[14]);       MULADD(at[1], at[13]);       MULADD(at[2], at[12]);       MULADD(at[3], at[11]);       MULADD(at[4], at[10]);       MULADD(at[5], at[9]);       MULADD(at[6], at[8]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]);       MULADD(at[2], at[13]);       MULADD(at[3], at[12]);       MULADD(at[4], at[11]);       MULADD(at[5], at[10]);       MULADD(at[6], at[9]);       MULADD(at[7], at[8]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[1], at[15]);       MULADD(at[2], at[14]);       MULADD(at[3], at[13]);       MULADD(at[4], at[12]);       MULADD(at[5], at[11]);       MULADD(at[6], at[10]);       MULADD(at[7], at[9]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[2], at[15]);       MULADD(at[3], at[14]);       MULADD(at[4], at[13]);       MULADD(at[5], at[12]);       MULADD(at[6], at[11]);       MULADD(at[7], at[10]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[3], at[15]);       MULADD(at[4], at[14]);       MULADD(at[5], at[13]);       MULADD(at[6], at[12]);       MULADD(at[7], at[11]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[4], at[15]);       MULADD(at[5], at[14]);       MULADD(at[6], at[13]);       MULADD(at[7], at[12]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[5], at[15]);       MULADD(at[6], at[14]);       MULADD(at[7], at[13]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[6], at[15]);       MULADD(at[7], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[7], at[15]); 
      COMBA_STORE(C->dp[14]);
      COMBA_STORE2(C->dp[15]);
      C->used = 16;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 9:
      memcpy(at, A->dp, 9 * sizeof(fp_digit));
      memcpy(at+9, B->dp, 9 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[9]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[10]);       MULADD(at[1], at[9]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[11]);       MULADD(at[1], at[10]);       MULADD(at[2], at[9]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[12]);       MULADD(at[1], at[11]);       MULADD(at[2], at[10]);       MULADD(at[3], at[9]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[13]);       MULADD(at[1], at[12]);       MULADD(at[2], at[11]);       MULADD(at[3], at[10]);       MULADD(at[4], at[9]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[14]);       MULADD(at[1], at[13]);       MULADD(at[2], at[12]);       MULADD(at[3], at[11]);       MULADD(at[4], at[10]);       MULADD(at[5], at[9]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]);       MULADD(at[2], at[13]);       MULADD(at[3], at[12]);       MULADD(at[4], at[11]);       MULADD(at[5], at[10]);       MULADD(at[6], at[9]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]);       MULADD(at[2], at[14]);       MULADD(at[3], at[13]);       MULADD(at[4], at[12]);       MULADD(at[5], at[11]);       MULADD(at[6], at[10]);       MULADD(at[7], at[9]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]);       MULADD(at[3], at[14]);       MULADD(at[4], at[13]);       MULADD(at[5], at[12]);       MULADD(at[6], at[11]);       MULADD(at[7], at[10]);       MULADD(at[8], at[9]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]);       MULADD(at[4], at[14]);       MULADD(at[5], at[13]);       MULADD(at[6], at[12]);       MULADD(at[7], at[11]);       MULADD(at[8], at[10]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]);       MULADD(at[5], at[14]);       MULADD(at[6], at[13]);       MULADD(at[7], at[12]);       MULADD(at[8], at[11]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]);       MULADD(at[6], at[14]);       MULADD(at[7], at[13]);       MULADD(at[8], at[12]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]);       MULADD(at[7], at[14]);       MULADD(at[8], at[13]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]);       MULADD(at[8], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[7], at[17]);       MULADD(at[8], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[8], at[17]); 
      COMBA_STORE(C->dp[16]);
      COMBA_STORE2(C->dp[17]);
      C->used = 18;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 10:
      memcpy(at, A->dp, 10 * sizeof(fp_digit));
      memcpy(at+10, B->dp, 10 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[10]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[11]);       MULADD(at[1], at[10]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[12]);       MULADD(at[1], at[11]);       MULADD(at[2], at[10]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[13]);       MULADD(at[1], at[12]);       MULADD(at[2], at[11]);       MULADD(at[3], at[10]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[14]);       MULADD(at[1], at[13]);       MULADD(at[2], at[12]);       MULADD(at[3], at[11]);       MULADD(at[4], at[10]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]);       MULADD(at[2], at[13]);       MULADD(at[3], at[12]);       MULADD(at[4], at[11]);       MULADD(at[5], at[10]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]);       MULADD(at[2], at[14]);       MULADD(at[3], at[13]);       MULADD(at[4], at[12]);       MULADD(at[5], at[11]);       MULADD(at[6], at[10]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]);       MULADD(at[3], at[14]);       MULADD(at[4], at[13]);       MULADD(at[5], at[12]);       MULADD(at[6], at[11]);       MULADD(at[7], at[10]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]);       MULADD(at[4], at[14]);       MULADD(at[5], at[13]);       MULADD(at[6], at[12]);       MULADD(at[7], at[11]);       MULADD(at[8], at[10]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]);       MULADD(at[5], at[14]);       MULADD(at[6], at[13]);       MULADD(at[7], at[12]);       MULADD(at[8], at[11]);       MULADD(at[9], at[10]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]);       MULADD(at[6], at[14]);       MULADD(at[7], at[13]);       MULADD(at[8], at[12]);       MULADD(at[9], at[11]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]);       MULADD(at[7], at[14]);       MULADD(at[8], at[13]);       MULADD(at[9], at[12]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]);       MULADD(at[8], at[14]);       MULADD(at[9], at[13]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]);       MULADD(at[9], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]);       MULADD(at[9], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[8], at[19]);       MULADD(at[9], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[9], at[19]); 
      COMBA_STORE(C->dp[18]);
      COMBA_STORE2(C->dp[19]);
      C->used = 20;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 11:
      memcpy(at, A->dp, 11 * sizeof(fp_digit));
      memcpy(at+11, B->dp, 11 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[11]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[12]);       MULADD(at[1], at[11]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[13]);       MULADD(at[1], at[12]);       MULADD(at[2], at[11]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[14]);       MULADD(at[1], at[13]);       MULADD(at[2], at[12]);       MULADD(at[3], at[11]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]);       MULADD(at[2], at[13]);       MULADD(at[3], at[12]);       MULADD(at[4], at[11]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]);       MULADD(at[2], at[14]);       MULADD(at[3], at[13]);       MULADD(at[4], at[12]);       MULADD(at[5], at[11]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]);       MULADD(at[3], at[14]);       MULADD(at[4], at[13]);       MULADD(at[5], at[12]);       MULADD(at[6], at[11]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]);       MULADD(at[4], at[14]);       MULADD(at[5], at[13]);       MULADD(at[6], at[12]);       MULADD(at[7], at[11]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]);       MULADD(at[5], at[14]);       MULADD(at[6], at[13]);       MULADD(at[7], at[12]);       MULADD(at[8], at[11]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[20]);       MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]);       MULADD(at[6], at[14]);       MULADD(at[7], at[13]);       MULADD(at[8], at[12]);       MULADD(at[9], at[11]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[0], at[21]);       MULADD(at[1], at[20]);       MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]);       MULADD(at[7], at[14]);       MULADD(at[8], at[13]);       MULADD(at[9], at[12]);       MULADD(at[10], at[11]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[1], at[21]);       MULADD(at[2], at[20]);       MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]);       MULADD(at[8], at[14]);       MULADD(at[9], at[13]);       MULADD(at[10], at[12]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[2], at[21]);       MULADD(at[3], at[20]);       MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]);       MULADD(at[9], at[14]);       MULADD(at[10], at[13]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[3], at[21]);       MULADD(at[4], at[20]);       MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]);       MULADD(at[9], at[15]);       MULADD(at[10], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[4], at[21]);       MULADD(at[5], at[20]);       MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]);       MULADD(at[10], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[5], at[21]);       MULADD(at[6], at[20]);       MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]);       MULADD(at[10], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[6], at[21]);       MULADD(at[7], at[20]);       MULADD(at[8], at[19]);       MULADD(at[9], at[18]);       MULADD(at[10], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[7], at[21]);       MULADD(at[8], at[20]);       MULADD(at[9], at[19]);       MULADD(at[10], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[8], at[21]);       MULADD(at[9], at[20]);       MULADD(at[10], at[19]); 
      COMBA_STORE(C->dp[18]);
      /* 19 */
      COMBA_FORWARD;
      MULADD(at[9], at[21]);       MULADD(at[10], at[20]); 
      COMBA_STORE(C->dp[19]);
      /* 20 */
      COMBA_FORWARD;
      MULADD(at[10], at[21]); 
      COMBA_STORE(C->dp[20]);
      COMBA_STORE2(C->dp[21]);
      C->used = 22;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 12:
      memcpy(at, A->dp, 12 * sizeof(fp_digit));
      memcpy(at+12, B->dp, 12 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[12]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[13]);       MULADD(at[1], at[12]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[14]);       MULADD(at[1], at[13]);       MULADD(at[2], at[12]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]);       MULADD(at[2], at[13]);       MULADD(at[3], at[12]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]);       MULADD(at[2], at[14]);       MULADD(at[3], at[13]);       MULADD(at[4], at[12]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]);       MULADD(at[3], at[14]);       MULADD(at[4], at[13]);       MULADD(at[5], at[12]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]);       MULADD(at[4], at[14]);       MULADD(at[5], at[13]);       MULADD(at[6], at[12]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]);       MULADD(at[5], at[14]);       MULADD(at[6], at[13]);       MULADD(at[7], at[12]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[20]);       MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]);       MULADD(at[6], at[14]);       MULADD(at[7], at[13]);       MULADD(at[8], at[12]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[21]);       MULADD(at[1], at[20]);       MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]);       MULADD(at[7], at[14]);       MULADD(at[8], at[13]);       MULADD(at[9], at[12]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[0], at[22]);       MULADD(at[1], at[21]);       MULADD(at[2], at[20]);       MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]);       MULADD(at[8], at[14]);       MULADD(at[9], at[13]);       MULADD(at[10], at[12]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[0], at[23]);       MULADD(at[1], at[22]);       MULADD(at[2], at[21]);       MULADD(at[3], at[20]);       MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]);       MULADD(at[9], at[14]);       MULADD(at[10], at[13]);       MULADD(at[11], at[12]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[1], at[23]);       MULADD(at[2], at[22]);       MULADD(at[3], at[21]);       MULADD(at[4], at[20]);       MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]);       MULADD(at[9], at[15]);       MULADD(at[10], at[14]);       MULADD(at[11], at[13]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[2], at[23]);       MULADD(at[3], at[22]);       MULADD(at[4], at[21]);       MULADD(at[5], at[20]);       MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]);       MULADD(at[10], at[15]);       MULADD(at[11], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[3], at[23]);       MULADD(at[4], at[22]);       MULADD(at[5], at[21]);       MULADD(at[6], at[20]);       MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]);       MULADD(at[10], at[16]);       MULADD(at[11], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[4], at[23]);       MULADD(at[5], at[22]);       MULADD(at[6], at[21]);       MULADD(at[7], at[20]);       MULADD(at[8], at[19]);       MULADD(at[9], at[18]);       MULADD(at[10], at[17]);       MULADD(at[11], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[5], at[23]);       MULADD(at[6], at[22]);       MULADD(at[7], at[21]);       MULADD(at[8], at[20]);       MULADD(at[9], at[19]);       MULADD(at[10], at[18]);       MULADD(at[11], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[6], at[23]);       MULADD(at[7], at[22]);       MULADD(at[8], at[21]);       MULADD(at[9], at[20]);       MULADD(at[10], at[19]);       MULADD(at[11], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[7], at[23]);       MULADD(at[8], at[22]);       MULADD(at[9], at[21]);       MULADD(at[10], at[20]);       MULADD(at[11], at[19]); 
      COMBA_STORE(C->dp[18]);
      /* 19 */
      COMBA_FORWARD;
      MULADD(at[8], at[23]);       MULADD(at[9], at[22]);       MULADD(at[10], at[21]);       MULADD(at[11], at[20]); 
      COMBA_STORE(C->dp[19]);
      /* 20 */
      COMBA_FORWARD;
      MULADD(at[9], at[23]);       MULADD(at[10], at[22]);       MULADD(at[11], at[21]); 
      COMBA_STORE(C->dp[20]);
      /* 21 */
      COMBA_FORWARD;
      MULADD(at[10], at[23]);       MULADD(at[11], at[22]); 
      COMBA_STORE(C->dp[21]);
      /* 22 */
      COMBA_FORWARD;
      MULADD(at[11], at[23]); 
      COMBA_STORE(C->dp[22]);
      COMBA_STORE2(C->dp[23]);
      C->used = 24;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 13:
      memcpy(at, A->dp, 13 * sizeof(fp_digit));
      memcpy(at+13, B->dp, 13 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[13]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[14]);       MULADD(at[1], at[13]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]);       MULADD(at[2], at[13]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]);       MULADD(at[2], at[14]);       MULADD(at[3], at[13]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]);       MULADD(at[3], at[14]);       MULADD(at[4], at[13]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]);       MULADD(at[4], at[14]);       MULADD(at[5], at[13]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]);       MULADD(at[5], at[14]);       MULADD(at[6], at[13]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[20]);       MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]);       MULADD(at[6], at[14]);       MULADD(at[7], at[13]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[21]);       MULADD(at[1], at[20]);       MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]);       MULADD(at[7], at[14]);       MULADD(at[8], at[13]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[22]);       MULADD(at[1], at[21]);       MULADD(at[2], at[20]);       MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]);       MULADD(at[8], at[14]);       MULADD(at[9], at[13]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[0], at[23]);       MULADD(at[1], at[22]);       MULADD(at[2], at[21]);       MULADD(at[3], at[20]);       MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]);       MULADD(at[9], at[14]);       MULADD(at[10], at[13]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[0], at[24]);       MULADD(at[1], at[23]);       MULADD(at[2], at[22]);       MULADD(at[3], at[21]);       MULADD(at[4], at[20]);       MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]);       MULADD(at[9], at[15]);       MULADD(at[10], at[14]);       MULADD(at[11], at[13]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[0], at[25]);       MULADD(at[1], at[24]);       MULADD(at[2], at[23]);       MULADD(at[3], at[22]);       MULADD(at[4], at[21]);       MULADD(at[5], at[20]);       MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]);       MULADD(at[10], at[15]);       MULADD(at[11], at[14]);       MULADD(at[12], at[13]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[1], at[25]);       MULADD(at[2], at[24]);       MULADD(at[3], at[23]);       MULADD(at[4], at[22]);       MULADD(at[5], at[21]);       MULADD(at[6], at[20]);       MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]);       MULADD(at[10], at[16]);       MULADD(at[11], at[15]);       MULADD(at[12], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[2], at[25]);       MULADD(at[3], at[24]);       MULADD(at[4], at[23]);       MULADD(at[5], at[22]);       MULADD(at[6], at[21]);       MULADD(at[7], at[20]);       MULADD(at[8], at[19]);       MULADD(at[9], at[18]);       MULADD(at[10], at[17]);       MULADD(at[11], at[16]);       MULADD(at[12], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[3], at[25]);       MULADD(at[4], at[24]);       MULADD(at[5], at[23]);       MULADD(at[6], at[22]);       MULADD(at[7], at[21]);       MULADD(at[8], at[20]);       MULADD(at[9], at[19]);       MULADD(at[10], at[18]);       MULADD(at[11], at[17]);       MULADD(at[12], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[4], at[25]);       MULADD(at[5], at[24]);       MULADD(at[6], at[23]);       MULADD(at[7], at[22]);       MULADD(at[8], at[21]);       MULADD(at[9], at[20]);       MULADD(at[10], at[19]);       MULADD(at[11], at[18]);       MULADD(at[12], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[5], at[25]);       MULADD(at[6], at[24]);       MULADD(at[7], at[23]);       MULADD(at[8], at[22]);       MULADD(at[9], at[21]);       MULADD(at[10], at[20]);       MULADD(at[11], at[19]);       MULADD(at[12], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[6], at[25]);       MULADD(at[7], at[24]);       MULADD(at[8], at[23]);       MULADD(at[9], at[22]);       MULADD(at[10], at[21]);       MULADD(at[11], at[20]);       MULADD(at[12], at[19]); 
      COMBA_STORE(C->dp[18]);
      /* 19 */
      COMBA_FORWARD;
      MULADD(at[7], at[25]);       MULADD(at[8], at[24]);       MULADD(at[9], at[23]);       MULADD(at[10], at[22]);       MULADD(at[11], at[21]);       MULADD(at[12], at[20]); 
      COMBA_STORE(C->dp[19]);
      /* 20 */
      COMBA_FORWARD;
      MULADD(at[8], at[25]);       MULADD(at[9], at[24]);       MULADD(at[10], at[23]);       MULADD(at[11], at[22]);       MULADD(at[12], at[21]); 
      COMBA_STORE(C->dp[20]);
      /* 21 */
      COMBA_FORWARD;
      MULADD(at[9], at[25]);       MULADD(at[10], at[24]);       MULADD(at[11], at[23]);       MULADD(at[12], at[22]); 
      COMBA_STORE(C->dp[21]);
      /* 22 */
      COMBA_FORWARD;
      MULADD(at[10], at[25]);       MULADD(at[11], at[24]);       MULADD(at[12], at[23]); 
      COMBA_STORE(C->dp[22]);
      /* 23 */
      COMBA_FORWARD;
      MULADD(at[11], at[25]);       MULADD(at[12], at[24]); 
      COMBA_STORE(C->dp[23]);
      /* 24 */
      COMBA_FORWARD;
      MULADD(at[12], at[25]); 
      COMBA_STORE(C->dp[24]);
      COMBA_STORE2(C->dp[25]);
      C->used = 26;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 14:
      memcpy(at, A->dp, 14 * sizeof(fp_digit));
      memcpy(at+14, B->dp, 14 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[14]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[15]);       MULADD(at[1], at[14]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]);       MULADD(at[2], at[14]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]);       MULADD(at[3], at[14]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]);       MULADD(at[4], at[14]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]);       MULADD(at[5], at[14]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[20]);       MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]);       MULADD(at[6], at[14]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[21]);       MULADD(at[1], at[20]);       MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]);       MULADD(at[7], at[14]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[22]);       MULADD(at[1], at[21]);       MULADD(at[2], at[20]);       MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]);       MULADD(at[8], at[14]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[23]);       MULADD(at[1], at[22]);       MULADD(at[2], at[21]);       MULADD(at[3], at[20]);       MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]);       MULADD(at[9], at[14]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[0], at[24]);       MULADD(at[1], at[23]);       MULADD(at[2], at[22]);       MULADD(at[3], at[21]);       MULADD(at[4], at[20]);       MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]);       MULADD(at[9], at[15]);       MULADD(at[10], at[14]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[0], at[25]);       MULADD(at[1], at[24]);       MULADD(at[2], at[23]);       MULADD(at[3], at[22]);       MULADD(at[4], at[21]);       MULADD(at[5], at[20]);       MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]);       MULADD(at[10], at[15]);       MULADD(at[11], at[14]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[0], at[26]);       MULADD(at[1], at[25]);       MULADD(at[2], at[24]);       MULADD(at[3], at[23]);       MULADD(at[4], at[22]);       MULADD(at[5], at[21]);       MULADD(at[6], at[20]);       MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]);       MULADD(at[10], at[16]);       MULADD(at[11], at[15]);       MULADD(at[12], at[14]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[0], at[27]);       MULADD(at[1], at[26]);       MULADD(at[2], at[25]);       MULADD(at[3], at[24]);       MULADD(at[4], at[23]);       MULADD(at[5], at[22]);       MULADD(at[6], at[21]);       MULADD(at[7], at[20]);       MULADD(at[8], at[19]);       MULADD(at[9], at[18]);       MULADD(at[10], at[17]);       MULADD(at[11], at[16]);       MULADD(at[12], at[15]);       MULADD(at[13], at[14]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[1], at[27]);       MULADD(at[2], at[26]);       MULADD(at[3], at[25]);       MULADD(at[4], at[24]);       MULADD(at[5], at[23]);       MULADD(at[6], at[22]);       MULADD(at[7], at[21]);       MULADD(at[8], at[20]);       MULADD(at[9], at[19]);       MULADD(at[10], at[18]);       MULADD(at[11], at[17]);       MULADD(at[12], at[16]);       MULADD(at[13], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[2], at[27]);       MULADD(at[3], at[26]);       MULADD(at[4], at[25]);       MULADD(at[5], at[24]);       MULADD(at[6], at[23]);       MULADD(at[7], at[22]);       MULADD(at[8], at[21]);       MULADD(at[9], at[20]);       MULADD(at[10], at[19]);       MULADD(at[11], at[18]);       MULADD(at[12], at[17]);       MULADD(at[13], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[3], at[27]);       MULADD(at[4], at[26]);       MULADD(at[5], at[25]);       MULADD(at[6], at[24]);       MULADD(at[7], at[23]);       MULADD(at[8], at[22]);       MULADD(at[9], at[21]);       MULADD(at[10], at[20]);       MULADD(at[11], at[19]);       MULADD(at[12], at[18]);       MULADD(at[13], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[4], at[27]);       MULADD(at[5], at[26]);       MULADD(at[6], at[25]);       MULADD(at[7], at[24]);       MULADD(at[8], at[23]);       MULADD(at[9], at[22]);       MULADD(at[10], at[21]);       MULADD(at[11], at[20]);       MULADD(at[12], at[19]);       MULADD(at[13], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[5], at[27]);       MULADD(at[6], at[26]);       MULADD(at[7], at[25]);       MULADD(at[8], at[24]);       MULADD(at[9], at[23]);       MULADD(at[10], at[22]);       MULADD(at[11], at[21]);       MULADD(at[12], at[20]);       MULADD(at[13], at[19]); 
      COMBA_STORE(C->dp[18]);
      /* 19 */
      COMBA_FORWARD;
      MULADD(at[6], at[27]);       MULADD(at[7], at[26]);       MULADD(at[8], at[25]);       MULADD(at[9], at[24]);       MULADD(at[10], at[23]);       MULADD(at[11], at[22]);       MULADD(at[12], at[21]);       MULADD(at[13], at[20]); 
      COMBA_STORE(C->dp[19]);
      /* 20 */
      COMBA_FORWARD;
      MULADD(at[7], at[27]);       MULADD(at[8], at[26]);       MULADD(at[9], at[25]);       MULADD(at[10], at[24]);       MULADD(at[11], at[23]);       MULADD(at[12], at[22]);       MULADD(at[13], at[21]); 
      COMBA_STORE(C->dp[20]);
      /* 21 */
      COMBA_FORWARD;
      MULADD(at[8], at[27]);       MULADD(at[9], at[26]);       MULADD(at[10], at[25]);       MULADD(at[11], at[24]);       MULADD(at[12], at[23]);       MULADD(at[13], at[22]); 
      COMBA_STORE(C->dp[21]);
      /* 22 */
      COMBA_FORWARD;
      MULADD(at[9], at[27]);       MULADD(at[10], at[26]);       MULADD(at[11], at[25]);       MULADD(at[12], at[24]);       MULADD(at[13], at[23]); 
      COMBA_STORE(C->dp[22]);
      /* 23 */
      COMBA_FORWARD;
      MULADD(at[10], at[27]);       MULADD(at[11], at[26]);       MULADD(at[12], at[25]);       MULADD(at[13], at[24]); 
      COMBA_STORE(C->dp[23]);
      /* 24 */
      COMBA_FORWARD;
      MULADD(at[11], at[27]);       MULADD(at[12], at[26]);       MULADD(at[13], at[25]); 
      COMBA_STORE(C->dp[24]);
      /* 25 */
      COMBA_FORWARD;
      MULADD(at[12], at[27]);       MULADD(at[13], at[26]); 
      COMBA_STORE(C->dp[25]);
      /* 26 */
      COMBA_FORWARD;
      MULADD(at[13], at[27]); 
      COMBA_STORE(C->dp[26]);
      COMBA_STORE2(C->dp[27]);
      C->used = 28;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 15:
      memcpy(at, A->dp, 15 * sizeof(fp_digit));
      memcpy(at+15, B->dp, 15 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[15]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[16]);       MULADD(at[1], at[15]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]);       MULADD(at[2], at[15]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]);       MULADD(at[3], at[15]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]);       MULADD(at[4], at[15]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[20]);       MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]);       MULADD(at[5], at[15]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[21]);       MULADD(at[1], at[20]);       MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]);       MULADD(at[6], at[15]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[22]);       MULADD(at[1], at[21]);       MULADD(at[2], at[20]);       MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]);       MULADD(at[7], at[15]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[23]);       MULADD(at[1], at[22]);       MULADD(at[2], at[21]);       MULADD(at[3], at[20]);       MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]);       MULADD(at[8], at[15]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[24]);       MULADD(at[1], at[23]);       MULADD(at[2], at[22]);       MULADD(at[3], at[21]);       MULADD(at[4], at[20]);       MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]);       MULADD(at[9], at[15]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[0], at[25]);       MULADD(at[1], at[24]);       MULADD(at[2], at[23]);       MULADD(at[3], at[22]);       MULADD(at[4], at[21]);       MULADD(at[5], at[20]);       MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]);       MULADD(at[10], at[15]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[0], at[26]);       MULADD(at[1], at[25]);       MULADD(at[2], at[24]);       MULADD(at[3], at[23]);       MULADD(at[4], at[22]);       MULADD(at[5], at[21]);       MULADD(at[6], at[20]);       MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]);       MULADD(at[10], at[16]);       MULADD(at[11], at[15]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[0], at[27]);       MULADD(at[1], at[26]);       MULADD(at[2], at[25]);       MULADD(at[3], at[24]);       MULADD(at[4], at[23]);       MULADD(at[5], at[22]);       MULADD(at[6], at[21]);       MULADD(at[7], at[20]);       MULADD(at[8], at[19]);       MULADD(at[9], at[18]);       MULADD(at[10], at[17]);       MULADD(at[11], at[16]);       MULADD(at[12], at[15]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[0], at[28]);       MULADD(at[1], at[27]);       MULADD(at[2], at[26]);       MULADD(at[3], at[25]);       MULADD(at[4], at[24]);       MULADD(at[5], at[23]);       MULADD(at[6], at[22]);       MULADD(at[7], at[21]);       MULADD(at[8], at[20]);       MULADD(at[9], at[19]);       MULADD(at[10], at[18]);       MULADD(at[11], at[17]);       MULADD(at[12], at[16]);       MULADD(at[13], at[15]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[0], at[29]);       MULADD(at[1], at[28]);       MULADD(at[2], at[27]);       MULADD(at[3], at[26]);       MULADD(at[4], at[25]);       MULADD(at[5], at[24]);       MULADD(at[6], at[23]);       MULADD(at[7], at[22]);       MULADD(at[8], at[21]);       MULADD(at[9], at[20]);       MULADD(at[10], at[19]);       MULADD(at[11], at[18]);       MULADD(at[12], at[17]);       MULADD(at[13], at[16]);       MULADD(at[14], at[15]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[1], at[29]);       MULADD(at[2], at[28]);       MULADD(at[3], at[27]);       MULADD(at[4], at[26]);       MULADD(at[5], at[25]);       MULADD(at[6], at[24]);       MULADD(at[7], at[23]);       MULADD(at[8], at[22]);       MULADD(at[9], at[21]);       MULADD(at[10], at[20]);       MULADD(at[11], at[19]);       MULADD(at[12], at[18]);       MULADD(at[13], at[17]);       MULADD(at[14], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[2], at[29]);       MULADD(at[3], at[28]);       MULADD(at[4], at[27]);       MULADD(at[5], at[26]);       MULADD(at[6], at[25]);       MULADD(at[7], at[24]);       MULADD(at[8], at[23]);       MULADD(at[9], at[22]);       MULADD(at[10], at[21]);       MULADD(at[11], at[20]);       MULADD(at[12], at[19]);       MULADD(at[13], at[18]);       MULADD(at[14], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[3], at[29]);       MULADD(at[4], at[28]);       MULADD(at[5], at[27]);       MULADD(at[6], at[26]);       MULADD(at[7], at[25]);       MULADD(at[8], at[24]);       MULADD(at[9], at[23]);       MULADD(at[10], at[22]);       MULADD(at[11], at[21]);       MULADD(at[12], at[20]);       MULADD(at[13], at[19]);       MULADD(at[14], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[4], at[29]);       MULADD(at[5], at[28]);       MULADD(at[6], at[27]);       MULADD(at[7], at[26]);       MULADD(at[8], at[25]);       MULADD(at[9], at[24]);       MULADD(at[10], at[23]);       MULADD(at[11], at[22]);       MULADD(at[12], at[21]);       MULADD(at[13], at[20]);       MULADD(at[14], at[19]); 
      COMBA_STORE(C->dp[18]);
      /* 19 */
      COMBA_FORWARD;
      MULADD(at[5], at[29]);       MULADD(at[6], at[28]);       MULADD(at[7], at[27]);       MULADD(at[8], at[26]);       MULADD(at[9], at[25]);       MULADD(at[10], at[24]);       MULADD(at[11], at[23]);       MULADD(at[12], at[22]);       MULADD(at[13], at[21]);       MULADD(at[14], at[20]); 
      COMBA_STORE(C->dp[19]);
      /* 20 */
      COMBA_FORWARD;
      MULADD(at[6], at[29]);       MULADD(at[7], at[28]);       MULADD(at[8], at[27]);       MULADD(at[9], at[26]);       MULADD(at[10], at[25]);       MULADD(at[11], at[24]);       MULADD(at[12], at[23]);       MULADD(at[13], at[22]);       MULADD(at[14], at[21]); 
      COMBA_STORE(C->dp[20]);
      /* 21 */
      COMBA_FORWARD;
      MULADD(at[7], at[29]);       MULADD(at[8], at[28]);       MULADD(at[9], at[27]);       MULADD(at[10], at[26]);       MULADD(at[11], at[25]);       MULADD(at[12], at[24]);       MULADD(at[13], at[23]);       MULADD(at[14], at[22]); 
      COMBA_STORE(C->dp[21]);
      /* 22 */
      COMBA_FORWARD;
      MULADD(at[8], at[29]);       MULADD(at[9], at[28]);       MULADD(at[10], at[27]);       MULADD(at[11], at[26]);       MULADD(at[12], at[25]);       MULADD(at[13], at[24]);       MULADD(at[14], at[23]); 
      COMBA_STORE(C->dp[22]);
      /* 23 */
      COMBA_FORWARD;
      MULADD(at[9], at[29]);       MULADD(at[10], at[28]);       MULADD(at[11], at[27]);       MULADD(at[12], at[26]);       MULADD(at[13], at[25]);       MULADD(at[14], at[24]); 
      COMBA_STORE(C->dp[23]);
      /* 24 */
      COMBA_FORWARD;
      MULADD(at[10], at[29]);       MULADD(at[11], at[28]);       MULADD(at[12], at[27]);       MULADD(at[13], at[26]);       MULADD(at[14], at[25]); 
      COMBA_STORE(C->dp[24]);
      /* 25 */
      COMBA_FORWARD;
      MULADD(at[11], at[29]);       MULADD(at[12], at[28]);       MULADD(at[13], at[27]);       MULADD(at[14], at[26]); 
      COMBA_STORE(C->dp[25]);
      /* 26 */
      COMBA_FORWARD;
      MULADD(at[12], at[29]);       MULADD(at[13], at[28]);       MULADD(at[14], at[27]); 
      COMBA_STORE(C->dp[26]);
      /* 27 */
      COMBA_FORWARD;
      MULADD(at[13], at[29]);       MULADD(at[14], at[28]); 
      COMBA_STORE(C->dp[27]);
      /* 28 */
      COMBA_FORWARD;
      MULADD(at[14], at[29]); 
      COMBA_STORE(C->dp[28]);
      COMBA_STORE2(C->dp[29]);
      C->used = 30;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;

   case 16:
      memcpy(at, A->dp, 16 * sizeof(fp_digit));
      memcpy(at+16, B->dp, 16 * sizeof(fp_digit));
      COMBA_START;

      COMBA_CLEAR;
      /* 0 */
      MULADD(at[0], at[16]); 
      COMBA_STORE(C->dp[0]);
      /* 1 */
      COMBA_FORWARD;
      MULADD(at[0], at[17]);       MULADD(at[1], at[16]); 
      COMBA_STORE(C->dp[1]);
      /* 2 */
      COMBA_FORWARD;
      MULADD(at[0], at[18]);       MULADD(at[1], at[17]);       MULADD(at[2], at[16]); 
      COMBA_STORE(C->dp[2]);
      /* 3 */
      COMBA_FORWARD;
      MULADD(at[0], at[19]);       MULADD(at[1], at[18]);       MULADD(at[2], at[17]);       MULADD(at[3], at[16]); 
      COMBA_STORE(C->dp[3]);
      /* 4 */
      COMBA_FORWARD;
      MULADD(at[0], at[20]);       MULADD(at[1], at[19]);       MULADD(at[2], at[18]);       MULADD(at[3], at[17]);       MULADD(at[4], at[16]); 
      COMBA_STORE(C->dp[4]);
      /* 5 */
      COMBA_FORWARD;
      MULADD(at[0], at[21]);       MULADD(at[1], at[20]);       MULADD(at[2], at[19]);       MULADD(at[3], at[18]);       MULADD(at[4], at[17]);       MULADD(at[5], at[16]); 
      COMBA_STORE(C->dp[5]);
      /* 6 */
      COMBA_FORWARD;
      MULADD(at[0], at[22]);       MULADD(at[1], at[21]);       MULADD(at[2], at[20]);       MULADD(at[3], at[19]);       MULADD(at[4], at[18]);       MULADD(at[5], at[17]);       MULADD(at[6], at[16]); 
      COMBA_STORE(C->dp[6]);
      /* 7 */
      COMBA_FORWARD;
      MULADD(at[0], at[23]);       MULADD(at[1], at[22]);       MULADD(at[2], at[21]);       MULADD(at[3], at[20]);       MULADD(at[4], at[19]);       MULADD(at[5], at[18]);       MULADD(at[6], at[17]);       MULADD(at[7], at[16]); 
      COMBA_STORE(C->dp[7]);
      /* 8 */
      COMBA_FORWARD;
      MULADD(at[0], at[24]);       MULADD(at[1], at[23]);       MULADD(at[2], at[22]);       MULADD(at[3], at[21]);       MULADD(at[4], at[20]);       MULADD(at[5], at[19]);       MULADD(at[6], at[18]);       MULADD(at[7], at[17]);       MULADD(at[8], at[16]); 
      COMBA_STORE(C->dp[8]);
      /* 9 */
      COMBA_FORWARD;
      MULADD(at[0], at[25]);       MULADD(at[1], at[24]);       MULADD(at[2], at[23]);       MULADD(at[3], at[22]);       MULADD(at[4], at[21]);       MULADD(at[5], at[20]);       MULADD(at[6], at[19]);       MULADD(at[7], at[18]);       MULADD(at[8], at[17]);       MULADD(at[9], at[16]); 
      COMBA_STORE(C->dp[9]);
      /* 10 */
      COMBA_FORWARD;
      MULADD(at[0], at[26]);       MULADD(at[1], at[25]);       MULADD(at[2], at[24]);       MULADD(at[3], at[23]);       MULADD(at[4], at[22]);       MULADD(at[5], at[21]);       MULADD(at[6], at[20]);       MULADD(at[7], at[19]);       MULADD(at[8], at[18]);       MULADD(at[9], at[17]);       MULADD(at[10], at[16]); 
      COMBA_STORE(C->dp[10]);
      /* 11 */
      COMBA_FORWARD;
      MULADD(at[0], at[27]);       MULADD(at[1], at[26]);       MULADD(at[2], at[25]);       MULADD(at[3], at[24]);       MULADD(at[4], at[23]);       MULADD(at[5], at[22]);       MULADD(at[6], at[21]);       MULADD(at[7], at[20]);       MULADD(at[8], at[19]);       MULADD(at[9], at[18]);       MULADD(at[10], at[17]);       MULADD(at[11], at[16]); 
      COMBA_STORE(C->dp[11]);
      /* 12 */
      COMBA_FORWARD;
      MULADD(at[0], at[28]);       MULADD(at[1], at[27]);       MULADD(at[2], at[26]);       MULADD(at[3], at[25]);       MULADD(at[4], at[24]);       MULADD(at[5], at[23]);       MULADD(at[6], at[22]);       MULADD(at[7], at[21]);       MULADD(at[8], at[20]);       MULADD(at[9], at[19]);       MULADD(at[10], at[18]);       MULADD(at[11], at[17]);       MULADD(at[12], at[16]); 
      COMBA_STORE(C->dp[12]);
      /* 13 */
      COMBA_FORWARD;
      MULADD(at[0], at[29]);       MULADD(at[1], at[28]);       MULADD(at[2], at[27]);       MULADD(at[3], at[26]);       MULADD(at[4], at[25]);       MULADD(at[5], at[24]);       MULADD(at[6], at[23]);       MULADD(at[7], at[22]);       MULADD(at[8], at[21]);       MULADD(at[9], at[20]);       MULADD(at[10], at[19]);       MULADD(at[11], at[18]);       MULADD(at[12], at[17]);       MULADD(at[13], at[16]); 
      COMBA_STORE(C->dp[13]);
      /* 14 */
      COMBA_FORWARD;
      MULADD(at[0], at[30]);       MULADD(at[1], at[29]);       MULADD(at[2], at[28]);       MULADD(at[3], at[27]);       MULADD(at[4], at[26]);       MULADD(at[5], at[25]);       MULADD(at[6], at[24]);       MULADD(at[7], at[23]);       MULADD(at[8], at[22]);       MULADD(at[9], at[21]);       MULADD(at[10], at[20]);       MULADD(at[11], at[19]);       MULADD(at[12], at[18]);       MULADD(at[13], at[17]);       MULADD(at[14], at[16]); 
      COMBA_STORE(C->dp[14]);
      /* 15 */
      COMBA_FORWARD;
      MULADD(at[0], at[31]);       MULADD(at[1], at[30]);       MULADD(at[2], at[29]);       MULADD(at[3], at[28]);       MULADD(at[4], at[27]);       MULADD(at[5], at[26]);       MULADD(at[6], at[25]);       MULADD(at[7], at[24]);       MULADD(at[8], at[23]);       MULADD(at[9], at[22]);       MULADD(at[10], at[21]);       MULADD(at[11], at[20]);       MULADD(at[12], at[19]);       MULADD(at[13], at[18]);       MULADD(at[14], at[17]);       MULADD(at[15], at[16]); 
      COMBA_STORE(C->dp[15]);
      /* 16 */
      COMBA_FORWARD;
      MULADD(at[1], at[31]);       MULADD(at[2], at[30]);       MULADD(at[3], at[29]);       MULADD(at[4], at[28]);       MULADD(at[5], at[27]);       MULADD(at[6], at[26]);       MULADD(at[7], at[25]);       MULADD(at[8], at[24]);       MULADD(at[9], at[23]);       MULADD(at[10], at[22]);       MULADD(at[11], at[21]);       MULADD(at[12], at[20]);       MULADD(at[13], at[19]);       MULADD(at[14], at[18]);       MULADD(at[15], at[17]); 
      COMBA_STORE(C->dp[16]);
      /* 17 */
      COMBA_FORWARD;
      MULADD(at[2], at[31]);       MULADD(at[3], at[30]);       MULADD(at[4], at[29]);       MULADD(at[5], at[28]);       MULADD(at[6], at[27]);       MULADD(at[7], at[26]);       MULADD(at[8], at[25]);       MULADD(at[9], at[24]);       MULADD(at[10], at[23]);       MULADD(at[11], at[22]);       MULADD(at[12], at[21]);       MULADD(at[13], at[20]);       MULADD(at[14], at[19]);       MULADD(at[15], at[18]); 
      COMBA_STORE(C->dp[17]);
      /* 18 */
      COMBA_FORWARD;
      MULADD(at[3], at[31]);       MULADD(at[4], at[30]);       MULADD(at[5], at[29]);       MULADD(at[6], at[28]);       MULADD(at[7], at[27]);       MULADD(at[8], at[26]);       MULADD(at[9], at[25]);       MULADD(at[10], at[24]);       MULADD(at[11], at[23]);       MULADD(at[12], at[22]);       MULADD(at[13], at[21]);       MULADD(at[14], at[20]);       MULADD(at[15], at[19]); 
      COMBA_STORE(C->dp[18]);
      /* 19 */
      COMBA_FORWARD;
      MULADD(at[4], at[31]);       MULADD(at[5], at[30]);       MULADD(at[6], at[29]);       MULADD(at[7], at[28]);       MULADD(at[8], at[27]);       MULADD(at[9], at[26]);       MULADD(at[10], at[25]);       MULADD(at[11], at[24]);       MULADD(at[12], at[23]);       MULADD(at[13], at[22]);       MULADD(at[14], at[21]);       MULADD(at[15], at[20]); 
      COMBA_STORE(C->dp[19]);
      /* 20 */
      COMBA_FORWARD;
      MULADD(at[5], at[31]);       MULADD(at[6], at[30]);       MULADD(at[7], at[29]);       MULADD(at[8], at[28]);       MULADD(at[9], at[27]);       MULADD(at[10], at[26]);       MULADD(at[11], at[25]);       MULADD(at[12], at[24]);       MULADD(at[13], at[23]);       MULADD(at[14], at[22]);       MULADD(at[15], at[21]); 
      COMBA_STORE(C->dp[20]);
      /* 21 */
      COMBA_FORWARD;
      MULADD(at[6], at[31]);       MULADD(at[7], at[30]);       MULADD(at[8], at[29]);       MULADD(at[9], at[28]);       MULADD(at[10], at[27]);       MULADD(at[11], at[26]);       MULADD(at[12], at[25]);       MULADD(at[13], at[24]);       MULADD(at[14], at[23]);       MULADD(at[15], at[22]); 
      COMBA_STORE(C->dp[21]);
      /* 22 */
      COMBA_FORWARD;
      MULADD(at[7], at[31]);       MULADD(at[8], at[30]);       MULADD(at[9], at[29]);       MULADD(at[10], at[28]);       MULADD(at[11], at[27]);       MULADD(at[12], at[26]);       MULADD(at[13], at[25]);       MULADD(at[14], at[24]);       MULADD(at[15], at[23]); 
      COMBA_STORE(C->dp[22]);
      /* 23 */
      COMBA_FORWARD;
      MULADD(at[8], at[31]);       MULADD(at[9], at[30]);       MULADD(at[10], at[29]);       MULADD(at[11], at[28]);       MULADD(at[12], at[27]);       MULADD(at[13], at[26]);       MULADD(at[14], at[25]);       MULADD(at[15], at[24]); 
      COMBA_STORE(C->dp[23]);
      /* 24 */
      COMBA_FORWARD;
      MULADD(at[9], at[31]);       MULADD(at[10], at[30]);       MULADD(at[11], at[29]);       MULADD(at[12], at[28]);       MULADD(at[13], at[27]);       MULADD(at[14], at[26]);       MULADD(at[15], at[25]); 
      COMBA_STORE(C->dp[24]);
      /* 25 */
      COMBA_FORWARD;
      MULADD(at[10], at[31]);       MULADD(at[11], at[30]);       MULADD(at[12], at[29]);       MULADD(at[13], at[28]);       MULADD(at[14], at[27]);       MULADD(at[15], at[26]); 
      COMBA_STORE(C->dp[25]);
      /* 26 */
      COMBA_FORWARD;
      MULADD(at[11], at[31]);       MULADD(at[12], at[30]);       MULADD(at[13], at[29]);       MULADD(at[14], at[28]);       MULADD(at[15], at[27]); 
      COMBA_STORE(C->dp[26]);
      /* 27 */
      COMBA_FORWARD;
      MULADD(at[12], at[31]);       MULADD(at[13], at[30]);       MULADD(at[14], at[29]);       MULADD(at[15], at[28]); 
      COMBA_STORE(C->dp[27]);
      /* 28 */
      COMBA_FORWARD;
      MULADD(at[13], at[31]);       MULADD(at[14], at[30]);       MULADD(at[15], at[29]); 
      COMBA_STORE(C->dp[28]);
      /* 29 */
      COMBA_FORWARD;
      MULADD(at[14], at[31]);       MULADD(at[15], at[30]); 
      COMBA_STORE(C->dp[29]);
      /* 30 */
      COMBA_FORWARD;
      MULADD(at[15], at[31]); 
      COMBA_STORE(C->dp[30]);
      COMBA_STORE2(C->dp[31]);
      C->used = 32;
      C->sign = A->sign ^ B->sign;
      fp_clamp(C);
      COMBA_FINI;
      break;
   }
}

#endif

#ifdef TFM_MUL32
void fp_mul_comba32(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[64];
   int out_size;

   out_size = A->used + B->used;
   memcpy(at, A->dp, 32 * sizeof(fp_digit));
   memcpy(at+32, B->dp, 32 * sizeof(fp_digit));
   COMBA_START;

   COMBA_CLEAR;
   /* 0 */
   MULADD(at[0], at[32]); 
   COMBA_STORE(C->dp[0]);
   /* 1 */
   COMBA_FORWARD;
   MULADD(at[0], at[33]);    MULADD(at[1], at[32]); 
   COMBA_STORE(C->dp[1]);
   /* 2 */
   COMBA_FORWARD;
   MULADD(at[0], at[34]);    MULADD(at[1], at[33]);    MULADD(at[2], at[32]); 
   COMBA_STORE(C->dp[2]);
   /* 3 */
   COMBA_FORWARD;
   MULADD(at[0], at[35]);    MULADD(at[1], at[34]);    MULADD(at[2], at[33]);    MULADD(at[3], at[32]); 
   COMBA_STORE(C->dp[3]);
   /* 4 */
   COMBA_FORWARD;
   MULADD(at[0], at[36]);    MULADD(at[1], at[35]);    MULADD(at[2], at[34]);    MULADD(at[3], at[33]);    MULADD(at[4], at[32]); 
   COMBA_STORE(C->dp[4]);
   /* 5 */
   COMBA_FORWARD;
   MULADD(at[0], at[37]);    MULADD(at[1], at[36]);    MULADD(at[2], at[35]);    MULADD(at[3], at[34]);    MULADD(at[4], at[33]);    MULADD(at[5], at[32]); 
   COMBA_STORE(C->dp[5]);
   /* 6 */
   COMBA_FORWARD;
   MULADD(at[0], at[38]);    MULADD(at[1], at[37]);    MULADD(at[2], at[36]);    MULADD(at[3], at[35]);    MULADD(at[4], at[34]);    MULADD(at[5], at[33]);    MULADD(at[6], at[32]); 
   COMBA_STORE(C->dp[6]);
   /* 7 */
   COMBA_FORWARD;
   MULADD(at[0], at[39]);    MULADD(at[1], at[38]);    MULADD(at[2], at[37]);    MULADD(at[3], at[36]);    MULADD(at[4], at[35]);    MULADD(at[5], at[34]);    MULADD(at[6], at[33]);    MULADD(at[7], at[32]); 
   COMBA_STORE(C->dp[7]);
   /* 8 */
   COMBA_FORWARD;
   MULADD(at[0], at[40]);    MULADD(at[1], at[39]);    MULADD(at[2], at[38]);    MULADD(at[3], at[37]);    MULADD(at[4], at[36]);    MULADD(at[5], at[35]);    MULADD(at[6], at[34]);    MULADD(at[7], at[33]);    MULADD(at[8], at[32]); 
   COMBA_STORE(C->dp[8]);
   /* 9 */
   COMBA_FORWARD;
   MULADD(at[0], at[41]);    MULADD(at[1], at[40]);    MULADD(at[2], at[39]);    MULADD(at[3], at[38]);    MULADD(at[4], at[37]);    MULADD(at[5], at[36]);    MULADD(at[6], at[35]);    MULADD(at[7], at[34]);    MULADD(at[8], at[33]);    MULADD(at[9], at[32]); 
   COMBA_STORE(C->dp[9]);
   /* 10 */
   COMBA_FORWARD;
   MULADD(at[0], at[42]);    MULADD(at[1], at[41]);    MULADD(at[2], at[40]);    MULADD(at[3], at[39]);    MULADD(at[4], at[38]);    MULADD(at[5], at[37]);    MULADD(at[6], at[36]);    MULADD(at[7], at[35]);    MULADD(at[8], at[34]);    MULADD(at[9], at[33]);    MULADD(at[10], at[32]); 
   COMBA_STORE(C->dp[10]);
   /* 11 */
   COMBA_FORWARD;
   MULADD(at[0], at[43]);    MULADD(at[1], at[42]);    MULADD(at[2], at[41]);    MULADD(at[3], at[40]);    MULADD(at[4], at[39]);    MULADD(at[5], at[38]);    MULADD(at[6], at[37]);    MULADD(at[7], at[36]);    MULADD(at[8], at[35]);    MULADD(at[9], at[34]);    MULADD(at[10], at[33]);    MULADD(at[11], at[32]); 
   COMBA_STORE(C->dp[11]);
   /* 12 */
   COMBA_FORWARD;
   MULADD(at[0], at[44]);    MULADD(at[1], at[43]);    MULADD(at[2], at[42]);    MULADD(at[3], at[41]);    MULADD(at[4], at[40]);    MULADD(at[5], at[39]);    MULADD(at[6], at[38]);    MULADD(at[7], at[37]);    MULADD(at[8], at[36]);    MULADD(at[9], at[35]);    MULADD(at[10], at[34]);    MULADD(at[11], at[33]);    MULADD(at[12], at[32]); 
   COMBA_STORE(C->dp[12]);
   /* 13 */
   COMBA_FORWARD;
   MULADD(at[0], at[45]);    MULADD(at[1], at[44]);    MULADD(at[2], at[43]);    MULADD(at[3], at[42]);    MULADD(at[4], at[41]);    MULADD(at[5], at[40]);    MULADD(at[6], at[39]);    MULADD(at[7], at[38]);    MULADD(at[8], at[37]);    MULADD(at[9], at[36]);    MULADD(at[10], at[35]);    MULADD(at[11], at[34]);    MULADD(at[12], at[33]);    MULADD(at[13], at[32]); 
   COMBA_STORE(C->dp[13]);
   /* 14 */
   COMBA_FORWARD;
   MULADD(at[0], at[46]);    MULADD(at[1], at[45]);    MULADD(at[2], at[44]);    MULADD(at[3], at[43]);    MULADD(at[4], at[42]);    MULADD(at[5], at[41]);    MULADD(at[6], at[40]);    MULADD(at[7], at[39]);    MULADD(at[8], at[38]);    MULADD(at[9], at[37]);    MULADD(at[10], at[36]);    MULADD(at[11], at[35]);    MULADD(at[12], at[34]);    MULADD(at[13], at[33]);    MULADD(at[14], at[32]); 
   COMBA_STORE(C->dp[14]);
   /* 15 */
   COMBA_FORWARD;
   MULADD(at[0], at[47]);    MULADD(at[1], at[46]);    MULADD(at[2], at[45]);    MULADD(at[3], at[44]);    MULADD(at[4], at[43]);    MULADD(at[5], at[42]);    MULADD(at[6], at[41]);    MULADD(at[7], at[40]);    MULADD(at[8], at[39]);    MULADD(at[9], at[38]);    MULADD(at[10], at[37]);    MULADD(at[11], at[36]);    MULADD(at[12], at[35]);    MULADD(at[13], at[34]);    MULADD(at[14], at[33]);    MULADD(at[15], at[32]); 
   COMBA_STORE(C->dp[15]);
   /* 16 */
   COMBA_FORWARD;
   MULADD(at[0], at[48]);    MULADD(at[1], at[47]);    MULADD(at[2], at[46]);    MULADD(at[3], at[45]);    MULADD(at[4], at[44]);    MULADD(at[5], at[43]);    MULADD(at[6], at[42]);    MULADD(at[7], at[41]);    MULADD(at[8], at[40]);    MULADD(at[9], at[39]);    MULADD(at[10], at[38]);    MULADD(at[11], at[37]);    MULADD(at[12], at[36]);    MULADD(at[13], at[35]);    MULADD(at[14], at[34]);    MULADD(at[15], at[33]);    MULADD(at[16], at[32]); 
   COMBA_STORE(C->dp[16]);
   /* 17 */
   COMBA_FORWARD;
   MULADD(at[0], at[49]);    MULADD(at[1], at[48]);    MULADD(at[2], at[47]);    MULADD(at[3], at[46]);    MULADD(at[4], at[45]);    MULADD(at[5], at[44]);    MULADD(at[6], at[43]);    MULADD(at[7], at[42]);    MULADD(at[8], at[41]);    MULADD(at[9], at[40]);    MULADD(at[10], at[39]);    MULADD(at[11], at[38]);    MULADD(at[12], at[37]);    MULADD(at[13], at[36]);    MULADD(at[14], at[35]);    MULADD(at[15], at[34]);    MULADD(at[16], at[33]);    MULADD(at[17], at[32]); 
   COMBA_STORE(C->dp[17]);
   /* 18 */
   COMBA_FORWARD;
   MULADD(at[0], at[50]);    MULADD(at[1], at[49]);    MULADD(at[2], at[48]);    MULADD(at[3], at[47]);    MULADD(at[4], at[46]);    MULADD(at[5], at[45]);    MULADD(at[6], at[44]);    MULADD(at[7], at[43]);    MULADD(at[8], at[42]);    MULADD(at[9], at[41]);    MULADD(at[10], at[40]);    MULADD(at[11], at[39]);    MULADD(at[12], at[38]);    MULADD(at[13], at[37]);    MULADD(at[14], at[36]);    MULADD(at[15], at[35]);    MULADD(at[16], at[34]);    MULADD(at[17], at[33]);    MULADD(at[18], at[32]); 
   COMBA_STORE(C->dp[18]);
   /* 19 */
   COMBA_FORWARD;
   MULADD(at[0], at[51]);    MULADD(at[1], at[50]);    MULADD(at[2], at[49]);    MULADD(at[3], at[48]);    MULADD(at[4], at[47]);    MULADD(at[5], at[46]);    MULADD(at[6], at[45]);    MULADD(at[7], at[44]);    MULADD(at[8], at[43]);    MULADD(at[9], at[42]);    MULADD(at[10], at[41]);    MULADD(at[11], at[40]);    MULADD(at[12], at[39]);    MULADD(at[13], at[38]);    MULADD(at[14], at[37]);    MULADD(at[15], at[36]);    MULADD(at[16], at[35]);    MULADD(at[17], at[34]);    MULADD(at[18], at[33]);    MULADD(at[19], at[32]); 
   COMBA_STORE(C->dp[19]);
   /* 20 */
   COMBA_FORWARD;
   MULADD(at[0], at[52]);    MULADD(at[1], at[51]);    MULADD(at[2], at[50]);    MULADD(at[3], at[49]);    MULADD(at[4], at[48]);    MULADD(at[5], at[47]);    MULADD(at[6], at[46]);    MULADD(at[7], at[45]);    MULADD(at[8], at[44]);    MULADD(at[9], at[43]);    MULADD(at[10], at[42]);    MULADD(at[11], at[41]);    MULADD(at[12], at[40]);    MULADD(at[13], at[39]);    MULADD(at[14], at[38]);    MULADD(at[15], at[37]);    MULADD(at[16], at[36]);    MULADD(at[17], at[35]);    MULADD(at[18], at[34]);    MULADD(at[19], at[33]);    MULADD(at[20], at[32]); 
   COMBA_STORE(C->dp[20]);
   /* 21 */
   COMBA_FORWARD;
   MULADD(at[0], at[53]);    MULADD(at[1], at[52]);    MULADD(at[2], at[51]);    MULADD(at[3], at[50]);    MULADD(at[4], at[49]);    MULADD(at[5], at[48]);    MULADD(at[6], at[47]);    MULADD(at[7], at[46]);    MULADD(at[8], at[45]);    MULADD(at[9], at[44]);    MULADD(at[10], at[43]);    MULADD(at[11], at[42]);    MULADD(at[12], at[41]);    MULADD(at[13], at[40]);    MULADD(at[14], at[39]);    MULADD(at[15], at[38]);    MULADD(at[16], at[37]);    MULADD(at[17], at[36]);    MULADD(at[18], at[35]);    MULADD(at[19], at[34]);    MULADD(at[20], at[33]);    MULADD(at[21], at[32]); 
   COMBA_STORE(C->dp[21]);
   /* 22 */
   COMBA_FORWARD;
   MULADD(at[0], at[54]);    MULADD(at[1], at[53]);    MULADD(at[2], at[52]);    MULADD(at[3], at[51]);    MULADD(at[4], at[50]);    MULADD(at[5], at[49]);    MULADD(at[6], at[48]);    MULADD(at[7], at[47]);    MULADD(at[8], at[46]);    MULADD(at[9], at[45]);    MULADD(at[10], at[44]);    MULADD(at[11], at[43]);    MULADD(at[12], at[42]);    MULADD(at[13], at[41]);    MULADD(at[14], at[40]);    MULADD(at[15], at[39]);    MULADD(at[16], at[38]);    MULADD(at[17], at[37]);    MULADD(at[18], at[36]);    MULADD(at[19], at[35]);    MULADD(at[20], at[34]);    MULADD(at[21], at[33]);    MULADD(at[22], at[32]); 
   COMBA_STORE(C->dp[22]);
   /* 23 */
   COMBA_FORWARD;
   MULADD(at[0], at[55]);    MULADD(at[1], at[54]);    MULADD(at[2], at[53]);    MULADD(at[3], at[52]);    MULADD(at[4], at[51]);    MULADD(at[5], at[50]);    MULADD(at[6], at[49]);    MULADD(at[7], at[48]);    MULADD(at[8], at[47]);    MULADD(at[9], at[46]);    MULADD(at[10], at[45]);    MULADD(at[11], at[44]);    MULADD(at[12], at[43]);    MULADD(at[13], at[42]);    MULADD(at[14], at[41]);    MULADD(at[15], at[40]);    MULADD(at[16], at[39]);    MULADD(at[17], at[38]);    MULADD(at[18], at[37]);    MULADD(at[19], at[36]);    MULADD(at[20], at[35]);    MULADD(at[21], at[34]);    MULADD(at[22], at[33]);    MULADD(at[23], at[32]); 
   COMBA_STORE(C->dp[23]);
   /* 24 */
   COMBA_FORWARD;
   MULADD(at[0], at[56]);    MULADD(at[1], at[55]);    MULADD(at[2], at[54]);    MULADD(at[3], at[53]);    MULADD(at[4], at[52]);    MULADD(at[5], at[51]);    MULADD(at[6], at[50]);    MULADD(at[7], at[49]);    MULADD(at[8], at[48]);    MULADD(at[9], at[47]);    MULADD(at[10], at[46]);    MULADD(at[11], at[45]);    MULADD(at[12], at[44]);    MULADD(at[13], at[43]);    MULADD(at[14], at[42]);    MULADD(at[15], at[41]);    MULADD(at[16], at[40]);    MULADD(at[17], at[39]);    MULADD(at[18], at[38]);    MULADD(at[19], at[37]);    MULADD(at[20], at[36]);    MULADD(at[21], at[35]);    MULADD(at[22], at[34]);    MULADD(at[23], at[33]);    MULADD(at[24], at[32]); 
   COMBA_STORE(C->dp[24]);
   /* 25 */
   COMBA_FORWARD;
   MULADD(at[0], at[57]);    MULADD(at[1], at[56]);    MULADD(at[2], at[55]);    MULADD(at[3], at[54]);    MULADD(at[4], at[53]);    MULADD(at[5], at[52]);    MULADD(at[6], at[51]);    MULADD(at[7], at[50]);    MULADD(at[8], at[49]);    MULADD(at[9], at[48]);    MULADD(at[10], at[47]);    MULADD(at[11], at[46]);    MULADD(at[12], at[45]);    MULADD(at[13], at[44]);    MULADD(at[14], at[43]);    MULADD(at[15], at[42]);    MULADD(at[16], at[41]);    MULADD(at[17], at[40]);    MULADD(at[18], at[39]);    MULADD(at[19], at[38]);    MULADD(at[20], at[37]);    MULADD(at[21], at[36]);    MULADD(at[22], at[35]);    MULADD(at[23], at[34]);    MULADD(at[24], at[33]);    MULADD(at[25], at[32]); 
   COMBA_STORE(C->dp[25]);
   /* 26 */
   COMBA_FORWARD;
   MULADD(at[0], at[58]);    MULADD(at[1], at[57]);    MULADD(at[2], at[56]);    MULADD(at[3], at[55]);    MULADD(at[4], at[54]);    MULADD(at[5], at[53]);    MULADD(at[6], at[52]);    MULADD(at[7], at[51]);    MULADD(at[8], at[50]);    MULADD(at[9], at[49]);    MULADD(at[10], at[48]);    MULADD(at[11], at[47]);    MULADD(at[12], at[46]);    MULADD(at[13], at[45]);    MULADD(at[14], at[44]);    MULADD(at[15], at[43]);    MULADD(at[16], at[42]);    MULADD(at[17], at[41]);    MULADD(at[18], at[40]);    MULADD(at[19], at[39]);    MULADD(at[20], at[38]);    MULADD(at[21], at[37]);    MULADD(at[22], at[36]);    MULADD(at[23], at[35]);    MULADD(at[24], at[34]);    MULADD(at[25], at[33]);    MULADD(at[26], at[32]); 
   COMBA_STORE(C->dp[26]);
   /* 27 */
   COMBA_FORWARD;
   MULADD(at[0], at[59]);    MULADD(at[1], at[58]);    MULADD(at[2], at[57]);    MULADD(at[3], at[56]);    MULADD(at[4], at[55]);    MULADD(at[5], at[54]);    MULADD(at[6], at[53]);    MULADD(at[7], at[52]);    MULADD(at[8], at[51]);    MULADD(at[9], at[50]);    MULADD(at[10], at[49]);    MULADD(at[11], at[48]);    MULADD(at[12], at[47]);    MULADD(at[13], at[46]);    MULADD(at[14], at[45]);    MULADD(at[15], at[44]);    MULADD(at[16], at[43]);    MULADD(at[17], at[42]);    MULADD(at[18], at[41]);    MULADD(at[19], at[40]);    MULADD(at[20], at[39]);    MULADD(at[21], at[38]);    MULADD(at[22], at[37]);    MULADD(at[23], at[36]);    MULADD(at[24], at[35]);    MULADD(at[25], at[34]);    MULADD(at[26], at[33]);    MULADD(at[27], at[32]); 
   COMBA_STORE(C->dp[27]);
   /* 28 */
   COMBA_FORWARD;
   MULADD(at[0], at[60]);    MULADD(at[1], at[59]);    MULADD(at[2], at[58]);    MULADD(at[3], at[57]);    MULADD(at[4], at[56]);    MULADD(at[5], at[55]);    MULADD(at[6], at[54]);    MULADD(at[7], at[53]);    MULADD(at[8], at[52]);    MULADD(at[9], at[51]);    MULADD(at[10], at[50]);    MULADD(at[11], at[49]);    MULADD(at[12], at[48]);    MULADD(at[13], at[47]);    MULADD(at[14], at[46]);    MULADD(at[15], at[45]);    MULADD(at[16], at[44]);    MULADD(at[17], at[43]);    MULADD(at[18], at[42]);    MULADD(at[19], at[41]);    MULADD(at[20], at[40]);    MULADD(at[21], at[39]);    MULADD(at[22], at[38]);    MULADD(at[23], at[37]);    MULADD(at[24], at[36]);    MULADD(at[25], at[35]);    MULADD(at[26], at[34]);    MULADD(at[27], at[33]);    MULADD(at[28], at[32]); 
   COMBA_STORE(C->dp[28]);
   /* 29 */
   COMBA_FORWARD;
   MULADD(at[0], at[61]);    MULADD(at[1], at[60]);    MULADD(at[2], at[59]);    MULADD(at[3], at[58]);    MULADD(at[4], at[57]);    MULADD(at[5], at[56]);    MULADD(at[6], at[55]);    MULADD(at[7], at[54]);    MULADD(at[8], at[53]);    MULADD(at[9], at[52]);    MULADD(at[10], at[51]);    MULADD(at[11], at[50]);    MULADD(at[12], at[49]);    MULADD(at[13], at[48]);    MULADD(at[14], at[47]);    MULADD(at[15], at[46]);    MULADD(at[16], at[45]);    MULADD(at[17], at[44]);    MULADD(at[18], at[43]);    MULADD(at[19], at[42]);    MULADD(at[20], at[41]);    MULADD(at[21], at[40]);    MULADD(at[22], at[39]);    MULADD(at[23], at[38]);    MULADD(at[24], at[37]);    MULADD(at[25], at[36]);    MULADD(at[26], at[35]);    MULADD(at[27], at[34]);    MULADD(at[28], at[33]);    MULADD(at[29], at[32]); 
   COMBA_STORE(C->dp[29]);
   /* 30 */
   COMBA_FORWARD;
   MULADD(at[0], at[62]);    MULADD(at[1], at[61]);    MULADD(at[2], at[60]);    MULADD(at[3], at[59]);    MULADD(at[4], at[58]);    MULADD(at[5], at[57]);    MULADD(at[6], at[56]);    MULADD(at[7], at[55]);    MULADD(at[8], at[54]);    MULADD(at[9], at[53]);    MULADD(at[10], at[52]);    MULADD(at[11], at[51]);    MULADD(at[12], at[50]);    MULADD(at[13], at[49]);    MULADD(at[14], at[48]);    MULADD(at[15], at[47]);    MULADD(at[16], at[46]);    MULADD(at[17], at[45]);    MULADD(at[18], at[44]);    MULADD(at[19], at[43]);    MULADD(at[20], at[42]);    MULADD(at[21], at[41]);    MULADD(at[22], at[40]);    MULADD(at[23], at[39]);    MULADD(at[24], at[38]);    MULADD(at[25], at[37]);    MULADD(at[26], at[36]);    MULADD(at[27], at[35]);    MULADD(at[28], at[34]);    MULADD(at[29], at[33]);    MULADD(at[30], at[32]); 
   COMBA_STORE(C->dp[30]);
   /* 31 */
   COMBA_FORWARD;
   MULADD(at[0], at[63]);    MULADD(at[1], at[62]);    MULADD(at[2], at[61]);    MULADD(at[3], at[60]);    MULADD(at[4], at[59]);    MULADD(at[5], at[58]);    MULADD(at[6], at[57]);    MULADD(at[7], at[56]);    MULADD(at[8], at[55]);    MULADD(at[9], at[54]);    MULADD(at[10], at[53]);    MULADD(at[11], at[52]);    MULADD(at[12], at[51]);    MULADD(at[13], at[50]);    MULADD(at[14], at[49]);    MULADD(at[15], at[48]);    MULADD(at[16], at[47]);    MULADD(at[17], at[46]);    MULADD(at[18], at[45]);    MULADD(at[19], at[44]);    MULADD(at[20], at[43]);    MULADD(at[21], at[42]);    MULADD(at[22], at[41]);    MULADD(at[23], at[40]);    MULADD(at[24], at[39]);    MULADD(at[25], at[38]);    MULADD(at[26], at[37]);    MULADD(at[27], at[36]);    MULADD(at[28], at[35]);    MULADD(at[29], at[34]);    MULADD(at[30], at[33]);    MULADD(at[31], at[32]); 
   COMBA_STORE(C->dp[31]);
   /* 32 */
   COMBA_FORWARD;
   MULADD(at[1], at[63]);    MULADD(at[2], at[62]);    MULADD(at[3], at[61]);    MULADD(at[4], at[60]);    MULADD(at[5], at[59]);    MULADD(at[6], at[58]);    MULADD(at[7], at[57]);    MULADD(at[8], at[56]);    MULADD(at[9], at[55]);    MULADD(at[10], at[54]);    MULADD(at[11], at[53]);    MULADD(at[12], at[52]);    MULADD(at[13], at[51]);    MULADD(at[14], at[50]);    MULADD(at[15], at[49]);    MULADD(at[16], at[48]);    MULADD(at[17], at[47]);    MULADD(at[18], at[46]);    MULADD(at[19], at[45]);    MULADD(at[20], at[44]);    MULADD(at[21], at[43]);    MULADD(at[22], at[42]);    MULADD(at[23], at[41]);    MULADD(at[24], at[40]);    MULADD(at[25], at[39]);    MULADD(at[26], at[38]);    MULADD(at[27], at[37]);    MULADD(at[28], at[36]);    MULADD(at[29], at[35]);    MULADD(at[30], at[34]);    MULADD(at[31], at[33]); 
   COMBA_STORE(C->dp[32]);
   /* 33 */
   COMBA_FORWARD;
   MULADD(at[2], at[63]);    MULADD(at[3], at[62]);    MULADD(at[4], at[61]);    MULADD(at[5], at[60]);    MULADD(at[6], at[59]);    MULADD(at[7], at[58]);    MULADD(at[8], at[57]);    MULADD(at[9], at[56]);    MULADD(at[10], at[55]);    MULADD(at[11], at[54]);    MULADD(at[12], at[53]);    MULADD(at[13], at[52]);    MULADD(at[14], at[51]);    MULADD(at[15], at[50]);    MULADD(at[16], at[49]);    MULADD(at[17], at[48]);    MULADD(at[18], at[47]);    MULADD(at[19], at[46]);    MULADD(at[20], at[45]);    MULADD(at[21], at[44]);    MULADD(at[22], at[43]);    MULADD(at[23], at[42]);    MULADD(at[24], at[41]);    MULADD(at[25], at[40]);    MULADD(at[26], at[39]);    MULADD(at[27], at[38]);    MULADD(at[28], at[37]);    MULADD(at[29], at[36]);    MULADD(at[30], at[35]);    MULADD(at[31], at[34]); 
   COMBA_STORE(C->dp[33]);
   /* 34 */
   COMBA_FORWARD;
   MULADD(at[3], at[63]);    MULADD(at[4], at[62]);    MULADD(at[5], at[61]);    MULADD(at[6], at[60]);    MULADD(at[7], at[59]);    MULADD(at[8], at[58]);    MULADD(at[9], at[57]);    MULADD(at[10], at[56]);    MULADD(at[11], at[55]);    MULADD(at[12], at[54]);    MULADD(at[13], at[53]);    MULADD(at[14], at[52]);    MULADD(at[15], at[51]);    MULADD(at[16], at[50]);    MULADD(at[17], at[49]);    MULADD(at[18], at[48]);    MULADD(at[19], at[47]);    MULADD(at[20], at[46]);    MULADD(at[21], at[45]);    MULADD(at[22], at[44]);    MULADD(at[23], at[43]);    MULADD(at[24], at[42]);    MULADD(at[25], at[41]);    MULADD(at[26], at[40]);    MULADD(at[27], at[39]);    MULADD(at[28], at[38]);    MULADD(at[29], at[37]);    MULADD(at[30], at[36]);    MULADD(at[31], at[35]); 
   COMBA_STORE(C->dp[34]);
   /* 35 */
   COMBA_FORWARD;
   MULADD(at[4], at[63]);    MULADD(at[5], at[62]);    MULADD(at[6], at[61]);    MULADD(at[7], at[60]);    MULADD(at[8], at[59]);    MULADD(at[9], at[58]);    MULADD(at[10], at[57]);    MULADD(at[11], at[56]);    MULADD(at[12], at[55]);    MULADD(at[13], at[54]);    MULADD(at[14], at[53]);    MULADD(at[15], at[52]);    MULADD(at[16], at[51]);    MULADD(at[17], at[50]);    MULADD(at[18], at[49]);    MULADD(at[19], at[48]);    MULADD(at[20], at[47]);    MULADD(at[21], at[46]);    MULADD(at[22], at[45]);    MULADD(at[23], at[44]);    MULADD(at[24], at[43]);    MULADD(at[25], at[42]);    MULADD(at[26], at[41]);    MULADD(at[27], at[40]);    MULADD(at[28], at[39]);    MULADD(at[29], at[38]);    MULADD(at[30], at[37]);    MULADD(at[31], at[36]); 
   COMBA_STORE(C->dp[35]);
   /* 36 */
   COMBA_FORWARD;
   MULADD(at[5], at[63]);    MULADD(at[6], at[62]);    MULADD(at[7], at[61]);    MULADD(at[8], at[60]);    MULADD(at[9], at[59]);    MULADD(at[10], at[58]);    MULADD(at[11], at[57]);    MULADD(at[12], at[56]);    MULADD(at[13], at[55]);    MULADD(at[14], at[54]);    MULADD(at[15], at[53]);    MULADD(at[16], at[52]);    MULADD(at[17], at[51]);    MULADD(at[18], at[50]);    MULADD(at[19], at[49]);    MULADD(at[20], at[48]);    MULADD(at[21], at[47]);    MULADD(at[22], at[46]);    MULADD(at[23], at[45]);    MULADD(at[24], at[44]);    MULADD(at[25], at[43]);    MULADD(at[26], at[42]);    MULADD(at[27], at[41]);    MULADD(at[28], at[40]);    MULADD(at[29], at[39]);    MULADD(at[30], at[38]);    MULADD(at[31], at[37]); 
   COMBA_STORE(C->dp[36]);
   /* 37 */
   COMBA_FORWARD;
   MULADD(at[6], at[63]);    MULADD(at[7], at[62]);    MULADD(at[8], at[61]);    MULADD(at[9], at[60]);    MULADD(at[10], at[59]);    MULADD(at[11], at[58]);    MULADD(at[12], at[57]);    MULADD(at[13], at[56]);    MULADD(at[14], at[55]);    MULADD(at[15], at[54]);    MULADD(at[16], at[53]);    MULADD(at[17], at[52]);    MULADD(at[18], at[51]);    MULADD(at[19], at[50]);    MULADD(at[20], at[49]);    MULADD(at[21], at[48]);    MULADD(at[22], at[47]);    MULADD(at[23], at[46]);    MULADD(at[24], at[45]);    MULADD(at[25], at[44]);    MULADD(at[26], at[43]);    MULADD(at[27], at[42]);    MULADD(at[28], at[41]);    MULADD(at[29], at[40]);    MULADD(at[30], at[39]);    MULADD(at[31], at[38]); 
   COMBA_STORE(C->dp[37]);
   /* 38 */
   COMBA_FORWARD;
   MULADD(at[7], at[63]);    MULADD(at[8], at[62]);    MULADD(at[9], at[61]);    MULADD(at[10], at[60]);    MULADD(at[11], at[59]);    MULADD(at[12], at[58]);    MULADD(at[13], at[57]);    MULADD(at[14], at[56]);    MULADD(at[15], at[55]);    MULADD(at[16], at[54]);    MULADD(at[17], at[53]);    MULADD(at[18], at[52]);    MULADD(at[19], at[51]);    MULADD(at[20], at[50]);    MULADD(at[21], at[49]);    MULADD(at[22], at[48]);    MULADD(at[23], at[47]);    MULADD(at[24], at[46]);    MULADD(at[25], at[45]);    MULADD(at[26], at[44]);    MULADD(at[27], at[43]);    MULADD(at[28], at[42]);    MULADD(at[29], at[41]);    MULADD(at[30], at[40]);    MULADD(at[31], at[39]); 
   COMBA_STORE(C->dp[38]);

   /* early out at 40 digits, 40*32==1280, or two 640 bit operands */
   if (out_size <= 40) { COMBA_STORE2(C->dp[39]); C->used = 40; C->sign = A->sign ^ B->sign; fp_clamp(C); COMBA_FINI; return; }

   /* 39 */
   COMBA_FORWARD;
   MULADD(at[8], at[63]);    MULADD(at[9], at[62]);    MULADD(at[10], at[61]);    MULADD(at[11], at[60]);    MULADD(at[12], at[59]);    MULADD(at[13], at[58]);    MULADD(at[14], at[57]);    MULADD(at[15], at[56]);    MULADD(at[16], at[55]);    MULADD(at[17], at[54]);    MULADD(at[18], at[53]);    MULADD(at[19], at[52]);    MULADD(at[20], at[51]);    MULADD(at[21], at[50]);    MULADD(at[22], at[49]);    MULADD(at[23], at[48]);    MULADD(at[24], at[47]);    MULADD(at[25], at[46]);    MULADD(at[26], at[45]);    MULADD(at[27], at[44]);    MULADD(at[28], at[43]);    MULADD(at[29], at[42]);    MULADD(at[30], at[41]);    MULADD(at[31], at[40]); 
   COMBA_STORE(C->dp[39]);
   /* 40 */
   COMBA_FORWARD;
   MULADD(at[9], at[63]);    MULADD(at[10], at[62]);    MULADD(at[11], at[61]);    MULADD(at[12], at[60]);    MULADD(at[13], at[59]);    MULADD(at[14], at[58]);    MULADD(at[15], at[57]);    MULADD(at[16], at[56]);    MULADD(at[17], at[55]);    MULADD(at[18], at[54]);    MULADD(at[19], at[53]);    MULADD(at[20], at[52]);    MULADD(at[21], at[51]);    MULADD(at[22], at[50]);    MULADD(at[23], at[49]);    MULADD(at[24], at[48]);    MULADD(at[25], at[47]);    MULADD(at[26], at[46]);    MULADD(at[27], at[45]);    MULADD(at[28], at[44]);    MULADD(at[29], at[43]);    MULADD(at[30], at[42]);    MULADD(at[31], at[41]); 
   COMBA_STORE(C->dp[40]);
   /* 41 */
   COMBA_FORWARD;
   MULADD(at[10], at[63]);    MULADD(at[11], at[62]);    MULADD(at[12], at[61]);    MULADD(at[13], at[60]);    MULADD(at[14], at[59]);    MULADD(at[15], at[58]);    MULADD(at[16], at[57]);    MULADD(at[17], at[56]);    MULADD(at[18], at[55]);    MULADD(at[19], at[54]);    MULADD(at[20], at[53]);    MULADD(at[21], at[52]);    MULADD(at[22], at[51]);    MULADD(at[23], at[50]);    MULADD(at[24], at[49]);    MULADD(at[25], at[48]);    MULADD(at[26], at[47]);    MULADD(at[27], at[46]);    MULADD(at[28], at[45]);    MULADD(at[29], at[44]);    MULADD(at[30], at[43]);    MULADD(at[31], at[42]); 
   COMBA_STORE(C->dp[41]);
   /* 42 */
   COMBA_FORWARD;
   MULADD(at[11], at[63]);    MULADD(at[12], at[62]);    MULADD(at[13], at[61]);    MULADD(at[14], at[60]);    MULADD(at[15], at[59]);    MULADD(at[16], at[58]);    MULADD(at[17], at[57]);    MULADD(at[18], at[56]);    MULADD(at[19], at[55]);    MULADD(at[20], at[54]);    MULADD(at[21], at[53]);    MULADD(at[22], at[52]);    MULADD(at[23], at[51]);    MULADD(at[24], at[50]);    MULADD(at[25], at[49]);    MULADD(at[26], at[48]);    MULADD(at[27], at[47]);    MULADD(at[28], at[46]);    MULADD(at[29], at[45]);    MULADD(at[30], at[44]);    MULADD(at[31], at[43]); 
   COMBA_STORE(C->dp[42]);
   /* 43 */
   COMBA_FORWARD;
   MULADD(at[12], at[63]);    MULADD(at[13], at[62]);    MULADD(at[14], at[61]);    MULADD(at[15], at[60]);    MULADD(at[16], at[59]);    MULADD(at[17], at[58]);    MULADD(at[18], at[57]);    MULADD(at[19], at[56]);    MULADD(at[20], at[55]);    MULADD(at[21], at[54]);    MULADD(at[22], at[53]);    MULADD(at[23], at[52]);    MULADD(at[24], at[51]);    MULADD(at[25], at[50]);    MULADD(at[26], at[49]);    MULADD(at[27], at[48]);    MULADD(at[28], at[47]);    MULADD(at[29], at[46]);    MULADD(at[30], at[45]);    MULADD(at[31], at[44]); 
   COMBA_STORE(C->dp[43]);
   /* 44 */
   COMBA_FORWARD;
   MULADD(at[13], at[63]);    MULADD(at[14], at[62]);    MULADD(at[15], at[61]);    MULADD(at[16], at[60]);    MULADD(at[17], at[59]);    MULADD(at[18], at[58]);    MULADD(at[19], at[57]);    MULADD(at[20], at[56]);    MULADD(at[21], at[55]);    MULADD(at[22], at[54]);    MULADD(at[23], at[53]);    MULADD(at[24], at[52]);    MULADD(at[25], at[51]);    MULADD(at[26], at[50]);    MULADD(at[27], at[49]);    MULADD(at[28], at[48]);    MULADD(at[29], at[47]);    MULADD(at[30], at[46]);    MULADD(at[31], at[45]); 
   COMBA_STORE(C->dp[44]);
   /* 45 */
   COMBA_FORWARD;
   MULADD(at[14], at[63]);    MULADD(at[15], at[62]);    MULADD(at[16], at[61]);    MULADD(at[17], at[60]);    MULADD(at[18], at[59]);    MULADD(at[19], at[58]);    MULADD(at[20], at[57]);    MULADD(at[21], at[56]);    MULADD(at[22], at[55]);    MULADD(at[23], at[54]);    MULADD(at[24], at[53]);    MULADD(at[25], at[52]);    MULADD(at[26], at[51]);    MULADD(at[27], at[50]);    MULADD(at[28], at[49]);    MULADD(at[29], at[48]);    MULADD(at[30], at[47]);    MULADD(at[31], at[46]); 
   COMBA_STORE(C->dp[45]);
   /* 46 */
   COMBA_FORWARD;
   MULADD(at[15], at[63]);    MULADD(at[16], at[62]);    MULADD(at[17], at[61]);    MULADD(at[18], at[60]);    MULADD(at[19], at[59]);    MULADD(at[20], at[58]);    MULADD(at[21], at[57]);    MULADD(at[22], at[56]);    MULADD(at[23], at[55]);    MULADD(at[24], at[54]);    MULADD(at[25], at[53]);    MULADD(at[26], at[52]);    MULADD(at[27], at[51]);    MULADD(at[28], at[50]);    MULADD(at[29], at[49]);    MULADD(at[30], at[48]);    MULADD(at[31], at[47]); 
   COMBA_STORE(C->dp[46]);

   /* early out at 48 digits, 48*32==1536, or two 768 bit operands */
   if (out_size <= 48) { COMBA_STORE2(C->dp[47]); C->used = 48; C->sign = A->sign ^ B->sign; fp_clamp(C); COMBA_FINI; return; }

   /* 47 */
   COMBA_FORWARD;
   MULADD(at[16], at[63]);    MULADD(at[17], at[62]);    MULADD(at[18], at[61]);    MULADD(at[19], at[60]);    MULADD(at[20], at[59]);    MULADD(at[21], at[58]);    MULADD(at[22], at[57]);    MULADD(at[23], at[56]);    MULADD(at[24], at[55]);    MULADD(at[25], at[54]);    MULADD(at[26], at[53]);    MULADD(at[27], at[52]);    MULADD(at[28], at[51]);    MULADD(at[29], at[50]);    MULADD(at[30], at[49]);    MULADD(at[31], at[48]); 
   COMBA_STORE(C->dp[47]);
   /* 48 */
   COMBA_FORWARD;
   MULADD(at[17], at[63]);    MULADD(at[18], at[62]);    MULADD(at[19], at[61]);    MULADD(at[20], at[60]);    MULADD(at[21], at[59]);    MULADD(at[22], at[58]);    MULADD(at[23], at[57]);    MULADD(at[24], at[56]);    MULADD(at[25], at[55]);    MULADD(at[26], at[54]);    MULADD(at[27], at[53]);    MULADD(at[28], at[52]);    MULADD(at[29], at[51]);    MULADD(at[30], at[50]);    MULADD(at[31], at[49]); 
   COMBA_STORE(C->dp[48]);
   /* 49 */
   COMBA_FORWARD;
   MULADD(at[18], at[63]);    MULADD(at[19], at[62]);    MULADD(at[20], at[61]);    MULADD(at[21], at[60]);    MULADD(at[22], at[59]);    MULADD(at[23], at[58]);    MULADD(at[24], at[57]);    MULADD(at[25], at[56]);    MULADD(at[26], at[55]);    MULADD(at[27], at[54]);    MULADD(at[28], at[53]);    MULADD(at[29], at[52]);    MULADD(at[30], at[51]);    MULADD(at[31], at[50]); 
   COMBA_STORE(C->dp[49]);
   /* 50 */
   COMBA_FORWARD;
   MULADD(at[19], at[63]);    MULADD(at[20], at[62]);    MULADD(at[21], at[61]);    MULADD(at[22], at[60]);    MULADD(at[23], at[59]);    MULADD(at[24], at[58]);    MULADD(at[25], at[57]);    MULADD(at[26], at[56]);    MULADD(at[27], at[55]);    MULADD(at[28], at[54]);    MULADD(at[29], at[53]);    MULADD(at[30], at[52]);    MULADD(at[31], at[51]); 
   COMBA_STORE(C->dp[50]);
   /* 51 */
   COMBA_FORWARD;
   MULADD(at[20], at[63]);    MULADD(at[21], at[62]);    MULADD(at[22], at[61]);    MULADD(at[23], at[60]);    MULADD(at[24], at[59]);    MULADD(at[25], at[58]);    MULADD(at[26], at[57]);    MULADD(at[27], at[56]);    MULADD(at[28], at[55]);    MULADD(at[29], at[54]);    MULADD(at[30], at[53]);    MULADD(at[31], at[52]); 
   COMBA_STORE(C->dp[51]);
   /* 52 */
   COMBA_FORWARD;
   MULADD(at[21], at[63]);    MULADD(at[22], at[62]);    MULADD(at[23], at[61]);    MULADD(at[24], at[60]);    MULADD(at[25], at[59]);    MULADD(at[26], at[58]);    MULADD(at[27], at[57]);    MULADD(at[28], at[56]);    MULADD(at[29], at[55]);    MULADD(at[30], at[54]);    MULADD(at[31], at[53]); 
   COMBA_STORE(C->dp[52]);
   /* 53 */
   COMBA_FORWARD;
   MULADD(at[22], at[63]);    MULADD(at[23], at[62]);    MULADD(at[24], at[61]);    MULADD(at[25], at[60]);    MULADD(at[26], at[59]);    MULADD(at[27], at[58]);    MULADD(at[28], at[57]);    MULADD(at[29], at[56]);    MULADD(at[30], at[55]);    MULADD(at[31], at[54]); 
   COMBA_STORE(C->dp[53]);
   /* 54 */
   COMBA_FORWARD;
   MULADD(at[23], at[63]);    MULADD(at[24], at[62]);    MULADD(at[25], at[61]);    MULADD(at[26], at[60]);    MULADD(at[27], at[59]);    MULADD(at[28], at[58]);    MULADD(at[29], at[57]);    MULADD(at[30], at[56]);    MULADD(at[31], at[55]); 
   COMBA_STORE(C->dp[54]);

   /* early out at 56 digits, 56*32==1792, or two 896 bit operands */
   if (out_size <= 56) { COMBA_STORE2(C->dp[55]); C->used = 56; C->sign = A->sign ^ B->sign; fp_clamp(C); COMBA_FINI; return; }

   /* 55 */
   COMBA_FORWARD;
   MULADD(at[24], at[63]);    MULADD(at[25], at[62]);    MULADD(at[26], at[61]);    MULADD(at[27], at[60]);    MULADD(at[28], at[59]);    MULADD(at[29], at[58]);    MULADD(at[30], at[57]);    MULADD(at[31], at[56]); 
   COMBA_STORE(C->dp[55]);
   /* 56 */
   COMBA_FORWARD;
   MULADD(at[25], at[63]);    MULADD(at[26], at[62]);    MULADD(at[27], at[61]);    MULADD(at[28], at[60]);    MULADD(at[29], at[59]);    MULADD(at[30], at[58]);    MULADD(at[31], at[57]); 
   COMBA_STORE(C->dp[56]);
   /* 57 */
   COMBA_FORWARD;
   MULADD(at[26], at[63]);    MULADD(at[27], at[62]);    MULADD(at[28], at[61]);    MULADD(at[29], at[60]);    MULADD(at[30], at[59]);    MULADD(at[31], at[58]); 
   COMBA_STORE(C->dp[57]);
   /* 58 */
   COMBA_FORWARD;
   MULADD(at[27], at[63]);    MULADD(at[28], at[62]);    MULADD(at[29], at[61]);    MULADD(at[30], at[60]);    MULADD(at[31], at[59]); 
   COMBA_STORE(C->dp[58]);
   /* 59 */
   COMBA_FORWARD;
   MULADD(at[28], at[63]);    MULADD(at[29], at[62]);    MULADD(at[30], at[61]);    MULADD(at[31], at[60]); 
   COMBA_STORE(C->dp[59]);
   /* 60 */
   COMBA_FORWARD;
   MULADD(at[29], at[63]);    MULADD(at[30], at[62]);    MULADD(at[31], at[61]); 
   COMBA_STORE(C->dp[60]);
   /* 61 */
   COMBA_FORWARD;
   MULADD(at[30], at[63]);    MULADD(at[31], at[62]); 
   COMBA_STORE(C->dp[61]);
   /* 62 */
   COMBA_FORWARD;
   MULADD(at[31], at[63]); 
   COMBA_STORE(C->dp[62]);
   COMBA_STORE2(C->dp[63]);
   C->used = 64;
   C->sign = A->sign ^ B->sign;
   fp_clamp(C);
   COMBA_FINI;
}
#endif

#ifdef TFM_MUL64
void fp_mul_comba64(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[128];

   memcpy(at, A->dp, 64 * sizeof(fp_digit));
   memcpy(at+64, B->dp, 64 * sizeof(fp_digit));
   COMBA_START;

   COMBA_CLEAR;
   /* 0 */
   MULADD(at[0], at[64]); 
   COMBA_STORE(C->dp[0]);
   /* 1 */
   COMBA_FORWARD;
   MULADD(at[0], at[65]);    MULADD(at[1], at[64]); 
   COMBA_STORE(C->dp[1]);
   /* 2 */
   COMBA_FORWARD;
   MULADD(at[0], at[66]);    MULADD(at[1], at[65]);    MULADD(at[2], at[64]); 
   COMBA_STORE(C->dp[2]);
   /* 3 */
   COMBA_FORWARD;
   MULADD(at[0], at[67]);    MULADD(at[1], at[66]);    MULADD(at[2], at[65]);    MULADD(at[3], at[64]); 
   COMBA_STORE(C->dp[3]);
   /* 4 */
   COMBA_FORWARD;
   MULADD(at[0], at[68]);    MULADD(at[1], at[67]);    MULADD(at[2], at[66]);    MULADD(at[3], at[65]);    MULADD(at[4], at[64]); 
   COMBA_STORE(C->dp[4]);
   /* 5 */
   COMBA_FORWARD;
   MULADD(at[0], at[69]);    MULADD(at[1], at[68]);    MULADD(at[2], at[67]);    MULADD(at[3], at[66]);    MULADD(at[4], at[65]);    MULADD(at[5], at[64]); 
   COMBA_STORE(C->dp[5]);
   /* 6 */
   COMBA_FORWARD;
   MULADD(at[0], at[70]);    MULADD(at[1], at[69]);    MULADD(at[2], at[68]);    MULADD(at[3], at[67]);    MULADD(at[4], at[66]);    MULADD(at[5], at[65]);    MULADD(at[6], at[64]); 
   COMBA_STORE(C->dp[6]);
   /* 7 */
   COMBA_FORWARD;
   MULADD(at[0], at[71]);    MULADD(at[1], at[70]);    MULADD(at[2], at[69]);    MULADD(at[3], at[68]);    MULADD(at[4], at[67]);    MULADD(at[5], at[66]);    MULADD(at[6], at[65]);    MULADD(at[7], at[64]); 
   COMBA_STORE(C->dp[7]);
   /* 8 */
   COMBA_FORWARD;
   MULADD(at[0], at[72]);    MULADD(at[1], at[71]);    MULADD(at[2], at[70]);    MULADD(at[3], at[69]);    MULADD(at[4], at[68]);    MULADD(at[5], at[67]);    MULADD(at[6], at[66]);    MULADD(at[7], at[65]);    MULADD(at[8], at[64]); 
   COMBA_STORE(C->dp[8]);
   /* 9 */
   COMBA_FORWARD;
   MULADD(at[0], at[73]);    MULADD(at[1], at[72]);    MULADD(at[2], at[71]);    MULADD(at[3], at[70]);    MULADD(at[4], at[69]);    MULADD(at[5], at[68]);    MULADD(at[6], at[67]);    MULADD(at[7], at[66]);    MULADD(at[8], at[65]);    MULADD(at[9], at[64]); 
   COMBA_STORE(C->dp[9]);
   /* 10 */
   COMBA_FORWARD;
   MULADD(at[0], at[74]);    MULADD(at[1], at[73]);    MULADD(at[2], at[72]);    MULADD(at[3], at[71]);    MULADD(at[4], at[70]);    MULADD(at[5], at[69]);    MULADD(at[6], at[68]);    MULADD(at[7], at[67]);    MULADD(at[8], at[66]);    MULADD(at[9], at[65]);    MULADD(at[10], at[64]); 
   COMBA_STORE(C->dp[10]);
   /* 11 */
   COMBA_FORWARD;
   MULADD(at[0], at[75]);    MULADD(at[1], at[74]);    MULADD(at[2], at[73]);    MULADD(at[3], at[72]);    MULADD(at[4], at[71]);    MULADD(at[5], at[70]);    MULADD(at[6], at[69]);    MULADD(at[7], at[68]);    MULADD(at[8], at[67]);    MULADD(at[9], at[66]);    MULADD(at[10], at[65]);    MULADD(at[11], at[64]); 
   COMBA_STORE(C->dp[11]);
   /* 12 */
   COMBA_FORWARD;
   MULADD(at[0], at[76]);    MULADD(at[1], at[75]);    MULADD(at[2], at[74]);    MULADD(at[3], at[73]);    MULADD(at[4], at[72]);    MULADD(at[5], at[71]);    MULADD(at[6], at[70]);    MULADD(at[7], at[69]);    MULADD(at[8], at[68]);    MULADD(at[9], at[67]);    MULADD(at[10], at[66]);    MULADD(at[11], at[65]);    MULADD(at[12], at[64]); 
   COMBA_STORE(C->dp[12]);
   /* 13 */
   COMBA_FORWARD;
   MULADD(at[0], at[77]);    MULADD(at[1], at[76]);    MULADD(at[2], at[75]);    MULADD(at[3], at[74]);    MULADD(at[4], at[73]);    MULADD(at[5], at[72]);    MULADD(at[6], at[71]);    MULADD(at[7], at[70]);    MULADD(at[8], at[69]);    MULADD(at[9], at[68]);    MULADD(at[10], at[67]);    MULADD(at[11], at[66]);    MULADD(at[12], at[65]);    MULADD(at[13], at[64]); 
   COMBA_STORE(C->dp[13]);
   /* 14 */
   COMBA_FORWARD;
   MULADD(at[0], at[78]);    MULADD(at[1], at[77]);    MULADD(at[2], at[76]);    MULADD(at[3], at[75]);    MULADD(at[4], at[74]);    MULADD(at[5], at[73]);    MULADD(at[6], at[72]);    MULADD(at[7], at[71]);    MULADD(at[8], at[70]);    MULADD(at[9], at[69]);    MULADD(at[10], at[68]);    MULADD(at[11], at[67]);    MULADD(at[12], at[66]);    MULADD(at[13], at[65]);    MULADD(at[14], at[64]); 
   COMBA_STORE(C->dp[14]);
   /* 15 */
   COMBA_FORWARD;
   MULADD(at[0], at[79]);    MULADD(at[1], at[78]);    MULADD(at[2], at[77]);    MULADD(at[3], at[76]);    MULADD(at[4], at[75]);    MULADD(at[5], at[74]);    MULADD(at[6], at[73]);    MULADD(at[7], at[72]);    MULADD(at[8], at[71]);    MULADD(at[9], at[70]);    MULADD(at[10], at[69]);    MULADD(at[11], at[68]);    MULADD(at[12], at[67]);    MULADD(at[13], at[66]);    MULADD(at[14], at[65]);    MULADD(at[15], at[64]); 
   COMBA_STORE(C->dp[15]);
   /* 16 */
   COMBA_FORWARD;
   MULADD(at[0], at[80]);    MULADD(at[1], at[79]);    MULADD(at[2], at[78]);    MULADD(at[3], at[77]);    MULADD(at[4], at[76]);    MULADD(at[5], at[75]);    MULADD(at[6], at[74]);    MULADD(at[7], at[73]);    MULADD(at[8], at[72]);    MULADD(at[9], at[71]);    MULADD(at[10], at[70]);    MULADD(at[11], at[69]);    MULADD(at[12], at[68]);    MULADD(at[13], at[67]);    MULADD(at[14], at[66]);    MULADD(at[15], at[65]);    MULADD(at[16], at[64]); 
   COMBA_STORE(C->dp[16]);
   /* 17 */
   COMBA_FORWARD;
   MULADD(at[0], at[81]);    MULADD(at[1], at[80]);    MULADD(at[2], at[79]);    MULADD(at[3], at[78]);    MULADD(at[4], at[77]);    MULADD(at[5], at[76]);    MULADD(at[6], at[75]);    MULADD(at[7], at[74]);    MULADD(at[8], at[73]);    MULADD(at[9], at[72]);    MULADD(at[10], at[71]);    MULADD(at[11], at[70]);    MULADD(at[12], at[69]);    MULADD(at[13], at[68]);    MULADD(at[14], at[67]);    MULADD(at[15], at[66]);    MULADD(at[16], at[65]);    MULADD(at[17], at[64]); 
   COMBA_STORE(C->dp[17]);
   /* 18 */
   COMBA_FORWARD;
   MULADD(at[0], at[82]);    MULADD(at[1], at[81]);    MULADD(at[2], at[80]);    MULADD(at[3], at[79]);    MULADD(at[4], at[78]);    MULADD(at[5], at[77]);    MULADD(at[6], at[76]);    MULADD(at[7], at[75]);    MULADD(at[8], at[74]);    MULADD(at[9], at[73]);    MULADD(at[10], at[72]);    MULADD(at[11], at[71]);    MULADD(at[12], at[70]);    MULADD(at[13], at[69]);    MULADD(at[14], at[68]);    MULADD(at[15], at[67]);    MULADD(at[16], at[66]);    MULADD(at[17], at[65]);    MULADD(at[18], at[64]); 
   COMBA_STORE(C->dp[18]);
   /* 19 */
   COMBA_FORWARD;
   MULADD(at[0], at[83]);    MULADD(at[1], at[82]);    MULADD(at[2], at[81]);    MULADD(at[3], at[80]);    MULADD(at[4], at[79]);    MULADD(at[5], at[78]);    MULADD(at[6], at[77]);    MULADD(at[7], at[76]);    MULADD(at[8], at[75]);    MULADD(at[9], at[74]);    MULADD(at[10], at[73]);    MULADD(at[11], at[72]);    MULADD(at[12], at[71]);    MULADD(at[13], at[70]);    MULADD(at[14], at[69]);    MULADD(at[15], at[68]);    MULADD(at[16], at[67]);    MULADD(at[17], at[66]);    MULADD(at[18], at[65]);    MULADD(at[19], at[64]); 
   COMBA_STORE(C->dp[19]);
   /* 20 */
   COMBA_FORWARD;
   MULADD(at[0], at[84]);    MULADD(at[1], at[83]);    MULADD(at[2], at[82]);    MULADD(at[3], at[81]);    MULADD(at[4], at[80]);    MULADD(at[5], at[79]);    MULADD(at[6], at[78]);    MULADD(at[7], at[77]);    MULADD(at[8], at[76]);    MULADD(at[9], at[75]);    MULADD(at[10], at[74]);    MULADD(at[11], at[73]);    MULADD(at[12], at[72]);    MULADD(at[13], at[71]);    MULADD(at[14], at[70]);    MULADD(at[15], at[69]);    MULADD(at[16], at[68]);    MULADD(at[17], at[67]);    MULADD(at[18], at[66]);    MULADD(at[19], at[65]);    MULADD(at[20], at[64]); 
   COMBA_STORE(C->dp[20]);
   /* 21 */
   COMBA_FORWARD;
   MULADD(at[0], at[85]);    MULADD(at[1], at[84]);    MULADD(at[2], at[83]);    MULADD(at[3], at[82]);    MULADD(at[4], at[81]);    MULADD(at[5], at[80]);    MULADD(at[6], at[79]);    MULADD(at[7], at[78]);    MULADD(at[8], at[77]);    MULADD(at[9], at[76]);    MULADD(at[10], at[75]);    MULADD(at[11], at[74]);    MULADD(at[12], at[73]);    MULADD(at[13], at[72]);    MULADD(at[14], at[71]);    MULADD(at[15], at[70]);    MULADD(at[16], at[69]);    MULADD(at[17], at[68]);    MULADD(at[18], at[67]);    MULADD(at[19], at[66]);    MULADD(at[20], at[65]);    MULADD(at[21], at[64]); 
   COMBA_STORE(C->dp[21]);
   /* 22 */
   COMBA_FORWARD;
   MULADD(at[0], at[86]);    MULADD(at[1], at[85]);    MULADD(at[2], at[84]);    MULADD(at[3], at[83]);    MULADD(at[4], at[82]);    MULADD(at[5], at[81]);    MULADD(at[6], at[80]);    MULADD(at[7], at[79]);    MULADD(at[8], at[78]);    MULADD(at[9], at[77]);    MULADD(at[10], at[76]);    MULADD(at[11], at[75]);    MULADD(at[12], at[74]);    MULADD(at[13], at[73]);    MULADD(at[14], at[72]);    MULADD(at[15], at[71]);    MULADD(at[16], at[70]);    MULADD(at[17], at[69]);    MULADD(at[18], at[68]);    MULADD(at[19], at[67]);    MULADD(at[20], at[66]);    MULADD(at[21], at[65]);    MULADD(at[22], at[64]); 
   COMBA_STORE(C->dp[22]);
   /* 23 */
   COMBA_FORWARD;
   MULADD(at[0], at[87]);    MULADD(at[1], at[86]);    MULADD(at[2], at[85]);    MULADD(at[3], at[84]);    MULADD(at[4], at[83]);    MULADD(at[5], at[82]);    MULADD(at[6], at[81]);    MULADD(at[7], at[80]);    MULADD(at[8], at[79]);    MULADD(at[9], at[78]);    MULADD(at[10], at[77]);    MULADD(at[11], at[76]);    MULADD(at[12], at[75]);    MULADD(at[13], at[74]);    MULADD(at[14], at[73]);    MULADD(at[15], at[72]);    MULADD(at[16], at[71]);    MULADD(at[17], at[70]);    MULADD(at[18], at[69]);    MULADD(at[19], at[68]);    MULADD(at[20], at[67]);    MULADD(at[21], at[66]);    MULADD(at[22], at[65]);    MULADD(at[23], at[64]); 
   COMBA_STORE(C->dp[23]);
   /* 24 */
   COMBA_FORWARD;
   MULADD(at[0], at[88]);    MULADD(at[1], at[87]);    MULADD(at[2], at[86]);    MULADD(at[3], at[85]);    MULADD(at[4], at[84]);    MULADD(at[5], at[83]);    MULADD(at[6], at[82]);    MULADD(at[7], at[81]);    MULADD(at[8], at[80]);    MULADD(at[9], at[79]);    MULADD(at[10], at[78]);    MULADD(at[11], at[77]);    MULADD(at[12], at[76]);    MULADD(at[13], at[75]);    MULADD(at[14], at[74]);    MULADD(at[15], at[73]);    MULADD(at[16], at[72]);    MULADD(at[17], at[71]);    MULADD(at[18], at[70]);    MULADD(at[19], at[69]);    MULADD(at[20], at[68]);    MULADD(at[21], at[67]);    MULADD(at[22], at[66]);    MULADD(at[23], at[65]);    MULADD(at[24], at[64]); 
   COMBA_STORE(C->dp[24]);
   /* 25 */
   COMBA_FORWARD;
   MULADD(at[0], at[89]);    MULADD(at[1], at[88]);    MULADD(at[2], at[87]);    MULADD(at[3], at[86]);    MULADD(at[4], at[85]);    MULADD(at[5], at[84]);    MULADD(at[6], at[83]);    MULADD(at[7], at[82]);    MULADD(at[8], at[81]);    MULADD(at[9], at[80]);    MULADD(at[10], at[79]);    MULADD(at[11], at[78]);    MULADD(at[12], at[77]);    MULADD(at[13], at[76]);    MULADD(at[14], at[75]);    MULADD(at[15], at[74]);    MULADD(at[16], at[73]);    MULADD(at[17], at[72]);    MULADD(at[18], at[71]);    MULADD(at[19], at[70]);    MULADD(at[20], at[69]);    MULADD(at[21], at[68]);    MULADD(at[22], at[67]);    MULADD(at[23], at[66]);    MULADD(at[24], at[65]);    MULADD(at[25], at[64]); 
   COMBA_STORE(C->dp[25]);
   /* 26 */
   COMBA_FORWARD;
   MULADD(at[0], at[90]);    MULADD(at[1], at[89]);    MULADD(at[2], at[88]);    MULADD(at[3], at[87]);    MULADD(at[4], at[86]);    MULADD(at[5], at[85]);    MULADD(at[6], at[84]);    MULADD(at[7], at[83]);    MULADD(at[8], at[82]);    MULADD(at[9], at[81]);    MULADD(at[10], at[80]);    MULADD(at[11], at[79]);    MULADD(at[12], at[78]);    MULADD(at[13], at[77]);    MULADD(at[14], at[76]);    MULADD(at[15], at[75]);    MULADD(at[16], at[74]);    MULADD(at[17], at[73]);    MULADD(at[18], at[72]);    MULADD(at[19], at[71]);    MULADD(at[20], at[70]);    MULADD(at[21], at[69]);    MULADD(at[22], at[68]);    MULADD(at[23], at[67]);    MULADD(at[24], at[66]);    MULADD(at[25], at[65]);    MULADD(at[26], at[64]); 
   COMBA_STORE(C->dp[26]);
   /* 27 */
   COMBA_FORWARD;
   MULADD(at[0], at[91]);    MULADD(at[1], at[90]);    MULADD(at[2], at[89]);    MULADD(at[3], at[88]);    MULADD(at[4], at[87]);    MULADD(at[5], at[86]);    MULADD(at[6], at[85]);    MULADD(at[7], at[84]);    MULADD(at[8], at[83]);    MULADD(at[9], at[82]);    MULADD(at[10], at[81]);    MULADD(at[11], at[80]);    MULADD(at[12], at[79]);    MULADD(at[13], at[78]);    MULADD(at[14], at[77]);    MULADD(at[15], at[76]);    MULADD(at[16], at[75]);    MULADD(at[17], at[74]);    MULADD(at[18], at[73]);    MULADD(at[19], at[72]);    MULADD(at[20], at[71]);    MULADD(at[21], at[70]);    MULADD(at[22], at[69]);    MULADD(at[23], at[68]);    MULADD(at[24], at[67]);    MULADD(at[25], at[66]);    MULADD(at[26], at[65]);    MULADD(at[27], at[64]); 
   COMBA_STORE(C->dp[27]);
   /* 28 */
   COMBA_FORWARD;
   MULADD(at[0], at[92]);    MULADD(at[1], at[91]);    MULADD(at[2], at[90]);    MULADD(at[3], at[89]);    MULADD(at[4], at[88]);    MULADD(at[5], at[87]);    MULADD(at[6], at[86]);    MULADD(at[7], at[85]);    MULADD(at[8], at[84]);    MULADD(at[9], at[83]);    MULADD(at[10], at[82]);    MULADD(at[11], at[81]);    MULADD(at[12], at[80]);    MULADD(at[13], at[79]);    MULADD(at[14], at[78]);    MULADD(at[15], at[77]);    MULADD(at[16], at[76]);    MULADD(at[17], at[75]);    MULADD(at[18], at[74]);    MULADD(at[19], at[73]);    MULADD(at[20], at[72]);    MULADD(at[21], at[71]);    MULADD(at[22], at[70]);    MULADD(at[23], at[69]);    MULADD(at[24], at[68]);    MULADD(at[25], at[67]);    MULADD(at[26], at[66]);    MULADD(at[27], at[65]);    MULADD(at[28], at[64]); 
   COMBA_STORE(C->dp[28]);
   /* 29 */
   COMBA_FORWARD;
   MULADD(at[0], at[93]);    MULADD(at[1], at[92]);    MULADD(at[2], at[91]);    MULADD(at[3], at[90]);    MULADD(at[4], at[89]);    MULADD(at[5], at[88]);    MULADD(at[6], at[87]);    MULADD(at[7], at[86]);    MULADD(at[8], at[85]);    MULADD(at[9], at[84]);    MULADD(at[10], at[83]);    MULADD(at[11], at[82]);    MULADD(at[12], at[81]);    MULADD(at[13], at[80]);    MULADD(at[14], at[79]);    MULADD(at[15], at[78]);    MULADD(at[16], at[77]);    MULADD(at[17], at[76]);    MULADD(at[18], at[75]);    MULADD(at[19], at[74]);    MULADD(at[20], at[73]);    MULADD(at[21], at[72]);    MULADD(at[22], at[71]);    MULADD(at[23], at[70]);    MULADD(at[24], at[69]);    MULADD(at[25], at[68]);    MULADD(at[26], at[67]);    MULADD(at[27], at[66]);    MULADD(at[28], at[65]);    MULADD(at[29], at[64]); 
   COMBA_STORE(C->dp[29]);
   /* 30 */
   COMBA_FORWARD;
   MULADD(at[0], at[94]);    MULADD(at[1], at[93]);    MULADD(at[2], at[92]);    MULADD(at[3], at[91]);    MULADD(at[4], at[90]);    MULADD(at[5], at[89]);    MULADD(at[6], at[88]);    MULADD(at[7], at[87]);    MULADD(at[8], at[86]);    MULADD(at[9], at[85]);    MULADD(at[10], at[84]);    MULADD(at[11], at[83]);    MULADD(at[12], at[82]);    MULADD(at[13], at[81]);    MULADD(at[14], at[80]);    MULADD(at[15], at[79]);    MULADD(at[16], at[78]);    MULADD(at[17], at[77]);    MULADD(at[18], at[76]);    MULADD(at[19], at[75]);    MULADD(at[20], at[74]);    MULADD(at[21], at[73]);    MULADD(at[22], at[72]);    MULADD(at[23], at[71]);    MULADD(at[24], at[70]);    MULADD(at[25], at[69]);    MULADD(at[26], at[68]);    MULADD(at[27], at[67]);    MULADD(at[28], at[66]);    MULADD(at[29], at[65]);    MULADD(at[30], at[64]); 
   COMBA_STORE(C->dp[30]);
   /* 31 */
   COMBA_FORWARD;
   MULADD(at[0], at[95]);    MULADD(at[1], at[94]);    MULADD(at[2], at[93]);    MULADD(at[3], at[92]);    MULADD(at[4], at[91]);    MULADD(at[5], at[90]);    MULADD(at[6], at[89]);    MULADD(at[7], at[88]);    MULADD(at[8], at[87]);    MULADD(at[9], at[86]);    MULADD(at[10], at[85]);    MULADD(at[11], at[84]);    MULADD(at[12], at[83]);    MULADD(at[13], at[82]);    MULADD(at[14], at[81]);    MULADD(at[15], at[80]);    MULADD(at[16], at[79]);    MULADD(at[17], at[78]);    MULADD(at[18], at[77]);    MULADD(at[19], at[76]);    MULADD(at[20], at[75]);    MULADD(at[21], at[74]);    MULADD(at[22], at[73]);    MULADD(at[23], at[72]);    MULADD(at[24], at[71]);    MULADD(at[25], at[70]);    MULADD(at[26], at[69]);    MULADD(at[27], at[68]);    MULADD(at[28], at[67]);    MULADD(at[29], at[66]);    MULADD(at[30], at[65]);    MULADD(at[31], at[64]); 
   COMBA_STORE(C->dp[31]);
   /* 32 */
   COMBA_FORWARD;
   MULADD(at[0], at[96]);    MULADD(at[1], at[95]);    MULADD(at[2], at[94]);    MULADD(at[3], at[93]);    MULADD(at[4], at[92]);    MULADD(at[5], at[91]);    MULADD(at[6], at[90]);    MULADD(at[7], at[89]);    MULADD(at[8], at[88]);    MULADD(at[9], at[87]);    MULADD(at[10], at[86]);    MULADD(at[11], at[85]);    MULADD(at[12], at[84]);    MULADD(at[13], at[83]);    MULADD(at[14], at[82]);    MULADD(at[15], at[81]);    MULADD(at[16], at[80]);    MULADD(at[17], at[79]);    MULADD(at[18], at[78]);    MULADD(at[19], at[77]);    MULADD(at[20], at[76]);    MULADD(at[21], at[75]);    MULADD(at[22], at[74]);    MULADD(at[23], at[73]);    MULADD(at[24], at[72]);    MULADD(at[25], at[71]);    MULADD(at[26], at[70]);    MULADD(at[27], at[69]);    MULADD(at[28], at[68]);    MULADD(at[29], at[67]);    MULADD(at[30], at[66]);    MULADD(at[31], at[65]);    MULADD(at[32], at[64]); 
   COMBA_STORE(C->dp[32]);
   /* 33 */
   COMBA_FORWARD;
   MULADD(at[0], at[97]);    MULADD(at[1], at[96]);    MULADD(at[2], at[95]);    MULADD(at[3], at[94]);    MULADD(at[4], at[93]);    MULADD(at[5], at[92]);    MULADD(at[6], at[91]);    MULADD(at[7], at[90]);    MULADD(at[8], at[89]);    MULADD(at[9], at[88]);    MULADD(at[10], at[87]);    MULADD(at[11], at[86]);    MULADD(at[12], at[85]);    MULADD(at[13], at[84]);    MULADD(at[14], at[83]);    MULADD(at[15], at[82]);    MULADD(at[16], at[81]);    MULADD(at[17], at[80]);    MULADD(at[18], at[79]);    MULADD(at[19], at[78]);    MULADD(at[20], at[77]);    MULADD(at[21], at[76]);    MULADD(at[22], at[75]);    MULADD(at[23], at[74]);    MULADD(at[24], at[73]);    MULADD(at[25], at[72]);    MULADD(at[26], at[71]);    MULADD(at[27], at[70]);    MULADD(at[28], at[69]);    MULADD(at[29], at[68]);    MULADD(at[30], at[67]);    MULADD(at[31], at[66]);    MULADD(at[32], at[65]);    MULADD(at[33], at[64]); 
   COMBA_STORE(C->dp[33]);
   /* 34 */
   COMBA_FORWARD;
   MULADD(at[0], at[98]);    MULADD(at[1], at[97]);    MULADD(at[2], at[96]);    MULADD(at[3], at[95]);    MULADD(at[4], at[94]);    MULADD(at[5], at[93]);    MULADD(at[6], at[92]);    MULADD(at[7], at[91]);    MULADD(at[8], at[90]);    MULADD(at[9], at[89]);    MULADD(at[10], at[88]);    MULADD(at[11], at[87]);    MULADD(at[12], at[86]);    MULADD(at[13], at[85]);    MULADD(at[14], at[84]);    MULADD(at[15], at[83]);    MULADD(at[16], at[82]);    MULADD(at[17], at[81]);    MULADD(at[18], at[80]);    MULADD(at[19], at[79]);    MULADD(at[20], at[78]);    MULADD(at[21], at[77]);    MULADD(at[22], at[76]);    MULADD(at[23], at[75]);    MULADD(at[24], at[74]);    MULADD(at[25], at[73]);    MULADD(at[26], at[72]);    MULADD(at[27], at[71]);    MULADD(at[28], at[70]);    MULADD(at[29], at[69]);    MULADD(at[30], at[68]);    MULADD(at[31], at[67]);    MULADD(at[32], at[66]);    MULADD(at[33], at[65]);    MULADD(at[34], at[64]); 
   COMBA_STORE(C->dp[34]);
   /* 35 */
   COMBA_FORWARD;
   MULADD(at[0], at[99]);    MULADD(at[1], at[98]);    MULADD(at[2], at[97]);    MULADD(at[3], at[96]);    MULADD(at[4], at[95]);    MULADD(at[5], at[94]);    MULADD(at[6], at[93]);    MULADD(at[7], at[92]);    MULADD(at[8], at[91]);    MULADD(at[9], at[90]);    MULADD(at[10], at[89]);    MULADD(at[11], at[88]);    MULADD(at[12], at[87]);    MULADD(at[13], at[86]);    MULADD(at[14], at[85]);    MULADD(at[15], at[84]);    MULADD(at[16], at[83]);    MULADD(at[17], at[82]);    MULADD(at[18], at[81]);    MULADD(at[19], at[80]);    MULADD(at[20], at[79]);    MULADD(at[21], at[78]);    MULADD(at[22], at[77]);    MULADD(at[23], at[76]);    MULADD(at[24], at[75]);    MULADD(at[25], at[74]);    MULADD(at[26], at[73]);    MULADD(at[27], at[72]);    MULADD(at[28], at[71]);    MULADD(at[29], at[70]);    MULADD(at[30], at[69]);    MULADD(at[31], at[68]);    MULADD(at[32], at[67]);    MULADD(at[33], at[66]);    MULADD(at[34], at[65]);    MULADD(at[35], at[64]); 
   COMBA_STORE(C->dp[35]);
   /* 36 */
   COMBA_FORWARD;
   MULADD(at[0], at[100]);    MULADD(at[1], at[99]);    MULADD(at[2], at[98]);    MULADD(at[3], at[97]);    MULADD(at[4], at[96]);    MULADD(at[5], at[95]);    MULADD(at[6], at[94]);    MULADD(at[7], at[93]);    MULADD(at[8], at[92]);    MULADD(at[9], at[91]);    MULADD(at[10], at[90]);    MULADD(at[11], at[89]);    MULADD(at[12], at[88]);    MULADD(at[13], at[87]);    MULADD(at[14], at[86]);    MULADD(at[15], at[85]);    MULADD(at[16], at[84]);    MULADD(at[17], at[83]);    MULADD(at[18], at[82]);    MULADD(at[19], at[81]);    MULADD(at[20], at[80]);    MULADD(at[21], at[79]);    MULADD(at[22], at[78]);    MULADD(at[23], at[77]);    MULADD(at[24], at[76]);    MULADD(at[25], at[75]);    MULADD(at[26], at[74]);    MULADD(at[27], at[73]);    MULADD(at[28], at[72]);    MULADD(at[29], at[71]);    MULADD(at[30], at[70]);    MULADD(at[31], at[69]);    MULADD(at[32], at[68]);    MULADD(at[33], at[67]);    MULADD(at[34], at[66]);    MULADD(at[35], at[65]);    MULADD(at[36], at[64]); 
   COMBA_STORE(C->dp[36]);
   /* 37 */
   COMBA_FORWARD;
   MULADD(at[0], at[101]);    MULADD(at[1], at[100]);    MULADD(at[2], at[99]);    MULADD(at[3], at[98]);    MULADD(at[4], at[97]);    MULADD(at[5], at[96]);    MULADD(at[6], at[95]);    MULADD(at[7], at[94]);    MULADD(at[8], at[93]);    MULADD(at[9], at[92]);    MULADD(at[10], at[91]);    MULADD(at[11], at[90]);    MULADD(at[12], at[89]);    MULADD(at[13], at[88]);    MULADD(at[14], at[87]);    MULADD(at[15], at[86]);    MULADD(at[16], at[85]);    MULADD(at[17], at[84]);    MULADD(at[18], at[83]);    MULADD(at[19], at[82]);    MULADD(at[20], at[81]);    MULADD(at[21], at[80]);    MULADD(at[22], at[79]);    MULADD(at[23], at[78]);    MULADD(at[24], at[77]);    MULADD(at[25], at[76]);    MULADD(at[26], at[75]);    MULADD(at[27], at[74]);    MULADD(at[28], at[73]);    MULADD(at[29], at[72]);    MULADD(at[30], at[71]);    MULADD(at[31], at[70]);    MULADD(at[32], at[69]);    MULADD(at[33], at[68]);    MULADD(at[34], at[67]);    MULADD(at[35], at[66]);    MULADD(at[36], at[65]);    MULADD(at[37], at[64]); 
   COMBA_STORE(C->dp[37]);
   /* 38 */
   COMBA_FORWARD;
   MULADD(at[0], at[102]);    MULADD(at[1], at[101]);    MULADD(at[2], at[100]);    MULADD(at[3], at[99]);    MULADD(at[4], at[98]);    MULADD(at[5], at[97]);    MULADD(at[6], at[96]);    MULADD(at[7], at[95]);    MULADD(at[8], at[94]);    MULADD(at[9], at[93]);    MULADD(at[10], at[92]);    MULADD(at[11], at[91]);    MULADD(at[12], at[90]);    MULADD(at[13], at[89]);    MULADD(at[14], at[88]);    MULADD(at[15], at[87]);    MULADD(at[16], at[86]);    MULADD(at[17], at[85]);    MULADD(at[18], at[84]);    MULADD(at[19], at[83]);    MULADD(at[20], at[82]);    MULADD(at[21], at[81]);    MULADD(at[22], at[80]);    MULADD(at[23], at[79]);    MULADD(at[24], at[78]);    MULADD(at[25], at[77]);    MULADD(at[26], at[76]);    MULADD(at[27], at[75]);    MULADD(at[28], at[74]);    MULADD(at[29], at[73]);    MULADD(at[30], at[72]);    MULADD(at[31], at[71]);    MULADD(at[32], at[70]);    MULADD(at[33], at[69]);    MULADD(at[34], at[68]);    MULADD(at[35], at[67]);    MULADD(at[36], at[66]);    MULADD(at[37], at[65]);    MULADD(at[38], at[64]); 
   COMBA_STORE(C->dp[38]);
   /* 39 */
   COMBA_FORWARD;
   MULADD(at[0], at[103]);    MULADD(at[1], at[102]);    MULADD(at[2], at[101]);    MULADD(at[3], at[100]);    MULADD(at[4], at[99]);    MULADD(at[5], at[98]);    MULADD(at[6], at[97]);    MULADD(at[7], at[96]);    MULADD(at[8], at[95]);    MULADD(at[9], at[94]);    MULADD(at[10], at[93]);    MULADD(at[11], at[92]);    MULADD(at[12], at[91]);    MULADD(at[13], at[90]);    MULADD(at[14], at[89]);    MULADD(at[15], at[88]);    MULADD(at[16], at[87]);    MULADD(at[17], at[86]);    MULADD(at[18], at[85]);    MULADD(at[19], at[84]);    MULADD(at[20], at[83]);    MULADD(at[21], at[82]);    MULADD(at[22], at[81]);    MULADD(at[23], at[80]);    MULADD(at[24], at[79]);    MULADD(at[25], at[78]);    MULADD(at[26], at[77]);    MULADD(at[27], at[76]);    MULADD(at[28], at[75]);    MULADD(at[29], at[74]);    MULADD(at[30], at[73]);    MULADD(at[31], at[72]);    MULADD(at[32], at[71]);    MULADD(at[33], at[70]);    MULADD(at[34], at[69]);    MULADD(at[35], at[68]);    MULADD(at[36], at[67]);    MULADD(at[37], at[66]);    MULADD(at[38], at[65]);    MULADD(at[39], at[64]); 
   COMBA_STORE(C->dp[39]);
   /* 40 */
   COMBA_FORWARD;
   MULADD(at[0], at[104]);    MULADD(at[1], at[103]);    MULADD(at[2], at[102]);    MULADD(at[3], at[101]);    MULADD(at[4], at[100]);    MULADD(at[5], at[99]);    MULADD(at[6], at[98]);    MULADD(at[7], at[97]);    MULADD(at[8], at[96]);    MULADD(at[9], at[95]);    MULADD(at[10], at[94]);    MULADD(at[11], at[93]);    MULADD(at[12], at[92]);    MULADD(at[13], at[91]);    MULADD(at[14], at[90]);    MULADD(at[15], at[89]);    MULADD(at[16], at[88]);    MULADD(at[17], at[87]);    MULADD(at[18], at[86]);    MULADD(at[19], at[85]);    MULADD(at[20], at[84]);    MULADD(at[21], at[83]);    MULADD(at[22], at[82]);    MULADD(at[23], at[81]);    MULADD(at[24], at[80]);    MULADD(at[25], at[79]);    MULADD(at[26], at[78]);    MULADD(at[27], at[77]);    MULADD(at[28], at[76]);    MULADD(at[29], at[75]);    MULADD(at[30], at[74]);    MULADD(at[31], at[73]);    MULADD(at[32], at[72]);    MULADD(at[33], at[71]);    MULADD(at[34], at[70]);    MULADD(at[35], at[69]);    MULADD(at[36], at[68]);    MULADD(at[37], at[67]);    MULADD(at[38], at[66]);    MULADD(at[39], at[65]);    MULADD(at[40], at[64]); 
   COMBA_STORE(C->dp[40]);
   /* 41 */
   COMBA_FORWARD;
   MULADD(at[0], at[105]);    MULADD(at[1], at[104]);    MULADD(at[2], at[103]);    MULADD(at[3], at[102]);    MULADD(at[4], at[101]);    MULADD(at[5], at[100]);    MULADD(at[6], at[99]);    MULADD(at[7], at[98]);    MULADD(at[8], at[97]);    MULADD(at[9], at[96]);    MULADD(at[10], at[95]);    MULADD(at[11], at[94]);    MULADD(at[12], at[93]);    MULADD(at[13], at[92]);    MULADD(at[14], at[91]);    MULADD(at[15], at[90]);    MULADD(at[16], at[89]);    MULADD(at[17], at[88]);    MULADD(at[18], at[87]);    MULADD(at[19], at[86]);    MULADD(at[20], at[85]);    MULADD(at[21], at[84]);    MULADD(at[22], at[83]);    MULADD(at[23], at[82]);    MULADD(at[24], at[81]);    MULADD(at[25], at[80]);    MULADD(at[26], at[79]);    MULADD(at[27], at[78]);    MULADD(at[28], at[77]);    MULADD(at[29], at[76]);    MULADD(at[30], at[75]);    MULADD(at[31], at[74]);    MULADD(at[32], at[73]);    MULADD(at[33], at[72]);    MULADD(at[34], at[71]);    MULADD(at[35], at[70]);    MULADD(at[36], at[69]);    MULADD(at[37], at[68]);    MULADD(at[38], at[67]);    MULADD(at[39], at[66]);    MULADD(at[40], at[65]);    MULADD(at[41], at[64]); 
   COMBA_STORE(C->dp[41]);
   /* 42 */
   COMBA_FORWARD;
   MULADD(at[0], at[106]);    MULADD(at[1], at[105]);    MULADD(at[2], at[104]);    MULADD(at[3], at[103]);    MULADD(at[4], at[102]);    MULADD(at[5], at[101]);    MULADD(at[6], at[100]);    MULADD(at[7], at[99]);    MULADD(at[8], at[98]);    MULADD(at[9], at[97]);    MULADD(at[10], at[96]);    MULADD(at[11], at[95]);    MULADD(at[12], at[94]);    MULADD(at[13], at[93]);    MULADD(at[14], at[92]);    MULADD(at[15], at[91]);    MULADD(at[16], at[90]);    MULADD(at[17], at[89]);    MULADD(at[18], at[88]);    MULADD(at[19], at[87]);    MULADD(at[20], at[86]);    MULADD(at[21], at[85]);    MULADD(at[22], at[84]);    MULADD(at[23], at[83]);    MULADD(at[24], at[82]);    MULADD(at[25], at[81]);    MULADD(at[26], at[80]);    MULADD(at[27], at[79]);    MULADD(at[28], at[78]);    MULADD(at[29], at[77]);    MULADD(at[30], at[76]);    MULADD(at[31], at[75]);    MULADD(at[32], at[74]);    MULADD(at[33], at[73]);    MULADD(at[34], at[72]);    MULADD(at[35], at[71]);    MULADD(at[36], at[70]);    MULADD(at[37], at[69]);    MULADD(at[38], at[68]);    MULADD(at[39], at[67]);    MULADD(at[40], at[66]);    MULADD(at[41], at[65]);    MULADD(at[42], at[64]); 
   COMBA_STORE(C->dp[42]);
   /* 43 */
   COMBA_FORWARD;
   MULADD(at[0], at[107]);    MULADD(at[1], at[106]);    MULADD(at[2], at[105]);    MULADD(at[3], at[104]);    MULADD(at[4], at[103]);    MULADD(at[5], at[102]);    MULADD(at[6], at[101]);    MULADD(at[7], at[100]);    MULADD(at[8], at[99]);    MULADD(at[9], at[98]);    MULADD(at[10], at[97]);    MULADD(at[11], at[96]);    MULADD(at[12], at[95]);    MULADD(at[13], at[94]);    MULADD(at[14], at[93]);    MULADD(at[15], at[92]);    MULADD(at[16], at[91]);    MULADD(at[17], at[90]);    MULADD(at[18], at[89]);    MULADD(at[19], at[88]);    MULADD(at[20], at[87]);    MULADD(at[21], at[86]);    MULADD(at[22], at[85]);    MULADD(at[23], at[84]);    MULADD(at[24], at[83]);    MULADD(at[25], at[82]);    MULADD(at[26], at[81]);    MULADD(at[27], at[80]);    MULADD(at[28], at[79]);    MULADD(at[29], at[78]);    MULADD(at[30], at[77]);    MULADD(at[31], at[76]);    MULADD(at[32], at[75]);    MULADD(at[33], at[74]);    MULADD(at[34], at[73]);    MULADD(at[35], at[72]);    MULADD(at[36], at[71]);    MULADD(at[37], at[70]);    MULADD(at[38], at[69]);    MULADD(at[39], at[68]);    MULADD(at[40], at[67]);    MULADD(at[41], at[66]);    MULADD(at[42], at[65]);    MULADD(at[43], at[64]); 
   COMBA_STORE(C->dp[43]);
   /* 44 */
   COMBA_FORWARD;
   MULADD(at[0], at[108]);    MULADD(at[1], at[107]);    MULADD(at[2], at[106]);    MULADD(at[3], at[105]);    MULADD(at[4], at[104]);    MULADD(at[5], at[103]);    MULADD(at[6], at[102]);    MULADD(at[7], at[101]);    MULADD(at[8], at[100]);    MULADD(at[9], at[99]);    MULADD(at[10], at[98]);    MULADD(at[11], at[97]);    MULADD(at[12], at[96]);    MULADD(at[13], at[95]);    MULADD(at[14], at[94]);    MULADD(at[15], at[93]);    MULADD(at[16], at[92]);    MULADD(at[17], at[91]);    MULADD(at[18], at[90]);    MULADD(at[19], at[89]);    MULADD(at[20], at[88]);    MULADD(at[21], at[87]);    MULADD(at[22], at[86]);    MULADD(at[23], at[85]);    MULADD(at[24], at[84]);    MULADD(at[25], at[83]);    MULADD(at[26], at[82]);    MULADD(at[27], at[81]);    MULADD(at[28], at[80]);    MULADD(at[29], at[79]);    MULADD(at[30], at[78]);    MULADD(at[31], at[77]);    MULADD(at[32], at[76]);    MULADD(at[33], at[75]);    MULADD(at[34], at[74]);    MULADD(at[35], at[73]);    MULADD(at[36], at[72]);    MULADD(at[37], at[71]);    MULADD(at[38], at[70]);    MULADD(at[39], at[69]);    MULADD(at[40], at[68]);    MULADD(at[41], at[67]);    MULADD(at[42], at[66]);    MULADD(at[43], at[65]);    MULADD(at[44], at[64]); 
   COMBA_STORE(C->dp[44]);
   /* 45 */
   COMBA_FORWARD;
   MULADD(at[0], at[109]);    MULADD(at[1], at[108]);    MULADD(at[2], at[107]);    MULADD(at[3], at[106]);    MULADD(at[4], at[105]);    MULADD(at[5], at[104]);    MULADD(at[6], at[103]);    MULADD(at[7], at[102]);    MULADD(at[8], at[101]);    MULADD(at[9], at[100]);    MULADD(at[10], at[99]);    MULADD(at[11], at[98]);    MULADD(at[12], at[97]);    MULADD(at[13], at[96]);    MULADD(at[14], at[95]);    MULADD(at[15], at[94]);    MULADD(at[16], at[93]);    MULADD(at[17], at[92]);    MULADD(at[18], at[91]);    MULADD(at[19], at[90]);    MULADD(at[20], at[89]);    MULADD(at[21], at[88]);    MULADD(at[22], at[87]);    MULADD(at[23], at[86]);    MULADD(at[24], at[85]);    MULADD(at[25], at[84]);    MULADD(at[26], at[83]);    MULADD(at[27], at[82]);    MULADD(at[28], at[81]);    MULADD(at[29], at[80]);    MULADD(at[30], at[79]);    MULADD(at[31], at[78]);    MULADD(at[32], at[77]);    MULADD(at[33], at[76]);    MULADD(at[34], at[75]);    MULADD(at[35], at[74]);    MULADD(at[36], at[73]);    MULADD(at[37], at[72]);    MULADD(at[38], at[71]);    MULADD(at[39], at[70]);    MULADD(at[40], at[69]);    MULADD(at[41], at[68]);    MULADD(at[42], at[67]);    MULADD(at[43], at[66]);    MULADD(at[44], at[65]);    MULADD(at[45], at[64]); 
   COMBA_STORE(C->dp[45]);
   /* 46 */
   COMBA_FORWARD;
   MULADD(at[0], at[110]);    MULADD(at[1], at[109]);    MULADD(at[2], at[108]);    MULADD(at[3], at[107]);    MULADD(at[4], at[106]);    MULADD(at[5], at[105]);    MULADD(at[6], at[104]);    MULADD(at[7], at[103]);    MULADD(at[8], at[102]);    MULADD(at[9], at[101]);    MULADD(at[10], at[100]);    MULADD(at[11], at[99]);    MULADD(at[12], at[98]);    MULADD(at[13], at[97]);    MULADD(at[14], at[96]);    MULADD(at[15], at[95]);    MULADD(at[16], at[94]);    MULADD(at[17], at[93]);    MULADD(at[18], at[92]);    MULADD(at[19], at[91]);    MULADD(at[20], at[90]);    MULADD(at[21], at[89]);    MULADD(at[22], at[88]);    MULADD(at[23], at[87]);    MULADD(at[24], at[86]);    MULADD(at[25], at[85]);    MULADD(at[26], at[84]);    MULADD(at[27], at[83]);    MULADD(at[28], at[82]);    MULADD(at[29], at[81]);    MULADD(at[30], at[80]);    MULADD(at[31], at[79]);    MULADD(at[32], at[78]);    MULADD(at[33], at[77]);    MULADD(at[34], at[76]);    MULADD(at[35], at[75]);    MULADD(at[36], at[74]);    MULADD(at[37], at[73]);    MULADD(at[38], at[72]);    MULADD(at[39], at[71]);    MULADD(at[40], at[70]);    MULADD(at[41], at[69]);    MULADD(at[42], at[68]);    MULADD(at[43], at[67]);    MULADD(at[44], at[66]);    MULADD(at[45], at[65]);    MULADD(at[46], at[64]); 
   COMBA_STORE(C->dp[46]);
   /* 47 */
   COMBA_FORWARD;
   MULADD(at[0], at[111]);    MULADD(at[1], at[110]);    MULADD(at[2], at[109]);    MULADD(at[3], at[108]);    MULADD(at[4], at[107]);    MULADD(at[5], at[106]);    MULADD(at[6], at[105]);    MULADD(at[7], at[104]);    MULADD(at[8], at[103]);    MULADD(at[9], at[102]);    MULADD(at[10], at[101]);    MULADD(at[11], at[100]);    MULADD(at[12], at[99]);    MULADD(at[13], at[98]);    MULADD(at[14], at[97]);    MULADD(at[15], at[96]);    MULADD(at[16], at[95]);    MULADD(at[17], at[94]);    MULADD(at[18], at[93]);    MULADD(at[19], at[92]);    MULADD(at[20], at[91]);    MULADD(at[21], at[90]);    MULADD(at[22], at[89]);    MULADD(at[23], at[88]);    MULADD(at[24], at[87]);    MULADD(at[25], at[86]);    MULADD(at[26], at[85]);    MULADD(at[27], at[84]);    MULADD(at[28], at[83]);    MULADD(at[29], at[82]);    MULADD(at[30], at[81]);    MULADD(at[31], at[80]);    MULADD(at[32], at[79]);    MULADD(at[33], at[78]);    MULADD(at[34], at[77]);    MULADD(at[35], at[76]);    MULADD(at[36], at[75]);    MULADD(at[37], at[74]);    MULADD(at[38], at[73]);    MULADD(at[39], at[72]);    MULADD(at[40], at[71]);    MULADD(at[41], at[70]);    MULADD(at[42], at[69]);    MULADD(at[43], at[68]);    MULADD(at[44], at[67]);    MULADD(at[45], at[66]);    MULADD(at[46], at[65]);    MULADD(at[47], at[64]); 
   COMBA_STORE(C->dp[47]);
   /* 48 */
   COMBA_FORWARD;
   MULADD(at[0], at[112]);    MULADD(at[1], at[111]);    MULADD(at[2], at[110]);    MULADD(at[3], at[109]);    MULADD(at[4], at[108]);    MULADD(at[5], at[107]);    MULADD(at[6], at[106]);    MULADD(at[7], at[105]);    MULADD(at[8], at[104]);    MULADD(at[9], at[103]);    MULADD(at[10], at[102]);    MULADD(at[11], at[101]);    MULADD(at[12], at[100]);    MULADD(at[13], at[99]);    MULADD(at[14], at[98]);    MULADD(at[15], at[97]);    MULADD(at[16], at[96]);    MULADD(at[17], at[95]);    MULADD(at[18], at[94]);    MULADD(at[19], at[93]);    MULADD(at[20], at[92]);    MULADD(at[21], at[91]);    MULADD(at[22], at[90]);    MULADD(at[23], at[89]);    MULADD(at[24], at[88]);    MULADD(at[25], at[87]);    MULADD(at[26], at[86]);    MULADD(at[27], at[85]);    MULADD(at[28], at[84]);    MULADD(at[29], at[83]);    MULADD(at[30], at[82]);    MULADD(at[31], at[81]);    MULADD(at[32], at[80]);    MULADD(at[33], at[79]);    MULADD(at[34], at[78]);    MULADD(at[35], at[77]);    MULADD(at[36], at[76]);    MULADD(at[37], at[75]);    MULADD(at[38], at[74]);    MULADD(at[39], at[73]);    MULADD(at[40], at[72]);    MULADD(at[41], at[71]);    MULADD(at[42], at[70]);    MULADD(at[43], at[69]);    MULADD(at[44], at[68]);    MULADD(at[45], at[67]);    MULADD(at[46], at[66]);    MULADD(at[47], at[65]);    MULADD(at[48], at[64]); 
   COMBA_STORE(C->dp[48]);
   /* 49 */
   COMBA_FORWARD;
   MULADD(at[0], at[113]);    MULADD(at[1], at[112]);    MULADD(at[2], at[111]);    MULADD(at[3], at[110]);    MULADD(at[4], at[109]);    MULADD(at[5], at[108]);    MULADD(at[6], at[107]);    MULADD(at[7], at[106]);    MULADD(at[8], at[105]);    MULADD(at[9], at[104]);    MULADD(at[10], at[103]);    MULADD(at[11], at[102]);    MULADD(at[12], at[101]);    MULADD(at[13], at[100]);    MULADD(at[14], at[99]);    MULADD(at[15], at[98]);    MULADD(at[16], at[97]);    MULADD(at[17], at[96]);    MULADD(at[18], at[95]);    MULADD(at[19], at[94]);    MULADD(at[20], at[93]);    MULADD(at[21], at[92]);    MULADD(at[22], at[91]);    MULADD(at[23], at[90]);    MULADD(at[24], at[89]);    MULADD(at[25], at[88]);    MULADD(at[26], at[87]);    MULADD(at[27], at[86]);    MULADD(at[28], at[85]);    MULADD(at[29], at[84]);    MULADD(at[30], at[83]);    MULADD(at[31], at[82]);    MULADD(at[32], at[81]);    MULADD(at[33], at[80]);    MULADD(at[34], at[79]);    MULADD(at[35], at[78]);    MULADD(at[36], at[77]);    MULADD(at[37], at[76]);    MULADD(at[38], at[75]);    MULADD(at[39], at[74]);    MULADD(at[40], at[73]);    MULADD(at[41], at[72]);    MULADD(at[42], at[71]);    MULADD(at[43], at[70]);    MULADD(at[44], at[69]);    MULADD(at[45], at[68]);    MULADD(at[46], at[67]);    MULADD(at[47], at[66]);    MULADD(at[48], at[65]);    MULADD(at[49], at[64]); 
   COMBA_STORE(C->dp[49]);
   /* 50 */
   COMBA_FORWARD;
   MULADD(at[0], at[114]);    MULADD(at[1], at[113]);    MULADD(at[2], at[112]);    MULADD(at[3], at[111]);    MULADD(at[4], at[110]);    MULADD(at[5], at[109]);    MULADD(at[6], at[108]);    MULADD(at[7], at[107]);    MULADD(at[8], at[106]);    MULADD(at[9], at[105]);    MULADD(at[10], at[104]);    MULADD(at[11], at[103]);    MULADD(at[12], at[102]);    MULADD(at[13], at[101]);    MULADD(at[14], at[100]);    MULADD(at[15], at[99]);    MULADD(at[16], at[98]);    MULADD(at[17], at[97]);    MULADD(at[18], at[96]);    MULADD(at[19], at[95]);    MULADD(at[20], at[94]);    MULADD(at[21], at[93]);    MULADD(at[22], at[92]);    MULADD(at[23], at[91]);    MULADD(at[24], at[90]);    MULADD(at[25], at[89]);    MULADD(at[26], at[88]);    MULADD(at[27], at[87]);    MULADD(at[28], at[86]);    MULADD(at[29], at[85]);    MULADD(at[30], at[84]);    MULADD(at[31], at[83]);    MULADD(at[32], at[82]);    MULADD(at[33], at[81]);    MULADD(at[34], at[80]);    MULADD(at[35], at[79]);    MULADD(at[36], at[78]);    MULADD(at[37], at[77]);    MULADD(at[38], at[76]);    MULADD(at[39], at[75]);    MULADD(at[40], at[74]);    MULADD(at[41], at[73]);    MULADD(at[42], at[72]);    MULADD(at[43], at[71]);    MULADD(at[44], at[70]);    MULADD(at[45], at[69]);    MULADD(at[46], at[68]);    MULADD(at[47], at[67]);    MULADD(at[48], at[66]);    MULADD(at[49], at[65]);    MULADD(at[50], at[64]); 
   COMBA_STORE(C->dp[50]);
   /* 51 */
   COMBA_FORWARD;
   MULADD(at[0], at[115]);    MULADD(at[1], at[114]);    MULADD(at[2], at[113]);    MULADD(at[3], at[112]);    MULADD(at[4], at[111]);    MULADD(at[5], at[110]);    MULADD(at[6], at[109]);    MULADD(at[7], at[108]);    MULADD(at[8], at[107]);    MULADD(at[9], at[106]);    MULADD(at[10], at[105]);    MULADD(at[11], at[104]);    MULADD(at[12], at[103]);    MULADD(at[13], at[102]);    MULADD(at[14], at[101]);    MULADD(at[15], at[100]);    MULADD(at[16], at[99]);    MULADD(at[17], at[98]);    MULADD(at[18], at[97]);    MULADD(at[19], at[96]);    MULADD(at[20], at[95]);    MULADD(at[21], at[94]);    MULADD(at[22], at[93]);    MULADD(at[23], at[92]);    MULADD(at[24], at[91]);    MULADD(at[25], at[90]);    MULADD(at[26], at[89]);    MULADD(at[27], at[88]);    MULADD(at[28], at[87]);    MULADD(at[29], at[86]);    MULADD(at[30], at[85]);    MULADD(at[31], at[84]);    MULADD(at[32], at[83]);    MULADD(at[33], at[82]);    MULADD(at[34], at[81]);    MULADD(at[35], at[80]);    MULADD(at[36], at[79]);    MULADD(at[37], at[78]);    MULADD(at[38], at[77]);    MULADD(at[39], at[76]);    MULADD(at[40], at[75]);    MULADD(at[41], at[74]);    MULADD(at[42], at[73]);    MULADD(at[43], at[72]);    MULADD(at[44], at[71]);    MULADD(at[45], at[70]);    MULADD(at[46], at[69]);    MULADD(at[47], at[68]);    MULADD(at[48], at[67]);    MULADD(at[49], at[66]);    MULADD(at[50], at[65]);    MULADD(at[51], at[64]); 
   COMBA_STORE(C->dp[51]);
   /* 52 */
   COMBA_FORWARD;
   MULADD(at[0], at[116]);    MULADD(at[1], at[115]);    MULADD(at[2], at[114]);    MULADD(at[3], at[113]);    MULADD(at[4], at[112]);    MULADD(at[5], at[111]);    MULADD(at[6], at[110]);    MULADD(at[7], at[109]);    MULADD(at[8], at[108]);    MULADD(at[9], at[107]);    MULADD(at[10], at[106]);    MULADD(at[11], at[105]);    MULADD(at[12], at[104]);    MULADD(at[13], at[103]);    MULADD(at[14], at[102]);    MULADD(at[15], at[101]);    MULADD(at[16], at[100]);    MULADD(at[17], at[99]);    MULADD(at[18], at[98]);    MULADD(at[19], at[97]);    MULADD(at[20], at[96]);    MULADD(at[21], at[95]);    MULADD(at[22], at[94]);    MULADD(at[23], at[93]);    MULADD(at[24], at[92]);    MULADD(at[25], at[91]);    MULADD(at[26], at[90]);    MULADD(at[27], at[89]);    MULADD(at[28], at[88]);    MULADD(at[29], at[87]);    MULADD(at[30], at[86]);    MULADD(at[31], at[85]);    MULADD(at[32], at[84]);    MULADD(at[33], at[83]);    MULADD(at[34], at[82]);    MULADD(at[35], at[81]);    MULADD(at[36], at[80]);    MULADD(at[37], at[79]);    MULADD(at[38], at[78]);    MULADD(at[39], at[77]);    MULADD(at[40], at[76]);    MULADD(at[41], at[75]);    MULADD(at[42], at[74]);    MULADD(at[43], at[73]);    MULADD(at[44], at[72]);    MULADD(at[45], at[71]);    MULADD(at[46], at[70]);    MULADD(at[47], at[69]);    MULADD(at[48], at[68]);    MULADD(at[49], at[67]);    MULADD(at[50], at[66]);    MULADD(at[51], at[65]);    MULADD(at[52], at[64]); 
   COMBA_STORE(C->dp[52]);
   /* 53 */
   COMBA_FORWARD;
   MULADD(at[0], at[117]);    MULADD(at[1], at[116]);    MULADD(at[2], at[115]);    MULADD(at[3], at[114]);    MULADD(at[4], at[113]);    MULADD(at[5], at[112]);    MULADD(at[6], at[111]);    MULADD(at[7], at[110]);    MULADD(at[8], at[109]);    MULADD(at[9], at[108]);    MULADD(at[10], at[107]);    MULADD(at[11], at[106]);    MULADD(at[12], at[105]);    MULADD(at[13], at[104]);    MULADD(at[14], at[103]);    MULADD(at[15], at[102]);    MULADD(at[16], at[101]);    MULADD(at[17], at[100]);    MULADD(at[18], at[99]);    MULADD(at[19], at[98]);    MULADD(at[20], at[97]);    MULADD(at[21], at[96]);    MULADD(at[22], at[95]);    MULADD(at[23], at[94]);    MULADD(at[24], at[93]);    MULADD(at[25], at[92]);    MULADD(at[26], at[91]);    MULADD(at[27], at[90]);    MULADD(at[28], at[89]);    MULADD(at[29], at[88]);    MULADD(at[30], at[87]);    MULADD(at[31], at[86]);    MULADD(at[32], at[85]);    MULADD(at[33], at[84]);    MULADD(at[34], at[83]);    MULADD(at[35], at[82]);    MULADD(at[36], at[81]);    MULADD(at[37], at[80]);    MULADD(at[38], at[79]);    MULADD(at[39], at[78]);    MULADD(at[40], at[77]);    MULADD(at[41], at[76]);    MULADD(at[42], at[75]);    MULADD(at[43], at[74]);    MULADD(at[44], at[73]);    MULADD(at[45], at[72]);    MULADD(at[46], at[71]);    MULADD(at[47], at[70]);    MULADD(at[48], at[69]);    MULADD(at[49], at[68]);    MULADD(at[50], at[67]);    MULADD(at[51], at[66]);    MULADD(at[52], at[65]);    MULADD(at[53], at[64]); 
   COMBA_STORE(C->dp[53]);
   /* 54 */
   COMBA_FORWARD;
   MULADD(at[0], at[118]);    MULADD(at[1], at[117]);    MULADD(at[2], at[116]);    MULADD(at[3], at[115]);    MULADD(at[4], at[114]);    MULADD(at[5], at[113]);    MULADD(at[6], at[112]);    MULADD(at[7], at[111]);    MULADD(at[8], at[110]);    MULADD(at[9], at[109]);    MULADD(at[10], at[108]);    MULADD(at[11], at[107]);    MULADD(at[12], at[106]);    MULADD(at[13], at[105]);    MULADD(at[14], at[104]);    MULADD(at[15], at[103]);    MULADD(at[16], at[102]);    MULADD(at[17], at[101]);    MULADD(at[18], at[100]);    MULADD(at[19], at[99]);    MULADD(at[20], at[98]);    MULADD(at[21], at[97]);    MULADD(at[22], at[96]);    MULADD(at[23], at[95]);    MULADD(at[24], at[94]);    MULADD(at[25], at[93]);    MULADD(at[26], at[92]);    MULADD(at[27], at[91]);    MULADD(at[28], at[90]);    MULADD(at[29], at[89]);    MULADD(at[30], at[88]);    MULADD(at[31], at[87]);    MULADD(at[32], at[86]);    MULADD(at[33], at[85]);    MULADD(at[34], at[84]);    MULADD(at[35], at[83]);    MULADD(at[36], at[82]);    MULADD(at[37], at[81]);    MULADD(at[38], at[80]);    MULADD(at[39], at[79]);    MULADD(at[40], at[78]);    MULADD(at[41], at[77]);    MULADD(at[42], at[76]);    MULADD(at[43], at[75]);    MULADD(at[44], at[74]);    MULADD(at[45], at[73]);    MULADD(at[46], at[72]);    MULADD(at[47], at[71]);    MULADD(at[48], at[70]);    MULADD(at[49], at[69]);    MULADD(at[50], at[68]);    MULADD(at[51], at[67]);    MULADD(at[52], at[66]);    MULADD(at[53], at[65]);    MULADD(at[54], at[64]); 
   COMBA_STORE(C->dp[54]);
   /* 55 */
   COMBA_FORWARD;
   MULADD(at[0], at[119]);    MULADD(at[1], at[118]);    MULADD(at[2], at[117]);    MULADD(at[3], at[116]);    MULADD(at[4], at[115]);    MULADD(at[5], at[114]);    MULADD(at[6], at[113]);    MULADD(at[7], at[112]);    MULADD(at[8], at[111]);    MULADD(at[9], at[110]);    MULADD(at[10], at[109]);    MULADD(at[11], at[108]);    MULADD(at[12], at[107]);    MULADD(at[13], at[106]);    MULADD(at[14], at[105]);    MULADD(at[15], at[104]);    MULADD(at[16], at[103]);    MULADD(at[17], at[102]);    MULADD(at[18], at[101]);    MULADD(at[19], at[100]);    MULADD(at[20], at[99]);    MULADD(at[21], at[98]);    MULADD(at[22], at[97]);    MULADD(at[23], at[96]);    MULADD(at[24], at[95]);    MULADD(at[25], at[94]);    MULADD(at[26], at[93]);    MULADD(at[27], at[92]);    MULADD(at[28], at[91]);    MULADD(at[29], at[90]);    MULADD(at[30], at[89]);    MULADD(at[31], at[88]);    MULADD(at[32], at[87]);    MULADD(at[33], at[86]);    MULADD(at[34], at[85]);    MULADD(at[35], at[84]);    MULADD(at[36], at[83]);    MULADD(at[37], at[82]);    MULADD(at[38], at[81]);    MULADD(at[39], at[80]);    MULADD(at[40], at[79]);    MULADD(at[41], at[78]);    MULADD(at[42], at[77]);    MULADD(at[43], at[76]);    MULADD(at[44], at[75]);    MULADD(at[45], at[74]);    MULADD(at[46], at[73]);    MULADD(at[47], at[72]);    MULADD(at[48], at[71]);    MULADD(at[49], at[70]);    MULADD(at[50], at[69]);    MULADD(at[51], at[68]);    MULADD(at[52], at[67]);    MULADD(at[53], at[66]);    MULADD(at[54], at[65]);    MULADD(at[55], at[64]); 
   COMBA_STORE(C->dp[55]);
   /* 56 */
   COMBA_FORWARD;
   MULADD(at[0], at[120]);    MULADD(at[1], at[119]);    MULADD(at[2], at[118]);    MULADD(at[3], at[117]);    MULADD(at[4], at[116]);    MULADD(at[5], at[115]);    MULADD(at[6], at[114]);    MULADD(at[7], at[113]);    MULADD(at[8], at[112]);    MULADD(at[9], at[111]);    MULADD(at[10], at[110]);    MULADD(at[11], at[109]);    MULADD(at[12], at[108]);    MULADD(at[13], at[107]);    MULADD(at[14], at[106]);    MULADD(at[15], at[105]);    MULADD(at[16], at[104]);    MULADD(at[17], at[103]);    MULADD(at[18], at[102]);    MULADD(at[19], at[101]);    MULADD(at[20], at[100]);    MULADD(at[21], at[99]);    MULADD(at[22], at[98]);    MULADD(at[23], at[97]);    MULADD(at[24], at[96]);    MULADD(at[25], at[95]);    MULADD(at[26], at[94]);    MULADD(at[27], at[93]);    MULADD(at[28], at[92]);    MULADD(at[29], at[91]);    MULADD(at[30], at[90]);    MULADD(at[31], at[89]);    MULADD(at[32], at[88]);    MULADD(at[33], at[87]);    MULADD(at[34], at[86]);    MULADD(at[35], at[85]);    MULADD(at[36], at[84]);    MULADD(at[37], at[83]);    MULADD(at[38], at[82]);    MULADD(at[39], at[81]);    MULADD(at[40], at[80]);    MULADD(at[41], at[79]);    MULADD(at[42], at[78]);    MULADD(at[43], at[77]);    MULADD(at[44], at[76]);    MULADD(at[45], at[75]);    MULADD(at[46], at[74]);    MULADD(at[47], at[73]);    MULADD(at[48], at[72]);    MULADD(at[49], at[71]);    MULADD(at[50], at[70]);    MULADD(at[51], at[69]);    MULADD(at[52], at[68]);    MULADD(at[53], at[67]);    MULADD(at[54], at[66]);    MULADD(at[55], at[65]);    MULADD(at[56], at[64]); 
   COMBA_STORE(C->dp[56]);
   /* 57 */
   COMBA_FORWARD;
   MULADD(at[0], at[121]);    MULADD(at[1], at[120]);    MULADD(at[2], at[119]);    MULADD(at[3], at[118]);    MULADD(at[4], at[117]);    MULADD(at[5], at[116]);    MULADD(at[6], at[115]);    MULADD(at[7], at[114]);    MULADD(at[8], at[113]);    MULADD(at[9], at[112]);    MULADD(at[10], at[111]);    MULADD(at[11], at[110]);    MULADD(at[12], at[109]);    MULADD(at[13], at[108]);    MULADD(at[14], at[107]);    MULADD(at[15], at[106]);    MULADD(at[16], at[105]);    MULADD(at[17], at[104]);    MULADD(at[18], at[103]);    MULADD(at[19], at[102]);    MULADD(at[20], at[101]);    MULADD(at[21], at[100]);    MULADD(at[22], at[99]);    MULADD(at[23], at[98]);    MULADD(at[24], at[97]);    MULADD(at[25], at[96]);    MULADD(at[26], at[95]);    MULADD(at[27], at[94]);    MULADD(at[28], at[93]);    MULADD(at[29], at[92]);    MULADD(at[30], at[91]);    MULADD(at[31], at[90]);    MULADD(at[32], at[89]);    MULADD(at[33], at[88]);    MULADD(at[34], at[87]);    MULADD(at[35], at[86]);    MULADD(at[36], at[85]);    MULADD(at[37], at[84]);    MULADD(at[38], at[83]);    MULADD(at[39], at[82]);    MULADD(at[40], at[81]);    MULADD(at[41], at[80]);    MULADD(at[42], at[79]);    MULADD(at[43], at[78]);    MULADD(at[44], at[77]);    MULADD(at[45], at[76]);    MULADD(at[46], at[75]);    MULADD(at[47], at[74]);    MULADD(at[48], at[73]);    MULADD(at[49], at[72]);    MULADD(at[50], at[71]);    MULADD(at[51], at[70]);    MULADD(at[52], at[69]);    MULADD(at[53], at[68]);    MULADD(at[54], at[67]);    MULADD(at[55], at[66]);    MULADD(at[56], at[65]);    MULADD(at[57], at[64]); 
   COMBA_STORE(C->dp[57]);
   /* 58 */
   COMBA_FORWARD;
   MULADD(at[0], at[122]);    MULADD(at[1], at[121]);    MULADD(at[2], at[120]);    MULADD(at[3], at[119]);    MULADD(at[4], at[118]);    MULADD(at[5], at[117]);    MULADD(at[6], at[116]);    MULADD(at[7], at[115]);    MULADD(at[8], at[114]);    MULADD(at[9], at[113]);    MULADD(at[10], at[112]);    MULADD(at[11], at[111]);    MULADD(at[12], at[110]);    MULADD(at[13], at[109]);    MULADD(at[14], at[108]);    MULADD(at[15], at[107]);    MULADD(at[16], at[106]);    MULADD(at[17], at[105]);    MULADD(at[18], at[104]);    MULADD(at[19], at[103]);    MULADD(at[20], at[102]);    MULADD(at[21], at[101]);    MULADD(at[22], at[100]);    MULADD(at[23], at[99]);    MULADD(at[24], at[98]);    MULADD(at[25], at[97]);    MULADD(at[26], at[96]);    MULADD(at[27], at[95]);    MULADD(at[28], at[94]);    MULADD(at[29], at[93]);    MULADD(at[30], at[92]);    MULADD(at[31], at[91]);    MULADD(at[32], at[90]);    MULADD(at[33], at[89]);    MULADD(at[34], at[88]);    MULADD(at[35], at[87]);    MULADD(at[36], at[86]);    MULADD(at[37], at[85]);    MULADD(at[38], at[84]);    MULADD(at[39], at[83]);    MULADD(at[40], at[82]);    MULADD(at[41], at[81]);    MULADD(at[42], at[80]);    MULADD(at[43], at[79]);    MULADD(at[44], at[78]);    MULADD(at[45], at[77]);    MULADD(at[46], at[76]);    MULADD(at[47], at[75]);    MULADD(at[48], at[74]);    MULADD(at[49], at[73]);    MULADD(at[50], at[72]);    MULADD(at[51], at[71]);    MULADD(at[52], at[70]);    MULADD(at[53], at[69]);    MULADD(at[54], at[68]);    MULADD(at[55], at[67]);    MULADD(at[56], at[66]);    MULADD(at[57], at[65]);    MULADD(at[58], at[64]); 
   COMBA_STORE(C->dp[58]);
   /* 59 */
   COMBA_FORWARD;
   MULADD(at[0], at[123]);    MULADD(at[1], at[122]);    MULADD(at[2], at[121]);    MULADD(at[3], at[120]);    MULADD(at[4], at[119]);    MULADD(at[5], at[118]);    MULADD(at[6], at[117]);    MULADD(at[7], at[116]);    MULADD(at[8], at[115]);    MULADD(at[9], at[114]);    MULADD(at[10], at[113]);    MULADD(at[11], at[112]);    MULADD(at[12], at[111]);    MULADD(at[13], at[110]);    MULADD(at[14], at[109]);    MULADD(at[15], at[108]);    MULADD(at[16], at[107]);    MULADD(at[17], at[106]);    MULADD(at[18], at[105]);    MULADD(at[19], at[104]);    MULADD(at[20], at[103]);    MULADD(at[21], at[102]);    MULADD(at[22], at[101]);    MULADD(at[23], at[100]);    MULADD(at[24], at[99]);    MULADD(at[25], at[98]);    MULADD(at[26], at[97]);    MULADD(at[27], at[96]);    MULADD(at[28], at[95]);    MULADD(at[29], at[94]);    MULADD(at[30], at[93]);    MULADD(at[31], at[92]);    MULADD(at[32], at[91]);    MULADD(at[33], at[90]);    MULADD(at[34], at[89]);    MULADD(at[35], at[88]);    MULADD(at[36], at[87]);    MULADD(at[37], at[86]);    MULADD(at[38], at[85]);    MULADD(at[39], at[84]);    MULADD(at[40], at[83]);    MULADD(at[41], at[82]);    MULADD(at[42], at[81]);    MULADD(at[43], at[80]);    MULADD(at[44], at[79]);    MULADD(at[45], at[78]);    MULADD(at[46], at[77]);    MULADD(at[47], at[76]);    MULADD(at[48], at[75]);    MULADD(at[49], at[74]);    MULADD(at[50], at[73]);    MULADD(at[51], at[72]);    MULADD(at[52], at[71]);    MULADD(at[53], at[70]);    MULADD(at[54], at[69]);    MULADD(at[55], at[68]);    MULADD(at[56], at[67]);    MULADD(at[57], at[66]);    MULADD(at[58], at[65]);    MULADD(at[59], at[64]); 
   COMBA_STORE(C->dp[59]);
   /* 60 */
   COMBA_FORWARD;
   MULADD(at[0], at[124]);    MULADD(at[1], at[123]);    MULADD(at[2], at[122]);    MULADD(at[3], at[121]);    MULADD(at[4], at[120]);    MULADD(at[5], at[119]);    MULADD(at[6], at[118]);    MULADD(at[7], at[117]);    MULADD(at[8], at[116]);    MULADD(at[9], at[115]);    MULADD(at[10], at[114]);    MULADD(at[11], at[113]);    MULADD(at[12], at[112]);    MULADD(at[13], at[111]);    MULADD(at[14], at[110]);    MULADD(at[15], at[109]);    MULADD(at[16], at[108]);    MULADD(at[17], at[107]);    MULADD(at[18], at[106]);    MULADD(at[19], at[105]);    MULADD(at[20], at[104]);    MULADD(at[21], at[103]);    MULADD(at[22], at[102]);    MULADD(at[23], at[101]);    MULADD(at[24], at[100]);    MULADD(at[25], at[99]);    MULADD(at[26], at[98]);    MULADD(at[27], at[97]);    MULADD(at[28], at[96]);    MULADD(at[29], at[95]);    MULADD(at[30], at[94]);    MULADD(at[31], at[93]);    MULADD(at[32], at[92]);    MULADD(at[33], at[91]);    MULADD(at[34], at[90]);    MULADD(at[35], at[89]);    MULADD(at[36], at[88]);    MULADD(at[37], at[87]);    MULADD(at[38], at[86]);    MULADD(at[39], at[85]);    MULADD(at[40], at[84]);    MULADD(at[41], at[83]);    MULADD(at[42], at[82]);    MULADD(at[43], at[81]);    MULADD(at[44], at[80]);    MULADD(at[45], at[79]);    MULADD(at[46], at[78]);    MULADD(at[47], at[77]);    MULADD(at[48], at[76]);    MULADD(at[49], at[75]);    MULADD(at[50], at[74]);    MULADD(at[51], at[73]);    MULADD(at[52], at[72]);    MULADD(at[53], at[71]);    MULADD(at[54], at[70]);    MULADD(at[55], at[69]);    MULADD(at[56], at[68]);    MULADD(at[57], at[67]);    MULADD(at[58], at[66]);    MULADD(at[59], at[65]);    MULADD(at[60], at[64]); 
   COMBA_STORE(C->dp[60]);
   /* 61 */
   COMBA_FORWARD;
   MULADD(at[0], at[125]);    MULADD(at[1], at[124]);    MULADD(at[2], at[123]);    MULADD(at[3], at[122]);    MULADD(at[4], at[121]);    MULADD(at[5], at[120]);    MULADD(at[6], at[119]);    MULADD(at[7], at[118]);    MULADD(at[8], at[117]);    MULADD(at[9], at[116]);    MULADD(at[10], at[115]);    MULADD(at[11], at[114]);    MULADD(at[12], at[113]);    MULADD(at[13], at[112]);    MULADD(at[14], at[111]);    MULADD(at[15], at[110]);    MULADD(at[16], at[109]);    MULADD(at[17], at[108]);    MULADD(at[18], at[107]);    MULADD(at[19], at[106]);    MULADD(at[20], at[105]);    MULADD(at[21], at[104]);    MULADD(at[22], at[103]);    MULADD(at[23], at[102]);    MULADD(at[24], at[101]);    MULADD(at[25], at[100]);    MULADD(at[26], at[99]);    MULADD(at[27], at[98]);    MULADD(at[28], at[97]);    MULADD(at[29], at[96]);    MULADD(at[30], at[95]);    MULADD(at[31], at[94]);    MULADD(at[32], at[93]);    MULADD(at[33], at[92]);    MULADD(at[34], at[91]);    MULADD(at[35], at[90]);    MULADD(at[36], at[89]);    MULADD(at[37], at[88]);    MULADD(at[38], at[87]);    MULADD(at[39], at[86]);    MULADD(at[40], at[85]);    MULADD(at[41], at[84]);    MULADD(at[42], at[83]);    MULADD(at[43], at[82]);    MULADD(at[44], at[81]);    MULADD(at[45], at[80]);    MULADD(at[46], at[79]);    MULADD(at[47], at[78]);    MULADD(at[48], at[77]);    MULADD(at[49], at[76]);    MULADD(at[50], at[75]);    MULADD(at[51], at[74]);    MULADD(at[52], at[73]);    MULADD(at[53], at[72]);    MULADD(at[54], at[71]);    MULADD(at[55], at[70]);    MULADD(at[56], at[69]);    MULADD(at[57], at[68]);    MULADD(at[58], at[67]);    MULADD(at[59], at[66]);    MULADD(at[60], at[65]);    MULADD(at[61], at[64]); 
   COMBA_STORE(C->dp[61]);
   /* 62 */
   COMBA_FORWARD;
   MULADD(at[0], at[126]);    MULADD(at[1], at[125]);    MULADD(at[2], at[124]);    MULADD(at[3], at[123]);    MULADD(at[4], at[122]);    MULADD(at[5], at[121]);    MULADD(at[6], at[120]);    MULADD(at[7], at[119]);    MULADD(at[8], at[118]);    MULADD(at[9], at[117]);    MULADD(at[10], at[116]);    MULADD(at[11], at[115]);    MULADD(at[12], at[114]);    MULADD(at[13], at[113]);    MULADD(at[14], at[112]);    MULADD(at[15], at[111]);    MULADD(at[16], at[110]);    MULADD(at[17], at[109]);    MULADD(at[18], at[108]);    MULADD(at[19], at[107]);    MULADD(at[20], at[106]);    MULADD(at[21], at[105]);    MULADD(at[22], at[104]);    MULADD(at[23], at[103]);    MULADD(at[24], at[102]);    MULADD(at[25], at[101]);    MULADD(at[26], at[100]);    MULADD(at[27], at[99]);    MULADD(at[28], at[98]);    MULADD(at[29], at[97]);    MULADD(at[30], at[96]);    MULADD(at[31], at[95]);    MULADD(at[32], at[94]);    MULADD(at[33], at[93]);    MULADD(at[34], at[92]);    MULADD(at[35], at[91]);    MULADD(at[36], at[90]);    MULADD(at[37], at[89]);    MULADD(at[38], at[88]);    MULADD(at[39], at[87]);    MULADD(at[40], at[86]);    MULADD(at[41], at[85]);    MULADD(at[42], at[84]);    MULADD(at[43], at[83]);    MULADD(at[44], at[82]);    MULADD(at[45], at[81]);    MULADD(at[46], at[80]);    MULADD(at[47], at[79]);    MULADD(at[48], at[78]);    MULADD(at[49], at[77]);    MULADD(at[50], at[76]);    MULADD(at[51], at[75]);    MULADD(at[52], at[74]);    MULADD(at[53], at[73]);    MULADD(at[54], at[72]);    MULADD(at[55], at[71]);    MULADD(at[56], at[70]);    MULADD(at[57], at[69]);    MULADD(at[58], at[68]);    MULADD(at[59], at[67]);    MULADD(at[60], at[66]);    MULADD(at[61], at[65]);    MULADD(at[62], at[64]); 
   COMBA_STORE(C->dp[62]);
   /* 63 */
   COMBA_FORWARD;
   MULADD(at[0], at[127]);    MULADD(at[1], at[126]);    MULADD(at[2], at[125]);    MULADD(at[3], at[124]);    MULADD(at[4], at[123]);    MULADD(at[5], at[122]);    MULADD(at[6], at[121]);    MULADD(at[7], at[120]);    MULADD(at[8], at[119]);    MULADD(at[9], at[118]);    MULADD(at[10], at[117]);    MULADD(at[11], at[116]);    MULADD(at[12], at[115]);    MULADD(at[13], at[114]);    MULADD(at[14], at[113]);    MULADD(at[15], at[112]);    MULADD(at[16], at[111]);    MULADD(at[17], at[110]);    MULADD(at[18], at[109]);    MULADD(at[19], at[108]);    MULADD(at[20], at[107]);    MULADD(at[21], at[106]);    MULADD(at[22], at[105]);    MULADD(at[23], at[104]);    MULADD(at[24], at[103]);    MULADD(at[25], at[102]);    MULADD(at[26], at[101]);    MULADD(at[27], at[100]);    MULADD(at[28], at[99]);    MULADD(at[29], at[98]);    MULADD(at[30], at[97]);    MULADD(at[31], at[96]);    MULADD(at[32], at[95]);    MULADD(at[33], at[94]);    MULADD(at[34], at[93]);    MULADD(at[35], at[92]);    MULADD(at[36], at[91]);    MULADD(at[37], at[90]);    MULADD(at[38], at[89]);    MULADD(at[39], at[88]);    MULADD(at[40], at[87]);    MULADD(at[41], at[86]);    MULADD(at[42], at[85]);    MULADD(at[43], at[84]);    MULADD(at[44], at[83]);    MULADD(at[45], at[82]);    MULADD(at[46], at[81]);    MULADD(at[47], at[80]);    MULADD(at[48], at[79]);    MULADD(at[49], at[78]);    MULADD(at[50], at[77]);    MULADD(at[51], at[76]);    MULADD(at[52], at[75]);    MULADD(at[53], at[74]);    MULADD(at[54], at[73]);    MULADD(at[55], at[72]);    MULADD(at[56], at[71]);    MULADD(at[57], at[70]);    MULADD(at[58], at[69]);    MULADD(at[59], at[68]);    MULADD(at[60], at[67]);    MULADD(at[61], at[66]);    MULADD(at[62], at[65]);    MULADD(at[63], at[64]); 
   COMBA_STORE(C->dp[63]);
   /* 64 */
   COMBA_FORWARD;
   MULADD(at[1], at[127]);    MULADD(at[2], at[126]);    MULADD(at[3], at[125]);    MULADD(at[4], at[124]);    MULADD(at[5], at[123]);    MULADD(at[6], at[122]);    MULADD(at[7], at[121]);    MULADD(at[8], at[120]);    MULADD(at[9], at[119]);    MULADD(at[10], at[118]);    MULADD(at[11], at[117]);    MULADD(at[12], at[116]);    MULADD(at[13], at[115]);    MULADD(at[14], at[114]);    MULADD(at[15], at[113]);    MULADD(at[16], at[112]);    MULADD(at[17], at[111]);    MULADD(at[18], at[110]);    MULADD(at[19], at[109]);    MULADD(at[20], at[108]);    MULADD(at[21], at[107]);    MULADD(at[22], at[106]);    MULADD(at[23], at[105]);    MULADD(at[24], at[104]);    MULADD(at[25], at[103]);    MULADD(at[26], at[102]);    MULADD(at[27], at[101]);    MULADD(at[28], at[100]);    MULADD(at[29], at[99]);    MULADD(at[30], at[98]);    MULADD(at[31], at[97]);    MULADD(at[32], at[96]);    MULADD(at[33], at[95]);    MULADD(at[34], at[94]);    MULADD(at[35], at[93]);    MULADD(at[36], at[92]);    MULADD(at[37], at[91]);    MULADD(at[38], at[90]);    MULADD(at[39], at[89]);    MULADD(at[40], at[88]);    MULADD(at[41], at[87]);    MULADD(at[42], at[86]);    MULADD(at[43], at[85]);    MULADD(at[44], at[84]);    MULADD(at[45], at[83]);    MULADD(at[46], at[82]);    MULADD(at[47], at[81]);    MULADD(at[48], at[80]);    MULADD(at[49], at[79]);    MULADD(at[50], at[78]);    MULADD(at[51], at[77]);    MULADD(at[52], at[76]);    MULADD(at[53], at[75]);    MULADD(at[54], at[74]);    MULADD(at[55], at[73]);    MULADD(at[56], at[72]);    MULADD(at[57], at[71]);    MULADD(at[58], at[70]);    MULADD(at[59], at[69]);    MULADD(at[60], at[68]);    MULADD(at[61], at[67]);    MULADD(at[62], at[66]);    MULADD(at[63], at[65]); 
   COMBA_STORE(C->dp[64]);
   /* 65 */
   COMBA_FORWARD;
   MULADD(at[2], at[127]);    MULADD(at[3], at[126]);    MULADD(at[4], at[125]);    MULADD(at[5], at[124]);    MULADD(at[6], at[123]);    MULADD(at[7], at[122]);    MULADD(at[8], at[121]);    MULADD(at[9], at[120]);    MULADD(at[10], at[119]);    MULADD(at[11], at[118]);    MULADD(at[12], at[117]);    MULADD(at[13], at[116]);    MULADD(at[14], at[115]);    MULADD(at[15], at[114]);    MULADD(at[16], at[113]);    MULADD(at[17], at[112]);    MULADD(at[18], at[111]);    MULADD(at[19], at[110]);    MULADD(at[20], at[109]);    MULADD(at[21], at[108]);    MULADD(at[22], at[107]);    MULADD(at[23], at[106]);    MULADD(at[24], at[105]);    MULADD(at[25], at[104]);    MULADD(at[26], at[103]);    MULADD(at[27], at[102]);    MULADD(at[28], at[101]);    MULADD(at[29], at[100]);    MULADD(at[30], at[99]);    MULADD(at[31], at[98]);    MULADD(at[32], at[97]);    MULADD(at[33], at[96]);    MULADD(at[34], at[95]);    MULADD(at[35], at[94]);    MULADD(at[36], at[93]);    MULADD(at[37], at[92]);    MULADD(at[38], at[91]);    MULADD(at[39], at[90]);    MULADD(at[40], at[89]);    MULADD(at[41], at[88]);    MULADD(at[42], at[87]);    MULADD(at[43], at[86]);    MULADD(at[44], at[85]);    MULADD(at[45], at[84]);    MULADD(at[46], at[83]);    MULADD(at[47], at[82]);    MULADD(at[48], at[81]);    MULADD(at[49], at[80]);    MULADD(at[50], at[79]);    MULADD(at[51], at[78]);    MULADD(at[52], at[77]);    MULADD(at[53], at[76]);    MULADD(at[54], at[75]);    MULADD(at[55], at[74]);    MULADD(at[56], at[73]);    MULADD(at[57], at[72]);    MULADD(at[58], at[71]);    MULADD(at[59], at[70]);    MULADD(at[60], at[69]);    MULADD(at[61], at[68]);    MULADD(at[62], at[67]);    MULADD(at[63], at[66]); 
   COMBA_STORE(C->dp[65]);
   /* 66 */
   COMBA_FORWARD;
   MULADD(at[3], at[127]);    MULADD(at[4], at[126]);    MULADD(at[5], at[125]);    MULADD(at[6], at[124]);    MULADD(at[7], at[123]);    MULADD(at[8], at[122]);    MULADD(at[9], at[121]);    MULADD(at[10], at[120]);    MULADD(at[11], at[119]);    MULADD(at[12], at[118]);    MULADD(at[13], at[117]);    MULADD(at[14], at[116]);    MULADD(at[15], at[115]);    MULADD(at[16], at[114]);    MULADD(at[17], at[113]);    MULADD(at[18], at[112]);    MULADD(at[19], at[111]);    MULADD(at[20], at[110]);    MULADD(at[21], at[109]);    MULADD(at[22], at[108]);    MULADD(at[23], at[107]);    MULADD(at[24], at[106]);    MULADD(at[25], at[105]);    MULADD(at[26], at[104]);    MULADD(at[27], at[103]);    MULADD(at[28], at[102]);    MULADD(at[29], at[101]);    MULADD(at[30], at[100]);    MULADD(at[31], at[99]);    MULADD(at[32], at[98]);    MULADD(at[33], at[97]);    MULADD(at[34], at[96]);    MULADD(at[35], at[95]);    MULADD(at[36], at[94]);    MULADD(at[37], at[93]);    MULADD(at[38], at[92]);    MULADD(at[39], at[91]);    MULADD(at[40], at[90]);    MULADD(at[41], at[89]);    MULADD(at[42], at[88]);    MULADD(at[43], at[87]);    MULADD(at[44], at[86]);    MULADD(at[45], at[85]);    MULADD(at[46], at[84]);    MULADD(at[47], at[83]);    MULADD(at[48], at[82]);    MULADD(at[49], at[81]);    MULADD(at[50], at[80]);    MULADD(at[51], at[79]);    MULADD(at[52], at[78]);    MULADD(at[53], at[77]);    MULADD(at[54], at[76]);    MULADD(at[55], at[75]);    MULADD(at[56], at[74]);    MULADD(at[57], at[73]);    MULADD(at[58], at[72]);    MULADD(at[59], at[71]);    MULADD(at[60], at[70]);    MULADD(at[61], at[69]);    MULADD(at[62], at[68]);    MULADD(at[63], at[67]); 
   COMBA_STORE(C->dp[66]);
   /* 67 */
   COMBA_FORWARD;
   MULADD(at[4], at[127]);    MULADD(at[5], at[126]);    MULADD(at[6], at[125]);    MULADD(at[7], at[124]);    MULADD(at[8], at[123]);    MULADD(at[9], at[122]);    MULADD(at[10], at[121]);    MULADD(at[11], at[120]);    MULADD(at[12], at[119]);    MULADD(at[13], at[118]);    MULADD(at[14], at[117]);    MULADD(at[15], at[116]);    MULADD(at[16], at[115]);    MULADD(at[17], at[114]);    MULADD(at[18], at[113]);    MULADD(at[19], at[112]);    MULADD(at[20], at[111]);    MULADD(at[21], at[110]);    MULADD(at[22], at[109]);    MULADD(at[23], at[108]);    MULADD(at[24], at[107]);    MULADD(at[25], at[106]);    MULADD(at[26], at[105]);    MULADD(at[27], at[104]);    MULADD(at[28], at[103]);    MULADD(at[29], at[102]);    MULADD(at[30], at[101]);    MULADD(at[31], at[100]);    MULADD(at[32], at[99]);    MULADD(at[33], at[98]);    MULADD(at[34], at[97]);    MULADD(at[35], at[96]);    MULADD(at[36], at[95]);    MULADD(at[37], at[94]);    MULADD(at[38], at[93]);    MULADD(at[39], at[92]);    MULADD(at[40], at[91]);    MULADD(at[41], at[90]);    MULADD(at[42], at[89]);    MULADD(at[43], at[88]);    MULADD(at[44], at[87]);    MULADD(at[45], at[86]);    MULADD(at[46], at[85]);    MULADD(at[47], at[84]);    MULADD(at[48], at[83]);    MULADD(at[49], at[82]);    MULADD(at[50], at[81]);    MULADD(at[51], at[80]);    MULADD(at[52], at[79]);    MULADD(at[53], at[78]);    MULADD(at[54], at[77]);    MULADD(at[55], at[76]);    MULADD(at[56], at[75]);    MULADD(at[57], at[74]);    MULADD(at[58], at[73]);    MULADD(at[59], at[72]);    MULADD(at[60], at[71]);    MULADD(at[61], at[70]);    MULADD(at[62], at[69]);    MULADD(at[63], at[68]); 
   COMBA_STORE(C->dp[67]);
   /* 68 */
   COMBA_FORWARD;
   MULADD(at[5], at[127]);    MULADD(at[6], at[126]);    MULADD(at[7], at[125]);    MULADD(at[8], at[124]);    MULADD(at[9], at[123]);    MULADD(at[10], at[122]);    MULADD(at[11], at[121]);    MULADD(at[12], at[120]);    MULADD(at[13], at[119]);    MULADD(at[14], at[118]);    MULADD(at[15], at[117]);    MULADD(at[16], at[116]);    MULADD(at[17], at[115]);    MULADD(at[18], at[114]);    MULADD(at[19], at[113]);    MULADD(at[20], at[112]);    MULADD(at[21], at[111]);    MULADD(at[22], at[110]);    MULADD(at[23], at[109]);    MULADD(at[24], at[108]);    MULADD(at[25], at[107]);    MULADD(at[26], at[106]);    MULADD(at[27], at[105]);    MULADD(at[28], at[104]);    MULADD(at[29], at[103]);    MULADD(at[30], at[102]);    MULADD(at[31], at[101]);    MULADD(at[32], at[100]);    MULADD(at[33], at[99]);    MULADD(at[34], at[98]);    MULADD(at[35], at[97]);    MULADD(at[36], at[96]);    MULADD(at[37], at[95]);    MULADD(at[38], at[94]);    MULADD(at[39], at[93]);    MULADD(at[40], at[92]);    MULADD(at[41], at[91]);    MULADD(at[42], at[90]);    MULADD(at[43], at[89]);    MULADD(at[44], at[88]);    MULADD(at[45], at[87]);    MULADD(at[46], at[86]);    MULADD(at[47], at[85]);    MULADD(at[48], at[84]);    MULADD(at[49], at[83]);    MULADD(at[50], at[82]);    MULADD(at[51], at[81]);    MULADD(at[52], at[80]);    MULADD(at[53], at[79]);    MULADD(at[54], at[78]);    MULADD(at[55], at[77]);    MULADD(at[56], at[76]);    MULADD(at[57], at[75]);    MULADD(at[58], at[74]);    MULADD(at[59], at[73]);    MULADD(at[60], at[72]);    MULADD(at[61], at[71]);    MULADD(at[62], at[70]);    MULADD(at[63], at[69]); 
   COMBA_STORE(C->dp[68]);
   /* 69 */
   COMBA_FORWARD;
   MULADD(at[6], at[127]);    MULADD(at[7], at[126]);    MULADD(at[8], at[125]);    MULADD(at[9], at[124]);    MULADD(at[10], at[123]);    MULADD(at[11], at[122]);    MULADD(at[12], at[121]);    MULADD(at[13], at[120]);    MULADD(at[14], at[119]);    MULADD(at[15], at[118]);    MULADD(at[16], at[117]);    MULADD(at[17], at[116]);    MULADD(at[18], at[115]);    MULADD(at[19], at[114]);    MULADD(at[20], at[113]);    MULADD(at[21], at[112]);    MULADD(at[22], at[111]);    MULADD(at[23], at[110]);    MULADD(at[24], at[109]);    MULADD(at[25], at[108]);    MULADD(at[26], at[107]);    MULADD(at[27], at[106]);    MULADD(at[28], at[105]);    MULADD(at[29], at[104]);    MULADD(at[30], at[103]);    MULADD(at[31], at[102]);    MULADD(at[32], at[101]);    MULADD(at[33], at[100]);    MULADD(at[34], at[99]);    MULADD(at[35], at[98]);    MULADD(at[36], at[97]);    MULADD(at[37], at[96]);    MULADD(at[38], at[95]);    MULADD(at[39], at[94]);    MULADD(at[40], at[93]);    MULADD(at[41], at[92]);    MULADD(at[42], at[91]);    MULADD(at[43], at[90]);    MULADD(at[44], at[89]);    MULADD(at[45], at[88]);    MULADD(at[46], at[87]);    MULADD(at[47], at[86]);    MULADD(at[48], at[85]);    MULADD(at[49], at[84]);    MULADD(at[50], at[83]);    MULADD(at[51], at[82]);    MULADD(at[52], at[81]);    MULADD(at[53], at[80]);    MULADD(at[54], at[79]);    MULADD(at[55], at[78]);    MULADD(at[56], at[77]);    MULADD(at[57], at[76]);    MULADD(at[58], at[75]);    MULADD(at[59], at[74]);    MULADD(at[60], at[73]);    MULADD(at[61], at[72]);    MULADD(at[62], at[71]);    MULADD(at[63], at[70]); 
   COMBA_STORE(C->dp[69]);
   /* 70 */
   COMBA_FORWARD;
   MULADD(at[7], at[127]);    MULADD(at[8], at[126]);    MULADD(at[9], at[125]);    MULADD(at[10], at[124]);    MULADD(at[11], at[123]);    MULADD(at[12], at[122]);    MULADD(at[13], at[121]);    MULADD(at[14], at[120]);    MULADD(at[15], at[119]);    MULADD(at[16], at[118]);    MULADD(at[17], at[117]);    MULADD(at[18], at[116]);    MULADD(at[19], at[115]);    MULADD(at[20], at[114]);    MULADD(at[21], at[113]);    MULADD(at[22], at[112]);    MULADD(at[23], at[111]);    MULADD(at[24], at[110]);    MULADD(at[25], at[109]);    MULADD(at[26], at[108]);    MULADD(at[27], at[107]);    MULADD(at[28], at[106]);    MULADD(at[29], at[105]);    MULADD(at[30], at[104]);    MULADD(at[31], at[103]);    MULADD(at[32], at[102]);    MULADD(at[33], at[101]);    MULADD(at[34], at[100]);    MULADD(at[35], at[99]);    MULADD(at[36], at[98]);    MULADD(at[37], at[97]);    MULADD(at[38], at[96]);    MULADD(at[39], at[95]);    MULADD(at[40], at[94]);    MULADD(at[41], at[93]);    MULADD(at[42], at[92]);    MULADD(at[43], at[91]);    MULADD(at[44], at[90]);    MULADD(at[45], at[89]);    MULADD(at[46], at[88]);    MULADD(at[47], at[87]);    MULADD(at[48], at[86]);    MULADD(at[49], at[85]);    MULADD(at[50], at[84]);    MULADD(at[51], at[83]);    MULADD(at[52], at[82]);    MULADD(at[53], at[81]);    MULADD(at[54], at[80]);    MULADD(at[55], at[79]);    MULADD(at[56], at[78]);    MULADD(at[57], at[77]);    MULADD(at[58], at[76]);    MULADD(at[59], at[75]);    MULADD(at[60], at[74]);    MULADD(at[61], at[73]);    MULADD(at[62], at[72]);    MULADD(at[63], at[71]); 
   COMBA_STORE(C->dp[70]);
   /* 71 */
   COMBA_FORWARD;
   MULADD(at[8], at[127]);    MULADD(at[9], at[126]);    MULADD(at[10], at[125]);    MULADD(at[11], at[124]);    MULADD(at[12], at[123]);    MULADD(at[13], at[122]);    MULADD(at[14], at[121]);    MULADD(at[15], at[120]);    MULADD(at[16], at[119]);    MULADD(at[17], at[118]);    MULADD(at[18], at[117]);    MULADD(at[19], at[116]);    MULADD(at[20], at[115]);    MULADD(at[21], at[114]);    MULADD(at[22], at[113]);    MULADD(at[23], at[112]);    MULADD(at[24], at[111]);    MULADD(at[25], at[110]);    MULADD(at[26], at[109]);    MULADD(at[27], at[108]);    MULADD(at[28], at[107]);    MULADD(at[29], at[106]);    MULADD(at[30], at[105]);    MULADD(at[31], at[104]);    MULADD(at[32], at[103]);    MULADD(at[33], at[102]);    MULADD(at[34], at[101]);    MULADD(at[35], at[100]);    MULADD(at[36], at[99]);    MULADD(at[37], at[98]);    MULADD(at[38], at[97]);    MULADD(at[39], at[96]);    MULADD(at[40], at[95]);    MULADD(at[41], at[94]);    MULADD(at[42], at[93]);    MULADD(at[43], at[92]);    MULADD(at[44], at[91]);    MULADD(at[45], at[90]);    MULADD(at[46], at[89]);    MULADD(at[47], at[88]);    MULADD(at[48], at[87]);    MULADD(at[49], at[86]);    MULADD(at[50], at[85]);    MULADD(at[51], at[84]);    MULADD(at[52], at[83]);    MULADD(at[53], at[82]);    MULADD(at[54], at[81]);    MULADD(at[55], at[80]);    MULADD(at[56], at[79]);    MULADD(at[57], at[78]);    MULADD(at[58], at[77]);    MULADD(at[59], at[76]);    MULADD(at[60], at[75]);    MULADD(at[61], at[74]);    MULADD(at[62], at[73]);    MULADD(at[63], at[72]); 
   COMBA_STORE(C->dp[71]);
   /* 72 */
   COMBA_FORWARD;
   MULADD(at[9], at[127]);    MULADD(at[10], at[126]);    MULADD(at[11], at[125]);    MULADD(at[12], at[124]);    MULADD(at[13], at[123]);    MULADD(at[14], at[122]);    MULADD(at[15], at[121]);    MULADD(at[16], at[120]);    MULADD(at[17], at[119]);    MULADD(at[18], at[118]);    MULADD(at[19], at[117]);    MULADD(at[20], at[116]);    MULADD(at[21], at[115]);    MULADD(at[22], at[114]);    MULADD(at[23], at[113]);    MULADD(at[24], at[112]);    MULADD(at[25], at[111]);    MULADD(at[26], at[110]);    MULADD(at[27], at[109]);    MULADD(at[28], at[108]);    MULADD(at[29], at[107]);    MULADD(at[30], at[106]);    MULADD(at[31], at[105]);    MULADD(at[32], at[104]);    MULADD(at[33], at[103]);    MULADD(at[34], at[102]);    MULADD(at[35], at[101]);    MULADD(at[36], at[100]);    MULADD(at[37], at[99]);    MULADD(at[38], at[98]);    MULADD(at[39], at[97]);    MULADD(at[40], at[96]);    MULADD(at[41], at[95]);    MULADD(at[42], at[94]);    MULADD(at[43], at[93]);    MULADD(at[44], at[92]);    MULADD(at[45], at[91]);    MULADD(at[46], at[90]);    MULADD(at[47], at[89]);    MULADD(at[48], at[88]);    MULADD(at[49], at[87]);    MULADD(at[50], at[86]);    MULADD(at[51], at[85]);    MULADD(at[52], at[84]);    MULADD(at[53], at[83]);    MULADD(at[54], at[82]);    MULADD(at[55], at[81]);    MULADD(at[56], at[80]);    MULADD(at[57], at[79]);    MULADD(at[58], at[78]);    MULADD(at[59], at[77]);    MULADD(at[60], at[76]);    MULADD(at[61], at[75]);    MULADD(at[62], at[74]);    MULADD(at[63], at[73]); 
   COMBA_STORE(C->dp[72]);
   /* 73 */
   COMBA_FORWARD;
   MULADD(at[10], at[127]);    MULADD(at[11], at[126]);    MULADD(at[12], at[125]);    MULADD(at[13], at[124]);    MULADD(at[14], at[123]);    MULADD(at[15], at[122]);    MULADD(at[16], at[121]);    MULADD(at[17], at[120]);    MULADD(at[18], at[119]);    MULADD(at[19], at[118]);    MULADD(at[20], at[117]);    MULADD(at[21], at[116]);    MULADD(at[22], at[115]);    MULADD(at[23], at[114]);    MULADD(at[24], at[113]);    MULADD(at[25], at[112]);    MULADD(at[26], at[111]);    MULADD(at[27], at[110]);    MULADD(at[28], at[109]);    MULADD(at[29], at[108]);    MULADD(at[30], at[107]);    MULADD(at[31], at[106]);    MULADD(at[32], at[105]);    MULADD(at[33], at[104]);    MULADD(at[34], at[103]);    MULADD(at[35], at[102]);    MULADD(at[36], at[101]);    MULADD(at[37], at[100]);    MULADD(at[38], at[99]);    MULADD(at[39], at[98]);    MULADD(at[40], at[97]);    MULADD(at[41], at[96]);    MULADD(at[42], at[95]);    MULADD(at[43], at[94]);    MULADD(at[44], at[93]);    MULADD(at[45], at[92]);    MULADD(at[46], at[91]);    MULADD(at[47], at[90]);    MULADD(at[48], at[89]);    MULADD(at[49], at[88]);    MULADD(at[50], at[87]);    MULADD(at[51], at[86]);    MULADD(at[52], at[85]);    MULADD(at[53], at[84]);    MULADD(at[54], at[83]);    MULADD(at[55], at[82]);    MULADD(at[56], at[81]);    MULADD(at[57], at[80]);    MULADD(at[58], at[79]);    MULADD(at[59], at[78]);    MULADD(at[60], at[77]);    MULADD(at[61], at[76]);    MULADD(at[62], at[75]);    MULADD(at[63], at[74]); 
   COMBA_STORE(C->dp[73]);
   /* 74 */
   COMBA_FORWARD;
   MULADD(at[11], at[127]);    MULADD(at[12], at[126]);    MULADD(at[13], at[125]);    MULADD(at[14], at[124]);    MULADD(at[15], at[123]);    MULADD(at[16], at[122]);    MULADD(at[17], at[121]);    MULADD(at[18], at[120]);    MULADD(at[19], at[119]);    MULADD(at[20], at[118]);    MULADD(at[21], at[117]);    MULADD(at[22], at[116]);    MULADD(at[23], at[115]);    MULADD(at[24], at[114]);    MULADD(at[25], at[113]);    MULADD(at[26], at[112]);    MULADD(at[27], at[111]);    MULADD(at[28], at[110]);    MULADD(at[29], at[109]);    MULADD(at[30], at[108]);    MULADD(at[31], at[107]);    MULADD(at[32], at[106]);    MULADD(at[33], at[105]);    MULADD(at[34], at[104]);    MULADD(at[35], at[103]);    MULADD(at[36], at[102]);    MULADD(at[37], at[101]);    MULADD(at[38], at[100]);    MULADD(at[39], at[99]);    MULADD(at[40], at[98]);    MULADD(at[41], at[97]);    MULADD(at[42], at[96]);    MULADD(at[43], at[95]);    MULADD(at[44], at[94]);    MULADD(at[45], at[93]);    MULADD(at[46], at[92]);    MULADD(at[47], at[91]);    MULADD(at[48], at[90]);    MULADD(at[49], at[89]);    MULADD(at[50], at[88]);    MULADD(at[51], at[87]);    MULADD(at[52], at[86]);    MULADD(at[53], at[85]);    MULADD(at[54], at[84]);    MULADD(at[55], at[83]);    MULADD(at[56], at[82]);    MULADD(at[57], at[81]);    MULADD(at[58], at[80]);    MULADD(at[59], at[79]);    MULADD(at[60], at[78]);    MULADD(at[61], at[77]);    MULADD(at[62], at[76]);    MULADD(at[63], at[75]); 
   COMBA_STORE(C->dp[74]);
   /* 75 */
   COMBA_FORWARD;
   MULADD(at[12], at[127]);    MULADD(at[13], at[126]);    MULADD(at[14], at[125]);    MULADD(at[15], at[124]);    MULADD(at[16], at[123]);    MULADD(at[17], at[122]);    MULADD(at[18], at[121]);    MULADD(at[19], at[120]);    MULADD(at[20], at[119]);    MULADD(at[21], at[118]);    MULADD(at[22], at[117]);    MULADD(at[23], at[116]);    MULADD(at[24], at[115]);    MULADD(at[25], at[114]);    MULADD(at[26], at[113]);    MULADD(at[27], at[112]);    MULADD(at[28], at[111]);    MULADD(at[29], at[110]);    MULADD(at[30], at[109]);    MULADD(at[31], at[108]);    MULADD(at[32], at[107]);    MULADD(at[33], at[106]);    MULADD(at[34], at[105]);    MULADD(at[35], at[104]);    MULADD(at[36], at[103]);    MULADD(at[37], at[102]);    MULADD(at[38], at[101]);    MULADD(at[39], at[100]);    MULADD(at[40], at[99]);    MULADD(at[41], at[98]);    MULADD(at[42], at[97]);    MULADD(at[43], at[96]);    MULADD(at[44], at[95]);    MULADD(at[45], at[94]);    MULADD(at[46], at[93]);    MULADD(at[47], at[92]);    MULADD(at[48], at[91]);    MULADD(at[49], at[90]);    MULADD(at[50], at[89]);    MULADD(at[51], at[88]);    MULADD(at[52], at[87]);    MULADD(at[53], at[86]);    MULADD(at[54], at[85]);    MULADD(at[55], at[84]);    MULADD(at[56], at[83]);    MULADD(at[57], at[82]);    MULADD(at[58], at[81]);    MULADD(at[59], at[80]);    MULADD(at[60], at[79]);    MULADD(at[61], at[78]);    MULADD(at[62], at[77]);    MULADD(at[63], at[76]); 
   COMBA_STORE(C->dp[75]);
   /* 76 */
   COMBA_FORWARD;
   MULADD(at[13], at[127]);    MULADD(at[14], at[126]);    MULADD(at[15], at[125]);    MULADD(at[16], at[124]);    MULADD(at[17], at[123]);    MULADD(at[18], at[122]);    MULADD(at[19], at[121]);    MULADD(at[20], at[120]);    MULADD(at[21], at[119]);    MULADD(at[22], at[118]);    MULADD(at[23], at[117]);    MULADD(at[24], at[116]);    MULADD(at[25], at[115]);    MULADD(at[26], at[114]);    MULADD(at[27], at[113]);    MULADD(at[28], at[112]);    MULADD(at[29], at[111]);    MULADD(at[30], at[110]);    MULADD(at[31], at[109]);    MULADD(at[32], at[108]);    MULADD(at[33], at[107]);    MULADD(at[34], at[106]);    MULADD(at[35], at[105]);    MULADD(at[36], at[104]);    MULADD(at[37], at[103]);    MULADD(at[38], at[102]);    MULADD(at[39], at[101]);    MULADD(at[40], at[100]);    MULADD(at[41], at[99]);    MULADD(at[42], at[98]);    MULADD(at[43], at[97]);    MULADD(at[44], at[96]);    MULADD(at[45], at[95]);    MULADD(at[46], at[94]);    MULADD(at[47], at[93]);    MULADD(at[48], at[92]);    MULADD(at[49], at[91]);    MULADD(at[50], at[90]);    MULADD(at[51], at[89]);    MULADD(at[52], at[88]);    MULADD(at[53], at[87]);    MULADD(at[54], at[86]);    MULADD(at[55], at[85]);    MULADD(at[56], at[84]);    MULADD(at[57], at[83]);    MULADD(at[58], at[82]);    MULADD(at[59], at[81]);    MULADD(at[60], at[80]);    MULADD(at[61], at[79]);    MULADD(at[62], at[78]);    MULADD(at[63], at[77]); 
   COMBA_STORE(C->dp[76]);
   /* 77 */
   COMBA_FORWARD;
   MULADD(at[14], at[127]);    MULADD(at[15], at[126]);    MULADD(at[16], at[125]);    MULADD(at[17], at[124]);    MULADD(at[18], at[123]);    MULADD(at[19], at[122]);    MULADD(at[20], at[121]);    MULADD(at[21], at[120]);    MULADD(at[22], at[119]);    MULADD(at[23], at[118]);    MULADD(at[24], at[117]);    MULADD(at[25], at[116]);    MULADD(at[26], at[115]);    MULADD(at[27], at[114]);    MULADD(at[28], at[113]);    MULADD(at[29], at[112]);    MULADD(at[30], at[111]);    MULADD(at[31], at[110]);    MULADD(at[32], at[109]);    MULADD(at[33], at[108]);    MULADD(at[34], at[107]);    MULADD(at[35], at[106]);    MULADD(at[36], at[105]);    MULADD(at[37], at[104]);    MULADD(at[38], at[103]);    MULADD(at[39], at[102]);    MULADD(at[40], at[101]);    MULADD(at[41], at[100]);    MULADD(at[42], at[99]);    MULADD(at[43], at[98]);    MULADD(at[44], at[97]);    MULADD(at[45], at[96]);    MULADD(at[46], at[95]);    MULADD(at[47], at[94]);    MULADD(at[48], at[93]);    MULADD(at[49], at[92]);    MULADD(at[50], at[91]);    MULADD(at[51], at[90]);    MULADD(at[52], at[89]);    MULADD(at[53], at[88]);    MULADD(at[54], at[87]);    MULADD(at[55], at[86]);    MULADD(at[56], at[85]);    MULADD(at[57], at[84]);    MULADD(at[58], at[83]);    MULADD(at[59], at[82]);    MULADD(at[60], at[81]);    MULADD(at[61], at[80]);    MULADD(at[62], at[79]);    MULADD(at[63], at[78]); 
   COMBA_STORE(C->dp[77]);
   /* 78 */
   COMBA_FORWARD;
   MULADD(at[15], at[127]);    MULADD(at[16], at[126]);    MULADD(at[17], at[125]);    MULADD(at[18], at[124]);    MULADD(at[19], at[123]);    MULADD(at[20], at[122]);    MULADD(at[21], at[121]);    MULADD(at[22], at[120]);    MULADD(at[23], at[119]);    MULADD(at[24], at[118]);    MULADD(at[25], at[117]);    MULADD(at[26], at[116]);    MULADD(at[27], at[115]);    MULADD(at[28], at[114]);    MULADD(at[29], at[113]);    MULADD(at[30], at[112]);    MULADD(at[31], at[111]);    MULADD(at[32], at[110]);    MULADD(at[33], at[109]);    MULADD(at[34], at[108]);    MULADD(at[35], at[107]);    MULADD(at[36], at[106]);    MULADD(at[37], at[105]);    MULADD(at[38], at[104]);    MULADD(at[39], at[103]);    MULADD(at[40], at[102]);    MULADD(at[41], at[101]);    MULADD(at[42], at[100]);    MULADD(at[43], at[99]);    MULADD(at[44], at[98]);    MULADD(at[45], at[97]);    MULADD(at[46], at[96]);    MULADD(at[47], at[95]);    MULADD(at[48], at[94]);    MULADD(at[49], at[93]);    MULADD(at[50], at[92]);    MULADD(at[51], at[91]);    MULADD(at[52], at[90]);    MULADD(at[53], at[89]);    MULADD(at[54], at[88]);    MULADD(at[55], at[87]);    MULADD(at[56], at[86]);    MULADD(at[57], at[85]);    MULADD(at[58], at[84]);    MULADD(at[59], at[83]);    MULADD(at[60], at[82]);    MULADD(at[61], at[81]);    MULADD(at[62], at[80]);    MULADD(at[63], at[79]); 
   COMBA_STORE(C->dp[78]);
   /* 79 */
   COMBA_FORWARD;
   MULADD(at[16], at[127]);    MULADD(at[17], at[126]);    MULADD(at[18], at[125]);    MULADD(at[19], at[124]);    MULADD(at[20], at[123]);    MULADD(at[21], at[122]);    MULADD(at[22], at[121]);    MULADD(at[23], at[120]);    MULADD(at[24], at[119]);    MULADD(at[25], at[118]);    MULADD(at[26], at[117]);    MULADD(at[27], at[116]);    MULADD(at[28], at[115]);    MULADD(at[29], at[114]);    MULADD(at[30], at[113]);    MULADD(at[31], at[112]);    MULADD(at[32], at[111]);    MULADD(at[33], at[110]);    MULADD(at[34], at[109]);    MULADD(at[35], at[108]);    MULADD(at[36], at[107]);    MULADD(at[37], at[106]);    MULADD(at[38], at[105]);    MULADD(at[39], at[104]);    MULADD(at[40], at[103]);    MULADD(at[41], at[102]);    MULADD(at[42], at[101]);    MULADD(at[43], at[100]);    MULADD(at[44], at[99]);    MULADD(at[45], at[98]);    MULADD(at[46], at[97]);    MULADD(at[47], at[96]);    MULADD(at[48], at[95]);    MULADD(at[49], at[94]);    MULADD(at[50], at[93]);    MULADD(at[51], at[92]);    MULADD(at[52], at[91]);    MULADD(at[53], at[90]);    MULADD(at[54], at[89]);    MULADD(at[55], at[88]);    MULADD(at[56], at[87]);    MULADD(at[57], at[86]);    MULADD(at[58], at[85]);    MULADD(at[59], at[84]);    MULADD(at[60], at[83]);    MULADD(at[61], at[82]);    MULADD(at[62], at[81]);    MULADD(at[63], at[80]); 
   COMBA_STORE(C->dp[79]);
   /* 80 */
   COMBA_FORWARD;
   MULADD(at[17], at[127]);    MULADD(at[18], at[126]);    MULADD(at[19], at[125]);    MULADD(at[20], at[124]);    MULADD(at[21], at[123]);    MULADD(at[22], at[122]);    MULADD(at[23], at[121]);    MULADD(at[24], at[120]);    MULADD(at[25], at[119]);    MULADD(at[26], at[118]);    MULADD(at[27], at[117]);    MULADD(at[28], at[116]);    MULADD(at[29], at[115]);    MULADD(at[30], at[114]);    MULADD(at[31], at[113]);    MULADD(at[32], at[112]);    MULADD(at[33], at[111]);    MULADD(at[34], at[110]);    MULADD(at[35], at[109]);    MULADD(at[36], at[108]);    MULADD(at[37], at[107]);    MULADD(at[38], at[106]);    MULADD(at[39], at[105]);    MULADD(at[40], at[104]);    MULADD(at[41], at[103]);    MULADD(at[42], at[102]);    MULADD(at[43], at[101]);    MULADD(at[44], at[100]);    MULADD(at[45], at[99]);    MULADD(at[46], at[98]);    MULADD(at[47], at[97]);    MULADD(at[48], at[96]);    MULADD(at[49], at[95]);    MULADD(at[50], at[94]);    MULADD(at[51], at[93]);    MULADD(at[52], at[92]);    MULADD(at[53], at[91]);    MULADD(at[54], at[90]);    MULADD(at[55], at[89]);    MULADD(at[56], at[88]);    MULADD(at[57], at[87]);    MULADD(at[58], at[86]);    MULADD(at[59], at[85]);    MULADD(at[60], at[84]);    MULADD(at[61], at[83]);    MULADD(at[62], at[82]);    MULADD(at[63], at[81]); 
   COMBA_STORE(C->dp[80]);
   /* 81 */
   COMBA_FORWARD;
   MULADD(at[18], at[127]);    MULADD(at[19], at[126]);    MULADD(at[20], at[125]);    MULADD(at[21], at[124]);    MULADD(at[22], at[123]);    MULADD(at[23], at[122]);    MULADD(at[24], at[121]);    MULADD(at[25], at[120]);    MULADD(at[26], at[119]);    MULADD(at[27], at[118]);    MULADD(at[28], at[117]);    MULADD(at[29], at[116]);    MULADD(at[30], at[115]);    MULADD(at[31], at[114]);    MULADD(at[32], at[113]);    MULADD(at[33], at[112]);    MULADD(at[34], at[111]);    MULADD(at[35], at[110]);    MULADD(at[36], at[109]);    MULADD(at[37], at[108]);    MULADD(at[38], at[107]);    MULADD(at[39], at[106]);    MULADD(at[40], at[105]);    MULADD(at[41], at[104]);    MULADD(at[42], at[103]);    MULADD(at[43], at[102]);    MULADD(at[44], at[101]);    MULADD(at[45], at[100]);    MULADD(at[46], at[99]);    MULADD(at[47], at[98]);    MULADD(at[48], at[97]);    MULADD(at[49], at[96]);    MULADD(at[50], at[95]);    MULADD(at[51], at[94]);    MULADD(at[52], at[93]);    MULADD(at[53], at[92]);    MULADD(at[54], at[91]);    MULADD(at[55], at[90]);    MULADD(at[56], at[89]);    MULADD(at[57], at[88]);    MULADD(at[58], at[87]);    MULADD(at[59], at[86]);    MULADD(at[60], at[85]);    MULADD(at[61], at[84]);    MULADD(at[62], at[83]);    MULADD(at[63], at[82]); 
   COMBA_STORE(C->dp[81]);
   /* 82 */
   COMBA_FORWARD;
   MULADD(at[19], at[127]);    MULADD(at[20], at[126]);    MULADD(at[21], at[125]);    MULADD(at[22], at[124]);    MULADD(at[23], at[123]);    MULADD(at[24], at[122]);    MULADD(at[25], at[121]);    MULADD(at[26], at[120]);    MULADD(at[27], at[119]);    MULADD(at[28], at[118]);    MULADD(at[29], at[117]);    MULADD(at[30], at[116]);    MULADD(at[31], at[115]);    MULADD(at[32], at[114]);    MULADD(at[33], at[113]);    MULADD(at[34], at[112]);    MULADD(at[35], at[111]);    MULADD(at[36], at[110]);    MULADD(at[37], at[109]);    MULADD(at[38], at[108]);    MULADD(at[39], at[107]);    MULADD(at[40], at[106]);    MULADD(at[41], at[105]);    MULADD(at[42], at[104]);    MULADD(at[43], at[103]);    MULADD(at[44], at[102]);    MULADD(at[45], at[101]);    MULADD(at[46], at[100]);    MULADD(at[47], at[99]);    MULADD(at[48], at[98]);    MULADD(at[49], at[97]);    MULADD(at[50], at[96]);    MULADD(at[51], at[95]);    MULADD(at[52], at[94]);    MULADD(at[53], at[93]);    MULADD(at[54], at[92]);    MULADD(at[55], at[91]);    MULADD(at[56], at[90]);    MULADD(at[57], at[89]);    MULADD(at[58], at[88]);    MULADD(at[59], at[87]);    MULADD(at[60], at[86]);    MULADD(at[61], at[85]);    MULADD(at[62], at[84]);    MULADD(at[63], at[83]); 
   COMBA_STORE(C->dp[82]);
   /* 83 */
   COMBA_FORWARD;
   MULADD(at[20], at[127]);    MULADD(at[21], at[126]);    MULADD(at[22], at[125]);    MULADD(at[23], at[124]);    MULADD(at[24], at[123]);    MULADD(at[25], at[122]);    MULADD(at[26], at[121]);    MULADD(at[27], at[120]);    MULADD(at[28], at[119]);    MULADD(at[29], at[118]);    MULADD(at[30], at[117]);    MULADD(at[31], at[116]);    MULADD(at[32], at[115]);    MULADD(at[33], at[114]);    MULADD(at[34], at[113]);    MULADD(at[35], at[112]);    MULADD(at[36], at[111]);    MULADD(at[37], at[110]);    MULADD(at[38], at[109]);    MULADD(at[39], at[108]);    MULADD(at[40], at[107]);    MULADD(at[41], at[106]);    MULADD(at[42], at[105]);    MULADD(at[43], at[104]);    MULADD(at[44], at[103]);    MULADD(at[45], at[102]);    MULADD(at[46], at[101]);    MULADD(at[47], at[100]);    MULADD(at[48], at[99]);    MULADD(at[49], at[98]);    MULADD(at[50], at[97]);    MULADD(at[51], at[96]);    MULADD(at[52], at[95]);    MULADD(at[53], at[94]);    MULADD(at[54], at[93]);    MULADD(at[55], at[92]);    MULADD(at[56], at[91]);    MULADD(at[57], at[90]);    MULADD(at[58], at[89]);    MULADD(at[59], at[88]);    MULADD(at[60], at[87]);    MULADD(at[61], at[86]);    MULADD(at[62], at[85]);    MULADD(at[63], at[84]); 
   COMBA_STORE(C->dp[83]);
   /* 84 */
   COMBA_FORWARD;
   MULADD(at[21], at[127]);    MULADD(at[22], at[126]);    MULADD(at[23], at[125]);    MULADD(at[24], at[124]);    MULADD(at[25], at[123]);    MULADD(at[26], at[122]);    MULADD(at[27], at[121]);    MULADD(at[28], at[120]);    MULADD(at[29], at[119]);    MULADD(at[30], at[118]);    MULADD(at[31], at[117]);    MULADD(at[32], at[116]);    MULADD(at[33], at[115]);    MULADD(at[34], at[114]);    MULADD(at[35], at[113]);    MULADD(at[36], at[112]);    MULADD(at[37], at[111]);    MULADD(at[38], at[110]);    MULADD(at[39], at[109]);    MULADD(at[40], at[108]);    MULADD(at[41], at[107]);    MULADD(at[42], at[106]);    MULADD(at[43], at[105]);    MULADD(at[44], at[104]);    MULADD(at[45], at[103]);    MULADD(at[46], at[102]);    MULADD(at[47], at[101]);    MULADD(at[48], at[100]);    MULADD(at[49], at[99]);    MULADD(at[50], at[98]);    MULADD(at[51], at[97]);    MULADD(at[52], at[96]);    MULADD(at[53], at[95]);    MULADD(at[54], at[94]);    MULADD(at[55], at[93]);    MULADD(at[56], at[92]);    MULADD(at[57], at[91]);    MULADD(at[58], at[90]);    MULADD(at[59], at[89]);    MULADD(at[60], at[88]);    MULADD(at[61], at[87]);    MULADD(at[62], at[86]);    MULADD(at[63], at[85]); 
   COMBA_STORE(C->dp[84]);
   /* 85 */
   COMBA_FORWARD;
   MULADD(at[22], at[127]);    MULADD(at[23], at[126]);    MULADD(at[24], at[125]);    MULADD(at[25], at[124]);    MULADD(at[26], at[123]);    MULADD(at[27], at[122]);    MULADD(at[28], at[121]);    MULADD(at[29], at[120]);    MULADD(at[30], at[119]);    MULADD(at[31], at[118]);    MULADD(at[32], at[117]);    MULADD(at[33], at[116]);    MULADD(at[34], at[115]);    MULADD(at[35], at[114]);    MULADD(at[36], at[113]);    MULADD(at[37], at[112]);    MULADD(at[38], at[111]);    MULADD(at[39], at[110]);    MULADD(at[40], at[109]);    MULADD(at[41], at[108]);    MULADD(at[42], at[107]);    MULADD(at[43], at[106]);    MULADD(at[44], at[105]);    MULADD(at[45], at[104]);    MULADD(at[46], at[103]);    MULADD(at[47], at[102]);    MULADD(at[48], at[101]);    MULADD(at[49], at[100]);    MULADD(at[50], at[99]);    MULADD(at[51], at[98]);    MULADD(at[52], at[97]);    MULADD(at[53], at[96]);    MULADD(at[54], at[95]);    MULADD(at[55], at[94]);    MULADD(at[56], at[93]);    MULADD(at[57], at[92]);    MULADD(at[58], at[91]);    MULADD(at[59], at[90]);    MULADD(at[60], at[89]);    MULADD(at[61], at[88]);    MULADD(at[62], at[87]);    MULADD(at[63], at[86]); 
   COMBA_STORE(C->dp[85]);
   /* 86 */
   COMBA_FORWARD;
   MULADD(at[23], at[127]);    MULADD(at[24], at[126]);    MULADD(at[25], at[125]);    MULADD(at[26], at[124]);    MULADD(at[27], at[123]);    MULADD(at[28], at[122]);    MULADD(at[29], at[121]);    MULADD(at[30], at[120]);    MULADD(at[31], at[119]);    MULADD(at[32], at[118]);    MULADD(at[33], at[117]);    MULADD(at[34], at[116]);    MULADD(at[35], at[115]);    MULADD(at[36], at[114]);    MULADD(at[37], at[113]);    MULADD(at[38], at[112]);    MULADD(at[39], at[111]);    MULADD(at[40], at[110]);    MULADD(at[41], at[109]);    MULADD(at[42], at[108]);    MULADD(at[43], at[107]);    MULADD(at[44], at[106]);    MULADD(at[45], at[105]);    MULADD(at[46], at[104]);    MULADD(at[47], at[103]);    MULADD(at[48], at[102]);    MULADD(at[49], at[101]);    MULADD(at[50], at[100]);    MULADD(at[51], at[99]);    MULADD(at[52], at[98]);    MULADD(at[53], at[97]);    MULADD(at[54], at[96]);    MULADD(at[55], at[95]);    MULADD(at[56], at[94]);    MULADD(at[57], at[93]);    MULADD(at[58], at[92]);    MULADD(at[59], at[91]);    MULADD(at[60], at[90]);    MULADD(at[61], at[89]);    MULADD(at[62], at[88]);    MULADD(at[63], at[87]); 
   COMBA_STORE(C->dp[86]);
   /* 87 */
   COMBA_FORWARD;
   MULADD(at[24], at[127]);    MULADD(at[25], at[126]);    MULADD(at[26], at[125]);    MULADD(at[27], at[124]);    MULADD(at[28], at[123]);    MULADD(at[29], at[122]);    MULADD(at[30], at[121]);    MULADD(at[31], at[120]);    MULADD(at[32], at[119]);    MULADD(at[33], at[118]);    MULADD(at[34], at[117]);    MULADD(at[35], at[116]);    MULADD(at[36], at[115]);    MULADD(at[37], at[114]);    MULADD(at[38], at[113]);    MULADD(at[39], at[112]);    MULADD(at[40], at[111]);    MULADD(at[41], at[110]);    MULADD(at[42], at[109]);    MULADD(at[43], at[108]);    MULADD(at[44], at[107]);    MULADD(at[45], at[106]);    MULADD(at[46], at[105]);    MULADD(at[47], at[104]);    MULADD(at[48], at[103]);    MULADD(at[49], at[102]);    MULADD(at[50], at[101]);    MULADD(at[51], at[100]);    MULADD(at[52], at[99]);    MULADD(at[53], at[98]);    MULADD(at[54], at[97]);    MULADD(at[55], at[96]);    MULADD(at[56], at[95]);    MULADD(at[57], at[94]);    MULADD(at[58], at[93]);    MULADD(at[59], at[92]);    MULADD(at[60], at[91]);    MULADD(at[61], at[90]);    MULADD(at[62], at[89]);    MULADD(at[63], at[88]); 
   COMBA_STORE(C->dp[87]);
   /* 88 */
   COMBA_FORWARD;
   MULADD(at[25], at[127]);    MULADD(at[26], at[126]);    MULADD(at[27], at[125]);    MULADD(at[28], at[124]);    MULADD(at[29], at[123]);    MULADD(at[30], at[122]);    MULADD(at[31], at[121]);    MULADD(at[32], at[120]);    MULADD(at[33], at[119]);    MULADD(at[34], at[118]);    MULADD(at[35], at[117]);    MULADD(at[36], at[116]);    MULADD(at[37], at[115]);    MULADD(at[38], at[114]);    MULADD(at[39], at[113]);    MULADD(at[40], at[112]);    MULADD(at[41], at[111]);    MULADD(at[42], at[110]);    MULADD(at[43], at[109]);    MULADD(at[44], at[108]);    MULADD(at[45], at[107]);    MULADD(at[46], at[106]);    MULADD(at[47], at[105]);    MULADD(at[48], at[104]);    MULADD(at[49], at[103]);    MULADD(at[50], at[102]);    MULADD(at[51], at[101]);    MULADD(at[52], at[100]);    MULADD(at[53], at[99]);    MULADD(at[54], at[98]);    MULADD(at[55], at[97]);    MULADD(at[56], at[96]);    MULADD(at[57], at[95]);    MULADD(at[58], at[94]);    MULADD(at[59], at[93]);    MULADD(at[60], at[92]);    MULADD(at[61], at[91]);    MULADD(at[62], at[90]);    MULADD(at[63], at[89]); 
   COMBA_STORE(C->dp[88]);
   /* 89 */
   COMBA_FORWARD;
   MULADD(at[26], at[127]);    MULADD(at[27], at[126]);    MULADD(at[28], at[125]);    MULADD(at[29], at[124]);    MULADD(at[30], at[123]);    MULADD(at[31], at[122]);    MULADD(at[32], at[121]);    MULADD(at[33], at[120]);    MULADD(at[34], at[119]);    MULADD(at[35], at[118]);    MULADD(at[36], at[117]);    MULADD(at[37], at[116]);    MULADD(at[38], at[115]);    MULADD(at[39], at[114]);    MULADD(at[40], at[113]);    MULADD(at[41], at[112]);    MULADD(at[42], at[111]);    MULADD(at[43], at[110]);    MULADD(at[44], at[109]);    MULADD(at[45], at[108]);    MULADD(at[46], at[107]);    MULADD(at[47], at[106]);    MULADD(at[48], at[105]);    MULADD(at[49], at[104]);    MULADD(at[50], at[103]);    MULADD(at[51], at[102]);    MULADD(at[52], at[101]);    MULADD(at[53], at[100]);    MULADD(at[54], at[99]);    MULADD(at[55], at[98]);    MULADD(at[56], at[97]);    MULADD(at[57], at[96]);    MULADD(at[58], at[95]);    MULADD(at[59], at[94]);    MULADD(at[60], at[93]);    MULADD(at[61], at[92]);    MULADD(at[62], at[91]);    MULADD(at[63], at[90]); 
   COMBA_STORE(C->dp[89]);
   /* 90 */
   COMBA_FORWARD;
   MULADD(at[27], at[127]);    MULADD(at[28], at[126]);    MULADD(at[29], at[125]);    MULADD(at[30], at[124]);    MULADD(at[31], at[123]);    MULADD(at[32], at[122]);    MULADD(at[33], at[121]);    MULADD(at[34], at[120]);    MULADD(at[35], at[119]);    MULADD(at[36], at[118]);    MULADD(at[37], at[117]);    MULADD(at[38], at[116]);    MULADD(at[39], at[115]);    MULADD(at[40], at[114]);    MULADD(at[41], at[113]);    MULADD(at[42], at[112]);    MULADD(at[43], at[111]);    MULADD(at[44], at[110]);    MULADD(at[45], at[109]);    MULADD(at[46], at[108]);    MULADD(at[47], at[107]);    MULADD(at[48], at[106]);    MULADD(at[49], at[105]);    MULADD(at[50], at[104]);    MULADD(at[51], at[103]);    MULADD(at[52], at[102]);    MULADD(at[53], at[101]);    MULADD(at[54], at[100]);    MULADD(at[55], at[99]);    MULADD(at[56], at[98]);    MULADD(at[57], at[97]);    MULADD(at[58], at[96]);    MULADD(at[59], at[95]);    MULADD(at[60], at[94]);    MULADD(at[61], at[93]);    MULADD(at[62], at[92]);    MULADD(at[63], at[91]); 
   COMBA_STORE(C->dp[90]);
   /* 91 */
   COMBA_FORWARD;
   MULADD(at[28], at[127]);    MULADD(at[29], at[126]);    MULADD(at[30], at[125]);    MULADD(at[31], at[124]);    MULADD(at[32], at[123]);    MULADD(at[33], at[122]);    MULADD(at[34], at[121]);    MULADD(at[35], at[120]);    MULADD(at[36], at[119]);    MULADD(at[37], at[118]);    MULADD(at[38], at[117]);    MULADD(at[39], at[116]);    MULADD(at[40], at[115]);    MULADD(at[41], at[114]);    MULADD(at[42], at[113]);    MULADD(at[43], at[112]);    MULADD(at[44], at[111]);    MULADD(at[45], at[110]);    MULADD(at[46], at[109]);    MULADD(at[47], at[108]);    MULADD(at[48], at[107]);    MULADD(at[49], at[106]);    MULADD(at[50], at[105]);    MULADD(at[51], at[104]);    MULADD(at[52], at[103]);    MULADD(at[53], at[102]);    MULADD(at[54], at[101]);    MULADD(at[55], at[100]);    MULADD(at[56], at[99]);    MULADD(at[57], at[98]);    MULADD(at[58], at[97]);    MULADD(at[59], at[96]);    MULADD(at[60], at[95]);    MULADD(at[61], at[94]);    MULADD(at[62], at[93]);    MULADD(at[63], at[92]); 
   COMBA_STORE(C->dp[91]);
   /* 92 */
   COMBA_FORWARD;
   MULADD(at[29], at[127]);    MULADD(at[30], at[126]);    MULADD(at[31], at[125]);    MULADD(at[32], at[124]);    MULADD(at[33], at[123]);    MULADD(at[34], at[122]);    MULADD(at[35], at[121]);    MULADD(at[36], at[120]);    MULADD(at[37], at[119]);    MULADD(at[38], at[118]);    MULADD(at[39], at[117]);    MULADD(at[40], at[116]);    MULADD(at[41], at[115]);    MULADD(at[42], at[114]);    MULADD(at[43], at[113]);    MULADD(at[44], at[112]);    MULADD(at[45], at[111]);    MULADD(at[46], at[110]);    MULADD(at[47], at[109]);    MULADD(at[48], at[108]);    MULADD(at[49], at[107]);    MULADD(at[50], at[106]);    MULADD(at[51], at[105]);    MULADD(at[52], at[104]);    MULADD(at[53], at[103]);    MULADD(at[54], at[102]);    MULADD(at[55], at[101]);    MULADD(at[56], at[100]);    MULADD(at[57], at[99]);    MULADD(at[58], at[98]);    MULADD(at[59], at[97]);    MULADD(at[60], at[96]);    MULADD(at[61], at[95]);    MULADD(at[62], at[94]);    MULADD(at[63], at[93]); 
   COMBA_STORE(C->dp[92]);
   /* 93 */
   COMBA_FORWARD;
   MULADD(at[30], at[127]);    MULADD(at[31], at[126]);    MULADD(at[32], at[125]);    MULADD(at[33], at[124]);    MULADD(at[34], at[123]);    MULADD(at[35], at[122]);    MULADD(at[36], at[121]);    MULADD(at[37], at[120]);    MULADD(at[38], at[119]);    MULADD(at[39], at[118]);    MULADD(at[40], at[117]);    MULADD(at[41], at[116]);    MULADD(at[42], at[115]);    MULADD(at[43], at[114]);    MULADD(at[44], at[113]);    MULADD(at[45], at[112]);    MULADD(at[46], at[111]);    MULADD(at[47], at[110]);    MULADD(at[48], at[109]);    MULADD(at[49], at[108]);    MULADD(at[50], at[107]);    MULADD(at[51], at[106]);    MULADD(at[52], at[105]);    MULADD(at[53], at[104]);    MULADD(at[54], at[103]);    MULADD(at[55], at[102]);    MULADD(at[56], at[101]);    MULADD(at[57], at[100]);    MULADD(at[58], at[99]);    MULADD(at[59], at[98]);    MULADD(at[60], at[97]);    MULADD(at[61], at[96]);    MULADD(at[62], at[95]);    MULADD(at[63], at[94]); 
   COMBA_STORE(C->dp[93]);
   /* 94 */
   COMBA_FORWARD;
   MULADD(at[31], at[127]);    MULADD(at[32], at[126]);    MULADD(at[33], at[125]);    MULADD(at[34], at[124]);    MULADD(at[35], at[123]);    MULADD(at[36], at[122]);    MULADD(at[37], at[121]);    MULADD(at[38], at[120]);    MULADD(at[39], at[119]);    MULADD(at[40], at[118]);    MULADD(at[41], at[117]);    MULADD(at[42], at[116]);    MULADD(at[43], at[115]);    MULADD(at[44], at[114]);    MULADD(at[45], at[113]);    MULADD(at[46], at[112]);    MULADD(at[47], at[111]);    MULADD(at[48], at[110]);    MULADD(at[49], at[109]);    MULADD(at[50], at[108]);    MULADD(at[51], at[107]);    MULADD(at[52], at[106]);    MULADD(at[53], at[105]);    MULADD(at[54], at[104]);    MULADD(at[55], at[103]);    MULADD(at[56], at[102]);    MULADD(at[57], at[101]);    MULADD(at[58], at[100]);    MULADD(at[59], at[99]);    MULADD(at[60], at[98]);    MULADD(at[61], at[97]);    MULADD(at[62], at[96]);    MULADD(at[63], at[95]); 
   COMBA_STORE(C->dp[94]);
   /* 95 */
   COMBA_FORWARD;
   MULADD(at[32], at[127]);    MULADD(at[33], at[126]);    MULADD(at[34], at[125]);    MULADD(at[35], at[124]);    MULADD(at[36], at[123]);    MULADD(at[37], at[122]);    MULADD(at[38], at[121]);    MULADD(at[39], at[120]);    MULADD(at[40], at[119]);    MULADD(at[41], at[118]);    MULADD(at[42], at[117]);    MULADD(at[43], at[116]);    MULADD(at[44], at[115]);    MULADD(at[45], at[114]);    MULADD(at[46], at[113]);    MULADD(at[47], at[112]);    MULADD(at[48], at[111]);    MULADD(at[49], at[110]);    MULADD(at[50], at[109]);    MULADD(at[51], at[108]);    MULADD(at[52], at[107]);    MULADD(at[53], at[106]);    MULADD(at[54], at[105]);    MULADD(at[55], at[104]);    MULADD(at[56], at[103]);    MULADD(at[57], at[102]);    MULADD(at[58], at[101]);    MULADD(at[59], at[100]);    MULADD(at[60], at[99]);    MULADD(at[61], at[98]);    MULADD(at[62], at[97]);    MULADD(at[63], at[96]); 
   COMBA_STORE(C->dp[95]);
   /* 96 */
   COMBA_FORWARD;
   MULADD(at[33], at[127]);    MULADD(at[34], at[126]);    MULADD(at[35], at[125]);    MULADD(at[36], at[124]);    MULADD(at[37], at[123]);    MULADD(at[38], at[122]);    MULADD(at[39], at[121]);    MULADD(at[40], at[120]);    MULADD(at[41], at[119]);    MULADD(at[42], at[118]);    MULADD(at[43], at[117]);    MULADD(at[44], at[116]);    MULADD(at[45], at[115]);    MULADD(at[46], at[114]);    MULADD(at[47], at[113]);    MULADD(at[48], at[112]);    MULADD(at[49], at[111]);    MULADD(at[50], at[110]);    MULADD(at[51], at[109]);    MULADD(at[52], at[108]);    MULADD(at[53], at[107]);    MULADD(at[54], at[106]);    MULADD(at[55], at[105]);    MULADD(at[56], at[104]);    MULADD(at[57], at[103]);    MULADD(at[58], at[102]);    MULADD(at[59], at[101]);    MULADD(at[60], at[100]);    MULADD(at[61], at[99]);    MULADD(at[62], at[98]);    MULADD(at[63], at[97]); 
   COMBA_STORE(C->dp[96]);
   /* 97 */
   COMBA_FORWARD;
   MULADD(at[34], at[127]);    MULADD(at[35], at[126]);    MULADD(at[36], at[125]);    MULADD(at[37], at[124]);    MULADD(at[38], at[123]);    MULADD(at[39], at[122]);    MULADD(at[40], at[121]);    MULADD(at[41], at[120]);    MULADD(at[42], at[119]);    MULADD(at[43], at[118]);    MULADD(at[44], at[117]);    MULADD(at[45], at[116]);    MULADD(at[46], at[115]);    MULADD(at[47], at[114]);    MULADD(at[48], at[113]);    MULADD(at[49], at[112]);    MULADD(at[50], at[111]);    MULADD(at[51], at[110]);    MULADD(at[52], at[109]);    MULADD(at[53], at[108]);    MULADD(at[54], at[107]);    MULADD(at[55], at[106]);    MULADD(at[56], at[105]);    MULADD(at[57], at[104]);    MULADD(at[58], at[103]);    MULADD(at[59], at[102]);    MULADD(at[60], at[101]);    MULADD(at[61], at[100]);    MULADD(at[62], at[99]);    MULADD(at[63], at[98]); 
   COMBA_STORE(C->dp[97]);
   /* 98 */
   COMBA_FORWARD;
   MULADD(at[35], at[127]);    MULADD(at[36], at[126]);    MULADD(at[37], at[125]);    MULADD(at[38], at[124]);    MULADD(at[39], at[123]);    MULADD(at[40], at[122]);    MULADD(at[41], at[121]);    MULADD(at[42], at[120]);    MULADD(at[43], at[119]);    MULADD(at[44], at[118]);    MULADD(at[45], at[117]);    MULADD(at[46], at[116]);    MULADD(at[47], at[115]);    MULADD(at[48], at[114]);    MULADD(at[49], at[113]);    MULADD(at[50], at[112]);    MULADD(at[51], at[111]);    MULADD(at[52], at[110]);    MULADD(at[53], at[109]);    MULADD(at[54], at[108]);    MULADD(at[55], at[107]);    MULADD(at[56], at[106]);    MULADD(at[57], at[105]);    MULADD(at[58], at[104]);    MULADD(at[59], at[103]);    MULADD(at[60], at[102]);    MULADD(at[61], at[101]);    MULADD(at[62], at[100]);    MULADD(at[63], at[99]); 
   COMBA_STORE(C->dp[98]);
   /* 99 */
   COMBA_FORWARD;
   MULADD(at[36], at[127]);    MULADD(at[37], at[126]);    MULADD(at[38], at[125]);    MULADD(at[39], at[124]);    MULADD(at[40], at[123]);    MULADD(at[41], at[122]);    MULADD(at[42], at[121]);    MULADD(at[43], at[120]);    MULADD(at[44], at[119]);    MULADD(at[45], at[118]);    MULADD(at[46], at[117]);    MULADD(at[47], at[116]);    MULADD(at[48], at[115]);    MULADD(at[49], at[114]);    MULADD(at[50], at[113]);    MULADD(at[51], at[112]);    MULADD(at[52], at[111]);    MULADD(at[53], at[110]);    MULADD(at[54], at[109]);    MULADD(at[55], at[108]);    MULADD(at[56], at[107]);    MULADD(at[57], at[106]);    MULADD(at[58], at[105]);    MULADD(at[59], at[104]);    MULADD(at[60], at[103]);    MULADD(at[61], at[102]);    MULADD(at[62], at[101]);    MULADD(at[63], at[100]); 
   COMBA_STORE(C->dp[99]);
   /* 100 */
   COMBA_FORWARD;
   MULADD(at[37], at[127]);    MULADD(at[38], at[126]);    MULADD(at[39], at[125]);    MULADD(at[40], at[124]);    MULADD(at[41], at[123]);    MULADD(at[42], at[122]);    MULADD(at[43], at[121]);    MULADD(at[44], at[120]);    MULADD(at[45], at[119]);    MULADD(at[46], at[118]);    MULADD(at[47], at[117]);    MULADD(at[48], at[116]);    MULADD(at[49], at[115]);    MULADD(at[50], at[114]);    MULADD(at[51], at[113]);    MULADD(at[52], at[112]);    MULADD(at[53], at[111]);    MULADD(at[54], at[110]);    MULADD(at[55], at[109]);    MULADD(at[56], at[108]);    MULADD(at[57], at[107]);    MULADD(at[58], at[106]);    MULADD(at[59], at[105]);    MULADD(at[60], at[104]);    MULADD(at[61], at[103]);    MULADD(at[62], at[102]);    MULADD(at[63], at[101]); 
   COMBA_STORE(C->dp[100]);
   /* 101 */
   COMBA_FORWARD;
   MULADD(at[38], at[127]);    MULADD(at[39], at[126]);    MULADD(at[40], at[125]);    MULADD(at[41], at[124]);    MULADD(at[42], at[123]);    MULADD(at[43], at[122]);    MULADD(at[44], at[121]);    MULADD(at[45], at[120]);    MULADD(at[46], at[119]);    MULADD(at[47], at[118]);    MULADD(at[48], at[117]);    MULADD(at[49], at[116]);    MULADD(at[50], at[115]);    MULADD(at[51], at[114]);    MULADD(at[52], at[113]);    MULADD(at[53], at[112]);    MULADD(at[54], at[111]);    MULADD(at[55], at[110]);    MULADD(at[56], at[109]);    MULADD(at[57], at[108]);    MULADD(at[58], at[107]);    MULADD(at[59], at[106]);    MULADD(at[60], at[105]);    MULADD(at[61], at[104]);    MULADD(at[62], at[103]);    MULADD(at[63], at[102]); 
   COMBA_STORE(C->dp[101]);
   /* 102 */
   COMBA_FORWARD;
   MULADD(at[39], at[127]);    MULADD(at[40], at[126]);    MULADD(at[41], at[125]);    MULADD(at[42], at[124]);    MULADD(at[43], at[123]);    MULADD(at[44], at[122]);    MULADD(at[45], at[121]);    MULADD(at[46], at[120]);    MULADD(at[47], at[119]);    MULADD(at[48], at[118]);    MULADD(at[49], at[117]);    MULADD(at[50], at[116]);    MULADD(at[51], at[115]);    MULADD(at[52], at[114]);    MULADD(at[53], at[113]);    MULADD(at[54], at[112]);    MULADD(at[55], at[111]);    MULADD(at[56], at[110]);    MULADD(at[57], at[109]);    MULADD(at[58], at[108]);    MULADD(at[59], at[107]);    MULADD(at[60], at[106]);    MULADD(at[61], at[105]);    MULADD(at[62], at[104]);    MULADD(at[63], at[103]); 
   COMBA_STORE(C->dp[102]);
   /* 103 */
   COMBA_FORWARD;
   MULADD(at[40], at[127]);    MULADD(at[41], at[126]);    MULADD(at[42], at[125]);    MULADD(at[43], at[124]);    MULADD(at[44], at[123]);    MULADD(at[45], at[122]);    MULADD(at[46], at[121]);    MULADD(at[47], at[120]);    MULADD(at[48], at[119]);    MULADD(at[49], at[118]);    MULADD(at[50], at[117]);    MULADD(at[51], at[116]);    MULADD(at[52], at[115]);    MULADD(at[53], at[114]);    MULADD(at[54], at[113]);    MULADD(at[55], at[112]);    MULADD(at[56], at[111]);    MULADD(at[57], at[110]);    MULADD(at[58], at[109]);    MULADD(at[59], at[108]);    MULADD(at[60], at[107]);    MULADD(at[61], at[106]);    MULADD(at[62], at[105]);    MULADD(at[63], at[104]); 
   COMBA_STORE(C->dp[103]);
   /* 104 */
   COMBA_FORWARD;
   MULADD(at[41], at[127]);    MULADD(at[42], at[126]);    MULADD(at[43], at[125]);    MULADD(at[44], at[124]);    MULADD(at[45], at[123]);    MULADD(at[46], at[122]);    MULADD(at[47], at[121]);    MULADD(at[48], at[120]);    MULADD(at[49], at[119]);    MULADD(at[50], at[118]);    MULADD(at[51], at[117]);    MULADD(at[52], at[116]);    MULADD(at[53], at[115]);    MULADD(at[54], at[114]);    MULADD(at[55], at[113]);    MULADD(at[56], at[112]);    MULADD(at[57], at[111]);    MULADD(at[58], at[110]);    MULADD(at[59], at[109]);    MULADD(at[60], at[108]);    MULADD(at[61], at[107]);    MULADD(at[62], at[106]);    MULADD(at[63], at[105]); 
   COMBA_STORE(C->dp[104]);
   /* 105 */
   COMBA_FORWARD;
   MULADD(at[42], at[127]);    MULADD(at[43], at[126]);    MULADD(at[44], at[125]);    MULADD(at[45], at[124]);    MULADD(at[46], at[123]);    MULADD(at[47], at[122]);    MULADD(at[48], at[121]);    MULADD(at[49], at[120]);    MULADD(at[50], at[119]);    MULADD(at[51], at[118]);    MULADD(at[52], at[117]);    MULADD(at[53], at[116]);    MULADD(at[54], at[115]);    MULADD(at[55], at[114]);    MULADD(at[56], at[113]);    MULADD(at[57], at[112]);    MULADD(at[58], at[111]);    MULADD(at[59], at[110]);    MULADD(at[60], at[109]);    MULADD(at[61], at[108]);    MULADD(at[62], at[107]);    MULADD(at[63], at[106]); 
   COMBA_STORE(C->dp[105]);
   /* 106 */
   COMBA_FORWARD;
   MULADD(at[43], at[127]);    MULADD(at[44], at[126]);    MULADD(at[45], at[125]);    MULADD(at[46], at[124]);    MULADD(at[47], at[123]);    MULADD(at[48], at[122]);    MULADD(at[49], at[121]);    MULADD(at[50], at[120]);    MULADD(at[51], at[119]);    MULADD(at[52], at[118]);    MULADD(at[53], at[117]);    MULADD(at[54], at[116]);    MULADD(at[55], at[115]);    MULADD(at[56], at[114]);    MULADD(at[57], at[113]);    MULADD(at[58], at[112]);    MULADD(at[59], at[111]);    MULADD(at[60], at[110]);    MULADD(at[61], at[109]);    MULADD(at[62], at[108]);    MULADD(at[63], at[107]); 
   COMBA_STORE(C->dp[106]);
   /* 107 */
   COMBA_FORWARD;
   MULADD(at[44], at[127]);    MULADD(at[45], at[126]);    MULADD(at[46], at[125]);    MULADD(at[47], at[124]);    MULADD(at[48], at[123]);    MULADD(at[49], at[122]);    MULADD(at[50], at[121]);    MULADD(at[51], at[120]);    MULADD(at[52], at[119]);    MULADD(at[53], at[118]);    MULADD(at[54], at[117]);    MULADD(at[55], at[116]);    MULADD(at[56], at[115]);    MULADD(at[57], at[114]);    MULADD(at[58], at[113]);    MULADD(at[59], at[112]);    MULADD(at[60], at[111]);    MULADD(at[61], at[110]);    MULADD(at[62], at[109]);    MULADD(at[63], at[108]); 
   COMBA_STORE(C->dp[107]);
   /* 108 */
   COMBA_FORWARD;
   MULADD(at[45], at[127]);    MULADD(at[46], at[126]);    MULADD(at[47], at[125]);    MULADD(at[48], at[124]);    MULADD(at[49], at[123]);    MULADD(at[50], at[122]);    MULADD(at[51], at[121]);    MULADD(at[52], at[120]);    MULADD(at[53], at[119]);    MULADD(at[54], at[118]);    MULADD(at[55], at[117]);    MULADD(at[56], at[116]);    MULADD(at[57], at[115]);    MULADD(at[58], at[114]);    MULADD(at[59], at[113]);    MULADD(at[60], at[112]);    MULADD(at[61], at[111]);    MULADD(at[62], at[110]);    MULADD(at[63], at[109]); 
   COMBA_STORE(C->dp[108]);
   /* 109 */
   COMBA_FORWARD;
   MULADD(at[46], at[127]);    MULADD(at[47], at[126]);    MULADD(at[48], at[125]);    MULADD(at[49], at[124]);    MULADD(at[50], at[123]);    MULADD(at[51], at[122]);    MULADD(at[52], at[121]);    MULADD(at[53], at[120]);    MULADD(at[54], at[119]);    MULADD(at[55], at[118]);    MULADD(at[56], at[117]);    MULADD(at[57], at[116]);    MULADD(at[58], at[115]);    MULADD(at[59], at[114]);    MULADD(at[60], at[113]);    MULADD(at[61], at[112]);    MULADD(at[62], at[111]);    MULADD(at[63], at[110]); 
   COMBA_STORE(C->dp[109]);
   /* 110 */
   COMBA_FORWARD;
   MULADD(at[47], at[127]);    MULADD(at[48], at[126]);    MULADD(at[49], at[125]);    MULADD(at[50], at[124]);    MULADD(at[51], at[123]);    MULADD(at[52], at[122]);    MULADD(at[53], at[121]);    MULADD(at[54], at[120]);    MULADD(at[55], at[119]);    MULADD(at[56], at[118]);    MULADD(at[57], at[117]);    MULADD(at[58], at[116]);    MULADD(at[59], at[115]);    MULADD(at[60], at[114]);    MULADD(at[61], at[113]);    MULADD(at[62], at[112]);    MULADD(at[63], at[111]); 
   COMBA_STORE(C->dp[110]);
   /* 111 */
   COMBA_FORWARD;
   MULADD(at[48], at[127]);    MULADD(at[49], at[126]);    MULADD(at[50], at[125]);    MULADD(at[51], at[124]);    MULADD(at[52], at[123]);    MULADD(at[53], at[122]);    MULADD(at[54], at[121]);    MULADD(at[55], at[120]);    MULADD(at[56], at[119]);    MULADD(at[57], at[118]);    MULADD(at[58], at[117]);    MULADD(at[59], at[116]);    MULADD(at[60], at[115]);    MULADD(at[61], at[114]);    MULADD(at[62], at[113]);    MULADD(at[63], at[112]); 
   COMBA_STORE(C->dp[111]);
   /* 112 */
   COMBA_FORWARD;
   MULADD(at[49], at[127]);    MULADD(at[50], at[126]);    MULADD(at[51], at[125]);    MULADD(at[52], at[124]);    MULADD(at[53], at[123]);    MULADD(at[54], at[122]);    MULADD(at[55], at[121]);    MULADD(at[56], at[120]);    MULADD(at[57], at[119]);    MULADD(at[58], at[118]);    MULADD(at[59], at[117]);    MULADD(at[60], at[116]);    MULADD(at[61], at[115]);    MULADD(at[62], at[114]);    MULADD(at[63], at[113]); 
   COMBA_STORE(C->dp[112]);
   /* 113 */
   COMBA_FORWARD;
   MULADD(at[50], at[127]);    MULADD(at[51], at[126]);    MULADD(at[52], at[125]);    MULADD(at[53], at[124]);    MULADD(at[54], at[123]);    MULADD(at[55], at[122]);    MULADD(at[56], at[121]);    MULADD(at[57], at[120]);    MULADD(at[58], at[119]);    MULADD(at[59], at[118]);    MULADD(at[60], at[117]);    MULADD(at[61], at[116]);    MULADD(at[62], at[115]);    MULADD(at[63], at[114]); 
   COMBA_STORE(C->dp[113]);
   /* 114 */
   COMBA_FORWARD;
   MULADD(at[51], at[127]);    MULADD(at[52], at[126]);    MULADD(at[53], at[125]);    MULADD(at[54], at[124]);    MULADD(at[55], at[123]);    MULADD(at[56], at[122]);    MULADD(at[57], at[121]);    MULADD(at[58], at[120]);    MULADD(at[59], at[119]);    MULADD(at[60], at[118]);    MULADD(at[61], at[117]);    MULADD(at[62], at[116]);    MULADD(at[63], at[115]); 
   COMBA_STORE(C->dp[114]);
   /* 115 */
   COMBA_FORWARD;
   MULADD(at[52], at[127]);    MULADD(at[53], at[126]);    MULADD(at[54], at[125]);    MULADD(at[55], at[124]);    MULADD(at[56], at[123]);    MULADD(at[57], at[122]);    MULADD(at[58], at[121]);    MULADD(at[59], at[120]);    MULADD(at[60], at[119]);    MULADD(at[61], at[118]);    MULADD(at[62], at[117]);    MULADD(at[63], at[116]); 
   COMBA_STORE(C->dp[115]);
   /* 116 */
   COMBA_FORWARD;
   MULADD(at[53], at[127]);    MULADD(at[54], at[126]);    MULADD(at[55], at[125]);    MULADD(at[56], at[124]);    MULADD(at[57], at[123]);    MULADD(at[58], at[122]);    MULADD(at[59], at[121]);    MULADD(at[60], at[120]);    MULADD(at[61], at[119]);    MULADD(at[62], at[118]);    MULADD(at[63], at[117]); 
   COMBA_STORE(C->dp[116]);
   /* 117 */
   COMBA_FORWARD;
   MULADD(at[54], at[127]);    MULADD(at[55], at[126]);    MULADD(at[56], at[125]);    MULADD(at[57], at[124]);    MULADD(at[58], at[123]);    MULADD(at[59], at[122]);    MULADD(at[60], at[121]);    MULADD(at[61], at[120]);    MULADD(at[62], at[119]);    MULADD(at[63], at[118]); 
   COMBA_STORE(C->dp[117]);
   /* 118 */
   COMBA_FORWARD;
   MULADD(at[55], at[127]);    MULADD(at[56], at[126]);    MULADD(at[57], at[125]);    MULADD(at[58], at[124]);    MULADD(at[59], at[123]);    MULADD(at[60], at[122]);    MULADD(at[61], at[121]);    MULADD(at[62], at[120]);    MULADD(at[63], at[119]); 
   COMBA_STORE(C->dp[118]);
   /* 119 */
   COMBA_FORWARD;
   MULADD(at[56], at[127]);    MULADD(at[57], at[126]);    MULADD(at[58], at[125]);    MULADD(at[59], at[124]);    MULADD(at[60], at[123]);    MULADD(at[61], at[122]);    MULADD(at[62], at[121]);    MULADD(at[63], at[120]); 
   COMBA_STORE(C->dp[119]);
   /* 120 */
   COMBA_FORWARD;
   MULADD(at[57], at[127]);    MULADD(at[58], at[126]);    MULADD(at[59], at[125]);    MULADD(at[60], at[124]);    MULADD(at[61], at[123]);    MULADD(at[62], at[122]);    MULADD(at[63], at[121]); 
   COMBA_STORE(C->dp[120]);
   /* 121 */
   COMBA_FORWARD;
   MULADD(at[58], at[127]);    MULADD(at[59], at[126]);    MULADD(at[60], at[125]);    MULADD(at[61], at[124]);    MULADD(at[62], at[123]);    MULADD(at[63], at[122]); 
   COMBA_STORE(C->dp[121]);
   /* 122 */
   COMBA_FORWARD;
   MULADD(at[59], at[127]);    MULADD(at[60], at[126]);    MULADD(at[61], at[125]);    MULADD(at[62], at[124]);    MULADD(at[63], at[123]); 
   COMBA_STORE(C->dp[122]);
   /* 123 */
   COMBA_FORWARD;
   MULADD(at[60], at[127]);    MULADD(at[61], at[126]);    MULADD(at[62], at[125]);    MULADD(at[63], at[124]); 
   COMBA_STORE(C->dp[123]);
   /* 124 */
   COMBA_FORWARD;
   MULADD(at[61], at[127]);    MULADD(at[62], at[126]);    MULADD(at[63], at[125]); 
   COMBA_STORE(C->dp[124]);
   /* 125 */
   COMBA_FORWARD;
   MULADD(at[62], at[127]);    MULADD(at[63], at[126]); 
   COMBA_STORE(C->dp[125]);
   /* 126 */
   COMBA_FORWARD;
   MULADD(at[63], at[127]); 
   COMBA_STORE(C->dp[126]);
   COMBA_STORE2(C->dp[127]);
   C->used = 128;
   C->sign = A->sign ^ B->sign;
   fp_clamp(C);
   COMBA_FINI;
}
#endif

#ifdef TFM_MUL48
void fp_mul_comba48(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[96];

   memcpy(at, A->dp, 48 * sizeof(fp_digit));
   memcpy(at+48, B->dp, 48 * sizeof(fp_digit));
   COMBA_START;

   COMBA_CLEAR;
   /* 0 */
   MULADD(at[0], at[48]); 
   COMBA_STORE(C->dp[0]);
   /* 1 */
   COMBA_FORWARD;
   MULADD(at[0], at[49]);    MULADD(at[1], at[48]); 
   COMBA_STORE(C->dp[1]);
   /* 2 */
   COMBA_FORWARD;
   MULADD(at[0], at[50]);    MULADD(at[1], at[49]);    MULADD(at[2], at[48]); 
   COMBA_STORE(C->dp[2]);
   /* 3 */
   COMBA_FORWARD;
   MULADD(at[0], at[51]);    MULADD(at[1], at[50]);    MULADD(at[2], at[49]);    MULADD(at[3], at[48]); 
   COMBA_STORE(C->dp[3]);
   /* 4 */
   COMBA_FORWARD;
   MULADD(at[0], at[52]);    MULADD(at[1], at[51]);    MULADD(at[2], at[50]);    MULADD(at[3], at[49]);    MULADD(at[4], at[48]); 
   COMBA_STORE(C->dp[4]);
   /* 5 */
   COMBA_FORWARD;
   MULADD(at[0], at[53]);    MULADD(at[1], at[52]);    MULADD(at[2], at[51]);    MULADD(at[3], at[50]);    MULADD(at[4], at[49]);    MULADD(at[5], at[48]); 
   COMBA_STORE(C->dp[5]);
   /* 6 */
   COMBA_FORWARD;
   MULADD(at[0], at[54]);    MULADD(at[1], at[53]);    MULADD(at[2], at[52]);    MULADD(at[3], at[51]);    MULADD(at[4], at[50]);    MULADD(at[5], at[49]);    MULADD(at[6], at[48]); 
   COMBA_STORE(C->dp[6]);
   /* 7 */
   COMBA_FORWARD;
   MULADD(at[0], at[55]);    MULADD(at[1], at[54]);    MULADD(at[2], at[53]);    MULADD(at[3], at[52]);    MULADD(at[4], at[51]);    MULADD(at[5], at[50]);    MULADD(at[6], at[49]);    MULADD(at[7], at[48]); 
   COMBA_STORE(C->dp[7]);
   /* 8 */
   COMBA_FORWARD;
   MULADD(at[0], at[56]);    MULADD(at[1], at[55]);    MULADD(at[2], at[54]);    MULADD(at[3], at[53]);    MULADD(at[4], at[52]);    MULADD(at[5], at[51]);    MULADD(at[6], at[50]);    MULADD(at[7], at[49]);    MULADD(at[8], at[48]); 
   COMBA_STORE(C->dp[8]);
   /* 9 */
   COMBA_FORWARD;
   MULADD(at[0], at[57]);    MULADD(at[1], at[56]);    MULADD(at[2], at[55]);    MULADD(at[3], at[54]);    MULADD(at[4], at[53]);    MULADD(at[5], at[52]);    MULADD(at[6], at[51]);    MULADD(at[7], at[50]);    MULADD(at[8], at[49]);    MULADD(at[9], at[48]); 
   COMBA_STORE(C->dp[9]);
   /* 10 */
   COMBA_FORWARD;
   MULADD(at[0], at[58]);    MULADD(at[1], at[57]);    MULADD(at[2], at[56]);    MULADD(at[3], at[55]);    MULADD(at[4], at[54]);    MULADD(at[5], at[53]);    MULADD(at[6], at[52]);    MULADD(at[7], at[51]);    MULADD(at[8], at[50]);    MULADD(at[9], at[49]);    MULADD(at[10], at[48]); 
   COMBA_STORE(C->dp[10]);
   /* 11 */
   COMBA_FORWARD;
   MULADD(at[0], at[59]);    MULADD(at[1], at[58]);    MULADD(at[2], at[57]);    MULADD(at[3], at[56]);    MULADD(at[4], at[55]);    MULADD(at[5], at[54]);    MULADD(at[6], at[53]);    MULADD(at[7], at[52]);    MULADD(at[8], at[51]);    MULADD(at[9], at[50]);    MULADD(at[10], at[49]);    MULADD(at[11], at[48]); 
   COMBA_STORE(C->dp[11]);
   /* 12 */
   COMBA_FORWARD;
   MULADD(at[0], at[60]);    MULADD(at[1], at[59]);    MULADD(at[2], at[58]);    MULADD(at[3], at[57]);    MULADD(at[4], at[56]);    MULADD(at[5], at[55]);    MULADD(at[6], at[54]);    MULADD(at[7], at[53]);    MULADD(at[8], at[52]);    MULADD(at[9], at[51]);    MULADD(at[10], at[50]);    MULADD(at[11], at[49]);    MULADD(at[12], at[48]); 
   COMBA_STORE(C->dp[12]);
   /* 13 */
   COMBA_FORWARD;
   MULADD(at[0], at[61]);    MULADD(at[1], at[60]);    MULADD(at[2], at[59]);    MULADD(at[3], at[58]);    MULADD(at[4], at[57]);    MULADD(at[5], at[56]);    MULADD(at[6], at[55]);    MULADD(at[7], at[54]);    MULADD(at[8], at[53]);    MULADD(at[9], at[52]);    MULADD(at[10], at[51]);    MULADD(at[11], at[50]);    MULADD(at[12], at[49]);    MULADD(at[13], at[48]); 
   COMBA_STORE(C->dp[13]);
   /* 14 */
   COMBA_FORWARD;
   MULADD(at[0], at[62]);    MULADD(at[1], at[61]);    MULADD(at[2], at[60]);    MULADD(at[3], at[59]);    MULADD(at[4], at[58]);    MULADD(at[5], at[57]);    MULADD(at[6], at[56]);    MULADD(at[7], at[55]);    MULADD(at[8], at[54]);    MULADD(at[9], at[53]);    MULADD(at[10], at[52]);    MULADD(at[11], at[51]);    MULADD(at[12], at[50]);    MULADD(at[13], at[49]);    MULADD(at[14], at[48]); 
   COMBA_STORE(C->dp[14]);
   /* 15 */
   COMBA_FORWARD;
   MULADD(at[0], at[63]);    MULADD(at[1], at[62]);    MULADD(at[2], at[61]);    MULADD(at[3], at[60]);    MULADD(at[4], at[59]);    MULADD(at[5], at[58]);    MULADD(at[6], at[57]);    MULADD(at[7], at[56]);    MULADD(at[8], at[55]);    MULADD(at[9], at[54]);    MULADD(at[10], at[53]);    MULADD(at[11], at[52]);    MULADD(at[12], at[51]);    MULADD(at[13], at[50]);    MULADD(at[14], at[49]);    MULADD(at[15], at[48]); 
   COMBA_STORE(C->dp[15]);
   /* 16 */
   COMBA_FORWARD;
   MULADD(at[0], at[64]);    MULADD(at[1], at[63]);    MULADD(at[2], at[62]);    MULADD(at[3], at[61]);    MULADD(at[4], at[60]);    MULADD(at[5], at[59]);    MULADD(at[6], at[58]);    MULADD(at[7], at[57]);    MULADD(at[8], at[56]);    MULADD(at[9], at[55]);    MULADD(at[10], at[54]);    MULADD(at[11], at[53]);    MULADD(at[12], at[52]);    MULADD(at[13], at[51]);    MULADD(at[14], at[50]);    MULADD(at[15], at[49]);    MULADD(at[16], at[48]); 
   COMBA_STORE(C->dp[16]);
   /* 17 */
   COMBA_FORWARD;
   MULADD(at[0], at[65]);    MULADD(at[1], at[64]);    MULADD(at[2], at[63]);    MULADD(at[3], at[62]);    MULADD(at[4], at[61]);    MULADD(at[5], at[60]);    MULADD(at[6], at[59]);    MULADD(at[7], at[58]);    MULADD(at[8], at[57]);    MULADD(at[9], at[56]);    MULADD(at[10], at[55]);    MULADD(at[11], at[54]);    MULADD(at[12], at[53]);    MULADD(at[13], at[52]);    MULADD(at[14], at[51]);    MULADD(at[15], at[50]);    MULADD(at[16], at[49]);    MULADD(at[17], at[48]); 
   COMBA_STORE(C->dp[17]);
   /* 18 */
   COMBA_FORWARD;
   MULADD(at[0], at[66]);    MULADD(at[1], at[65]);    MULADD(at[2], at[64]);    MULADD(at[3], at[63]);    MULADD(at[4], at[62]);    MULADD(at[5], at[61]);    MULADD(at[6], at[60]);    MULADD(at[7], at[59]);    MULADD(at[8], at[58]);    MULADD(at[9], at[57]);    MULADD(at[10], at[56]);    MULADD(at[11], at[55]);    MULADD(at[12], at[54]);    MULADD(at[13], at[53]);    MULADD(at[14], at[52]);    MULADD(at[15], at[51]);    MULADD(at[16], at[50]);    MULADD(at[17], at[49]);    MULADD(at[18], at[48]); 
   COMBA_STORE(C->dp[18]);
   /* 19 */
   COMBA_FORWARD;
   MULADD(at[0], at[67]);    MULADD(at[1], at[66]);    MULADD(at[2], at[65]);    MULADD(at[3], at[64]);    MULADD(at[4], at[63]);    MULADD(at[5], at[62]);    MULADD(at[6], at[61]);    MULADD(at[7], at[60]);    MULADD(at[8], at[59]);    MULADD(at[9], at[58]);    MULADD(at[10], at[57]);    MULADD(at[11], at[56]);    MULADD(at[12], at[55]);    MULADD(at[13], at[54]);    MULADD(at[14], at[53]);    MULADD(at[15], at[52]);    MULADD(at[16], at[51]);    MULADD(at[17], at[50]);    MULADD(at[18], at[49]);    MULADD(at[19], at[48]); 
   COMBA_STORE(C->dp[19]);
   /* 20 */
   COMBA_FORWARD;
   MULADD(at[0], at[68]);    MULADD(at[1], at[67]);    MULADD(at[2], at[66]);    MULADD(at[3], at[65]);    MULADD(at[4], at[64]);    MULADD(at[5], at[63]);    MULADD(at[6], at[62]);    MULADD(at[7], at[61]);    MULADD(at[8], at[60]);    MULADD(at[9], at[59]);    MULADD(at[10], at[58]);    MULADD(at[11], at[57]);    MULADD(at[12], at[56]);    MULADD(at[13], at[55]);    MULADD(at[14], at[54]);    MULADD(at[15], at[53]);    MULADD(at[16], at[52]);    MULADD(at[17], at[51]);    MULADD(at[18], at[50]);    MULADD(at[19], at[49]);    MULADD(at[20], at[48]); 
   COMBA_STORE(C->dp[20]);
   /* 21 */
   COMBA_FORWARD;
   MULADD(at[0], at[69]);    MULADD(at[1], at[68]);    MULADD(at[2], at[67]);    MULADD(at[3], at[66]);    MULADD(at[4], at[65]);    MULADD(at[5], at[64]);    MULADD(at[6], at[63]);    MULADD(at[7], at[62]);    MULADD(at[8], at[61]);    MULADD(at[9], at[60]);    MULADD(at[10], at[59]);    MULADD(at[11], at[58]);    MULADD(at[12], at[57]);    MULADD(at[13], at[56]);    MULADD(at[14], at[55]);    MULADD(at[15], at[54]);    MULADD(at[16], at[53]);    MULADD(at[17], at[52]);    MULADD(at[18], at[51]);    MULADD(at[19], at[50]);    MULADD(at[20], at[49]);    MULADD(at[21], at[48]); 
   COMBA_STORE(C->dp[21]);
   /* 22 */
   COMBA_FORWARD;
   MULADD(at[0], at[70]);    MULADD(at[1], at[69]);    MULADD(at[2], at[68]);    MULADD(at[3], at[67]);    MULADD(at[4], at[66]);    MULADD(at[5], at[65]);    MULADD(at[6], at[64]);    MULADD(at[7], at[63]);    MULADD(at[8], at[62]);    MULADD(at[9], at[61]);    MULADD(at[10], at[60]);    MULADD(at[11], at[59]);    MULADD(at[12], at[58]);    MULADD(at[13], at[57]);    MULADD(at[14], at[56]);    MULADD(at[15], at[55]);    MULADD(at[16], at[54]);    MULADD(at[17], at[53]);    MULADD(at[18], at[52]);    MULADD(at[19], at[51]);    MULADD(at[20], at[50]);    MULADD(at[21], at[49]);    MULADD(at[22], at[48]); 
   COMBA_STORE(C->dp[22]);
   /* 23 */
   COMBA_FORWARD;
   MULADD(at[0], at[71]);    MULADD(at[1], at[70]);    MULADD(at[2], at[69]);    MULADD(at[3], at[68]);    MULADD(at[4], at[67]);    MULADD(at[5], at[66]);    MULADD(at[6], at[65]);    MULADD(at[7], at[64]);    MULADD(at[8], at[63]);    MULADD(at[9], at[62]);    MULADD(at[10], at[61]);    MULADD(at[11], at[60]);    MULADD(at[12], at[59]);    MULADD(at[13], at[58]);    MULADD(at[14], at[57]);    MULADD(at[15], at[56]);    MULADD(at[16], at[55]);    MULADD(at[17], at[54]);    MULADD(at[18], at[53]);    MULADD(at[19], at[52]);    MULADD(at[20], at[51]);    MULADD(at[21], at[50]);    MULADD(at[22], at[49]);    MULADD(at[23], at[48]); 
   COMBA_STORE(C->dp[23]);
   /* 24 */
   COMBA_FORWARD;
   MULADD(at[0], at[72]);    MULADD(at[1], at[71]);    MULADD(at[2], at[70]);    MULADD(at[3], at[69]);    MULADD(at[4], at[68]);    MULADD(at[5], at[67]);    MULADD(at[6], at[66]);    MULADD(at[7], at[65]);    MULADD(at[8], at[64]);    MULADD(at[9], at[63]);    MULADD(at[10], at[62]);    MULADD(at[11], at[61]);    MULADD(at[12], at[60]);    MULADD(at[13], at[59]);    MULADD(at[14], at[58]);    MULADD(at[15], at[57]);    MULADD(at[16], at[56]);    MULADD(at[17], at[55]);    MULADD(at[18], at[54]);    MULADD(at[19], at[53]);    MULADD(at[20], at[52]);    MULADD(at[21], at[51]);    MULADD(at[22], at[50]);    MULADD(at[23], at[49]);    MULADD(at[24], at[48]); 
   COMBA_STORE(C->dp[24]);
   /* 25 */
   COMBA_FORWARD;
   MULADD(at[0], at[73]);    MULADD(at[1], at[72]);    MULADD(at[2], at[71]);    MULADD(at[3], at[70]);    MULADD(at[4], at[69]);    MULADD(at[5], at[68]);    MULADD(at[6], at[67]);    MULADD(at[7], at[66]);    MULADD(at[8], at[65]);    MULADD(at[9], at[64]);    MULADD(at[10], at[63]);    MULADD(at[11], at[62]);    MULADD(at[12], at[61]);    MULADD(at[13], at[60]);    MULADD(at[14], at[59]);    MULADD(at[15], at[58]);    MULADD(at[16], at[57]);    MULADD(at[17], at[56]);    MULADD(at[18], at[55]);    MULADD(at[19], at[54]);    MULADD(at[20], at[53]);    MULADD(at[21], at[52]);    MULADD(at[22], at[51]);    MULADD(at[23], at[50]);    MULADD(at[24], at[49]);    MULADD(at[25], at[48]); 
   COMBA_STORE(C->dp[25]);
   /* 26 */
   COMBA_FORWARD;
   MULADD(at[0], at[74]);    MULADD(at[1], at[73]);    MULADD(at[2], at[72]);    MULADD(at[3], at[71]);    MULADD(at[4], at[70]);    MULADD(at[5], at[69]);    MULADD(at[6], at[68]);    MULADD(at[7], at[67]);    MULADD(at[8], at[66]);    MULADD(at[9], at[65]);    MULADD(at[10], at[64]);    MULADD(at[11], at[63]);    MULADD(at[12], at[62]);    MULADD(at[13], at[61]);    MULADD(at[14], at[60]);    MULADD(at[15], at[59]);    MULADD(at[16], at[58]);    MULADD(at[17], at[57]);    MULADD(at[18], at[56]);    MULADD(at[19], at[55]);    MULADD(at[20], at[54]);    MULADD(at[21], at[53]);    MULADD(at[22], at[52]);    MULADD(at[23], at[51]);    MULADD(at[24], at[50]);    MULADD(at[25], at[49]);    MULADD(at[26], at[48]); 
   COMBA_STORE(C->dp[26]);
   /* 27 */
   COMBA_FORWARD;
   MULADD(at[0], at[75]);    MULADD(at[1], at[74]);    MULADD(at[2], at[73]);    MULADD(at[3], at[72]);    MULADD(at[4], at[71]);    MULADD(at[5], at[70]);    MULADD(at[6], at[69]);    MULADD(at[7], at[68]);    MULADD(at[8], at[67]);    MULADD(at[9], at[66]);    MULADD(at[10], at[65]);    MULADD(at[11], at[64]);    MULADD(at[12], at[63]);    MULADD(at[13], at[62]);    MULADD(at[14], at[61]);    MULADD(at[15], at[60]);    MULADD(at[16], at[59]);    MULADD(at[17], at[58]);    MULADD(at[18], at[57]);    MULADD(at[19], at[56]);    MULADD(at[20], at[55]);    MULADD(at[21], at[54]);    MULADD(at[22], at[53]);    MULADD(at[23], at[52]);    MULADD(at[24], at[51]);    MULADD(at[25], at[50]);    MULADD(at[26], at[49]);    MULADD(at[27], at[48]); 
   COMBA_STORE(C->dp[27]);
   /* 28 */
   COMBA_FORWARD;
   MULADD(at[0], at[76]);    MULADD(at[1], at[75]);    MULADD(at[2], at[74]);    MULADD(at[3], at[73]);    MULADD(at[4], at[72]);    MULADD(at[5], at[71]);    MULADD(at[6], at[70]);    MULADD(at[7], at[69]);    MULADD(at[8], at[68]);    MULADD(at[9], at[67]);    MULADD(at[10], at[66]);    MULADD(at[11], at[65]);    MULADD(at[12], at[64]);    MULADD(at[13], at[63]);    MULADD(at[14], at[62]);    MULADD(at[15], at[61]);    MULADD(at[16], at[60]);    MULADD(at[17], at[59]);    MULADD(at[18], at[58]);    MULADD(at[19], at[57]);    MULADD(at[20], at[56]);    MULADD(at[21], at[55]);    MULADD(at[22], at[54]);    MULADD(at[23], at[53]);    MULADD(at[24], at[52]);    MULADD(at[25], at[51]);    MULADD(at[26], at[50]);    MULADD(at[27], at[49]);    MULADD(at[28], at[48]); 
   COMBA_STORE(C->dp[28]);
   /* 29 */
   COMBA_FORWARD;
   MULADD(at[0], at[77]);    MULADD(at[1], at[76]);    MULADD(at[2], at[75]);    MULADD(at[3], at[74]);    MULADD(at[4], at[73]);    MULADD(at[5], at[72]);    MULADD(at[6], at[71]);    MULADD(at[7], at[70]);    MULADD(at[8], at[69]);    MULADD(at[9], at[68]);    MULADD(at[10], at[67]);    MULADD(at[11], at[66]);    MULADD(at[12], at[65]);    MULADD(at[13], at[64]);    MULADD(at[14], at[63]);    MULADD(at[15], at[62]);    MULADD(at[16], at[61]);    MULADD(at[17], at[60]);    MULADD(at[18], at[59]);    MULADD(at[19], at[58]);    MULADD(at[20], at[57]);    MULADD(at[21], at[56]);    MULADD(at[22], at[55]);    MULADD(at[23], at[54]);    MULADD(at[24], at[53]);    MULADD(at[25], at[52]);    MULADD(at[26], at[51]);    MULADD(at[27], at[50]);    MULADD(at[28], at[49]);    MULADD(at[29], at[48]); 
   COMBA_STORE(C->dp[29]);
   /* 30 */
   COMBA_FORWARD;
   MULADD(at[0], at[78]);    MULADD(at[1], at[77]);    MULADD(at[2], at[76]);    MULADD(at[3], at[75]);    MULADD(at[4], at[74]);    MULADD(at[5], at[73]);    MULADD(at[6], at[72]);    MULADD(at[7], at[71]);    MULADD(at[8], at[70]);    MULADD(at[9], at[69]);    MULADD(at[10], at[68]);    MULADD(at[11], at[67]);    MULADD(at[12], at[66]);    MULADD(at[13], at[65]);    MULADD(at[14], at[64]);    MULADD(at[15], at[63]);    MULADD(at[16], at[62]);    MULADD(at[17], at[61]);    MULADD(at[18], at[60]);    MULADD(at[19], at[59]);    MULADD(at[20], at[58]);    MULADD(at[21], at[57]);    MULADD(at[22], at[56]);    MULADD(at[23], at[55]);    MULADD(at[24], at[54]);    MULADD(at[25], at[53]);    MULADD(at[26], at[52]);    MULADD(at[27], at[51]);    MULADD(at[28], at[50]);    MULADD(at[29], at[49]);    MULADD(at[30], at[48]); 
   COMBA_STORE(C->dp[30]);
   /* 31 */
   COMBA_FORWARD;
   MULADD(at[0], at[79]);    MULADD(at[1], at[78]);    MULADD(at[2], at[77]);    MULADD(at[3], at[76]);    MULADD(at[4], at[75]);    MULADD(at[5], at[74]);    MULADD(at[6], at[73]);    MULADD(at[7], at[72]);    MULADD(at[8], at[71]);    MULADD(at[9], at[70]);    MULADD(at[10], at[69]);    MULADD(at[11], at[68]);    MULADD(at[12], at[67]);    MULADD(at[13], at[66]);    MULADD(at[14], at[65]);    MULADD(at[15], at[64]);    MULADD(at[16], at[63]);    MULADD(at[17], at[62]);    MULADD(at[18], at[61]);    MULADD(at[19], at[60]);    MULADD(at[20], at[59]);    MULADD(at[21], at[58]);    MULADD(at[22], at[57]);    MULADD(at[23], at[56]);    MULADD(at[24], at[55]);    MULADD(at[25], at[54]);    MULADD(at[26], at[53]);    MULADD(at[27], at[52]);    MULADD(at[28], at[51]);    MULADD(at[29], at[50]);    MULADD(at[30], at[49]);    MULADD(at[31], at[48]); 
   COMBA_STORE(C->dp[31]);
   /* 32 */
   COMBA_FORWARD;
   MULADD(at[0], at[80]);    MULADD(at[1], at[79]);    MULADD(at[2], at[78]);    MULADD(at[3], at[77]);    MULADD(at[4], at[76]);    MULADD(at[5], at[75]);    MULADD(at[6], at[74]);    MULADD(at[7], at[73]);    MULADD(at[8], at[72]);    MULADD(at[9], at[71]);    MULADD(at[10], at[70]);    MULADD(at[11], at[69]);    MULADD(at[12], at[68]);    MULADD(at[13], at[67]);    MULADD(at[14], at[66]);    MULADD(at[15], at[65]);    MULADD(at[16], at[64]);    MULADD(at[17], at[63]);    MULADD(at[18], at[62]);    MULADD(at[19], at[61]);    MULADD(at[20], at[60]);    MULADD(at[21], at[59]);    MULADD(at[22], at[58]);    MULADD(at[23], at[57]);    MULADD(at[24], at[56]);    MULADD(at[25], at[55]);    MULADD(at[26], at[54]);    MULADD(at[27], at[53]);    MULADD(at[28], at[52]);    MULADD(at[29], at[51]);    MULADD(at[30], at[50]);    MULADD(at[31], at[49]);    MULADD(at[32], at[48]); 
   COMBA_STORE(C->dp[32]);
   /* 33 */
   COMBA_FORWARD;
   MULADD(at[0], at[81]);    MULADD(at[1], at[80]);    MULADD(at[2], at[79]);    MULADD(at[3], at[78]);    MULADD(at[4], at[77]);    MULADD(at[5], at[76]);    MULADD(at[6], at[75]);    MULADD(at[7], at[74]);    MULADD(at[8], at[73]);    MULADD(at[9], at[72]);    MULADD(at[10], at[71]);    MULADD(at[11], at[70]);    MULADD(at[12], at[69]);    MULADD(at[13], at[68]);    MULADD(at[14], at[67]);    MULADD(at[15], at[66]);    MULADD(at[16], at[65]);    MULADD(at[17], at[64]);    MULADD(at[18], at[63]);    MULADD(at[19], at[62]);    MULADD(at[20], at[61]);    MULADD(at[21], at[60]);    MULADD(at[22], at[59]);    MULADD(at[23], at[58]);    MULADD(at[24], at[57]);    MULADD(at[25], at[56]);    MULADD(at[26], at[55]);    MULADD(at[27], at[54]);    MULADD(at[28], at[53]);    MULADD(at[29], at[52]);    MULADD(at[30], at[51]);    MULADD(at[31], at[50]);    MULADD(at[32], at[49]);    MULADD(at[33], at[48]); 
   COMBA_STORE(C->dp[33]);
   /* 34 */
   COMBA_FORWARD;
   MULADD(at[0], at[82]);    MULADD(at[1], at[81]);    MULADD(at[2], at[80]);    MULADD(at[3], at[79]);    MULADD(at[4], at[78]);    MULADD(at[5], at[77]);    MULADD(at[6], at[76]);    MULADD(at[7], at[75]);    MULADD(at[8], at[74]);    MULADD(at[9], at[73]);    MULADD(at[10], at[72]);    MULADD(at[11], at[71]);    MULADD(at[12], at[70]);    MULADD(at[13], at[69]);    MULADD(at[14], at[68]);    MULADD(at[15], at[67]);    MULADD(at[16], at[66]);    MULADD(at[17], at[65]);    MULADD(at[18], at[64]);    MULADD(at[19], at[63]);    MULADD(at[20], at[62]);    MULADD(at[21], at[61]);    MULADD(at[22], at[60]);    MULADD(at[23], at[59]);    MULADD(at[24], at[58]);    MULADD(at[25], at[57]);    MULADD(at[26], at[56]);    MULADD(at[27], at[55]);    MULADD(at[28], at[54]);    MULADD(at[29], at[53]);    MULADD(at[30], at[52]);    MULADD(at[31], at[51]);    MULADD(at[32], at[50]);    MULADD(at[33], at[49]);    MULADD(at[34], at[48]); 
   COMBA_STORE(C->dp[34]);
   /* 35 */
   COMBA_FORWARD;
   MULADD(at[0], at[83]);    MULADD(at[1], at[82]);    MULADD(at[2], at[81]);    MULADD(at[3], at[80]);    MULADD(at[4], at[79]);    MULADD(at[5], at[78]);    MULADD(at[6], at[77]);    MULADD(at[7], at[76]);    MULADD(at[8], at[75]);    MULADD(at[9], at[74]);    MULADD(at[10], at[73]);    MULADD(at[11], at[72]);    MULADD(at[12], at[71]);    MULADD(at[13], at[70]);    MULADD(at[14], at[69]);    MULADD(at[15], at[68]);    MULADD(at[16], at[67]);    MULADD(at[17], at[66]);    MULADD(at[18], at[65]);    MULADD(at[19], at[64]);    MULADD(at[20], at[63]);    MULADD(at[21], at[62]);    MULADD(at[22], at[61]);    MULADD(at[23], at[60]);    MULADD(at[24], at[59]);    MULADD(at[25], at[58]);    MULADD(at[26], at[57]);    MULADD(at[27], at[56]);    MULADD(at[28], at[55]);    MULADD(at[29], at[54]);    MULADD(at[30], at[53]);    MULADD(at[31], at[52]);    MULADD(at[32], at[51]);    MULADD(at[33], at[50]);    MULADD(at[34], at[49]);    MULADD(at[35], at[48]); 
   COMBA_STORE(C->dp[35]);
   /* 36 */
   COMBA_FORWARD;
   MULADD(at[0], at[84]);    MULADD(at[1], at[83]);    MULADD(at[2], at[82]);    MULADD(at[3], at[81]);    MULADD(at[4], at[80]);    MULADD(at[5], at[79]);    MULADD(at[6], at[78]);    MULADD(at[7], at[77]);    MULADD(at[8], at[76]);    MULADD(at[9], at[75]);    MULADD(at[10], at[74]);    MULADD(at[11], at[73]);    MULADD(at[12], at[72]);    MULADD(at[13], at[71]);    MULADD(at[14], at[70]);    MULADD(at[15], at[69]);    MULADD(at[16], at[68]);    MULADD(at[17], at[67]);    MULADD(at[18], at[66]);    MULADD(at[19], at[65]);    MULADD(at[20], at[64]);    MULADD(at[21], at[63]);    MULADD(at[22], at[62]);    MULADD(at[23], at[61]);    MULADD(at[24], at[60]);    MULADD(at[25], at[59]);    MULADD(at[26], at[58]);    MULADD(at[27], at[57]);    MULADD(at[28], at[56]);    MULADD(at[29], at[55]);    MULADD(at[30], at[54]);    MULADD(at[31], at[53]);    MULADD(at[32], at[52]);    MULADD(at[33], at[51]);    MULADD(at[34], at[50]);    MULADD(at[35], at[49]);    MULADD(at[36], at[48]); 
   COMBA_STORE(C->dp[36]);
   /* 37 */
   COMBA_FORWARD;
   MULADD(at[0], at[85]);    MULADD(at[1], at[84]);    MULADD(at[2], at[83]);    MULADD(at[3], at[82]);    MULADD(at[4], at[81]);    MULADD(at[5], at[80]);    MULADD(at[6], at[79]);    MULADD(at[7], at[78]);    MULADD(at[8], at[77]);    MULADD(at[9], at[76]);    MULADD(at[10], at[75]);    MULADD(at[11], at[74]);    MULADD(at[12], at[73]);    MULADD(at[13], at[72]);    MULADD(at[14], at[71]);    MULADD(at[15], at[70]);    MULADD(at[16], at[69]);    MULADD(at[17], at[68]);    MULADD(at[18], at[67]);    MULADD(at[19], at[66]);    MULADD(at[20], at[65]);    MULADD(at[21], at[64]);    MULADD(at[22], at[63]);    MULADD(at[23], at[62]);    MULADD(at[24], at[61]);    MULADD(at[25], at[60]);    MULADD(at[26], at[59]);    MULADD(at[27], at[58]);    MULADD(at[28], at[57]);    MULADD(at[29], at[56]);    MULADD(at[30], at[55]);    MULADD(at[31], at[54]);    MULADD(at[32], at[53]);    MULADD(at[33], at[52]);    MULADD(at[34], at[51]);    MULADD(at[35], at[50]);    MULADD(at[36], at[49]);    MULADD(at[37], at[48]); 
   COMBA_STORE(C->dp[37]);
   /* 38 */
   COMBA_FORWARD;
   MULADD(at[0], at[86]);    MULADD(at[1], at[85]);    MULADD(at[2], at[84]);    MULADD(at[3], at[83]);    MULADD(at[4], at[82]);    MULADD(at[5], at[81]);    MULADD(at[6], at[80]);    MULADD(at[7], at[79]);    MULADD(at[8], at[78]);    MULADD(at[9], at[77]);    MULADD(at[10], at[76]);    MULADD(at[11], at[75]);    MULADD(at[12], at[74]);    MULADD(at[13], at[73]);    MULADD(at[14], at[72]);    MULADD(at[15], at[71]);    MULADD(at[16], at[70]);    MULADD(at[17], at[69]);    MULADD(at[18], at[68]);    MULADD(at[19], at[67]);    MULADD(at[20], at[66]);    MULADD(at[21], at[65]);    MULADD(at[22], at[64]);    MULADD(at[23], at[63]);    MULADD(at[24], at[62]);    MULADD(at[25], at[61]);    MULADD(at[26], at[60]);    MULADD(at[27], at[59]);    MULADD(at[28], at[58]);    MULADD(at[29], at[57]);    MULADD(at[30], at[56]);    MULADD(at[31], at[55]);    MULADD(at[32], at[54]);    MULADD(at[33], at[53]);    MULADD(at[34], at[52]);    MULADD(at[35], at[51]);    MULADD(at[36], at[50]);    MULADD(at[37], at[49]);    MULADD(at[38], at[48]); 
   COMBA_STORE(C->dp[38]);
   /* 39 */
   COMBA_FORWARD;
   MULADD(at[0], at[87]);    MULADD(at[1], at[86]);    MULADD(at[2], at[85]);    MULADD(at[3], at[84]);    MULADD(at[4], at[83]);    MULADD(at[5], at[82]);    MULADD(at[6], at[81]);    MULADD(at[7], at[80]);    MULADD(at[8], at[79]);    MULADD(at[9], at[78]);    MULADD(at[10], at[77]);    MULADD(at[11], at[76]);    MULADD(at[12], at[75]);    MULADD(at[13], at[74]);    MULADD(at[14], at[73]);    MULADD(at[15], at[72]);    MULADD(at[16], at[71]);    MULADD(at[17], at[70]);    MULADD(at[18], at[69]);    MULADD(at[19], at[68]);    MULADD(at[20], at[67]);    MULADD(at[21], at[66]);    MULADD(at[22], at[65]);    MULADD(at[23], at[64]);    MULADD(at[24], at[63]);    MULADD(at[25], at[62]);    MULADD(at[26], at[61]);    MULADD(at[27], at[60]);    MULADD(at[28], at[59]);    MULADD(at[29], at[58]);    MULADD(at[30], at[57]);    MULADD(at[31], at[56]);    MULADD(at[32], at[55]);    MULADD(at[33], at[54]);    MULADD(at[34], at[53]);    MULADD(at[35], at[52]);    MULADD(at[36], at[51]);    MULADD(at[37], at[50]);    MULADD(at[38], at[49]);    MULADD(at[39], at[48]); 
   COMBA_STORE(C->dp[39]);
   /* 40 */
   COMBA_FORWARD;
   MULADD(at[0], at[88]);    MULADD(at[1], at[87]);    MULADD(at[2], at[86]);    MULADD(at[3], at[85]);    MULADD(at[4], at[84]);    MULADD(at[5], at[83]);    MULADD(at[6], at[82]);    MULADD(at[7], at[81]);    MULADD(at[8], at[80]);    MULADD(at[9], at[79]);    MULADD(at[10], at[78]);    MULADD(at[11], at[77]);    MULADD(at[12], at[76]);    MULADD(at[13], at[75]);    MULADD(at[14], at[74]);    MULADD(at[15], at[73]);    MULADD(at[16], at[72]);    MULADD(at[17], at[71]);    MULADD(at[18], at[70]);    MULADD(at[19], at[69]);    MULADD(at[20], at[68]);    MULADD(at[21], at[67]);    MULADD(at[22], at[66]);    MULADD(at[23], at[65]);    MULADD(at[24], at[64]);    MULADD(at[25], at[63]);    MULADD(at[26], at[62]);    MULADD(at[27], at[61]);    MULADD(at[28], at[60]);    MULADD(at[29], at[59]);    MULADD(at[30], at[58]);    MULADD(at[31], at[57]);    MULADD(at[32], at[56]);    MULADD(at[33], at[55]);    MULADD(at[34], at[54]);    MULADD(at[35], at[53]);    MULADD(at[36], at[52]);    MULADD(at[37], at[51]);    MULADD(at[38], at[50]);    MULADD(at[39], at[49]);    MULADD(at[40], at[48]); 
   COMBA_STORE(C->dp[40]);
   /* 41 */
   COMBA_FORWARD;
   MULADD(at[0], at[89]);    MULADD(at[1], at[88]);    MULADD(at[2], at[87]);    MULADD(at[3], at[86]);    MULADD(at[4], at[85]);    MULADD(at[5], at[84]);    MULADD(at[6], at[83]);    MULADD(at[7], at[82]);    MULADD(at[8], at[81]);    MULADD(at[9], at[80]);    MULADD(at[10], at[79]);    MULADD(at[11], at[78]);    MULADD(at[12], at[77]);    MULADD(at[13], at[76]);    MULADD(at[14], at[75]);    MULADD(at[15], at[74]);    MULADD(at[16], at[73]);    MULADD(at[17], at[72]);    MULADD(at[18], at[71]);    MULADD(at[19], at[70]);    MULADD(at[20], at[69]);    MULADD(at[21], at[68]);    MULADD(at[22], at[67]);    MULADD(at[23], at[66]);    MULADD(at[24], at[65]);    MULADD(at[25], at[64]);    MULADD(at[26], at[63]);    MULADD(at[27], at[62]);    MULADD(at[28], at[61]);    MULADD(at[29], at[60]);    MULADD(at[30], at[59]);    MULADD(at[31], at[58]);    MULADD(at[32], at[57]);    MULADD(at[33], at[56]);    MULADD(at[34], at[55]);    MULADD(at[35], at[54]);    MULADD(at[36], at[53]);    MULADD(at[37], at[52]);    MULADD(at[38], at[51]);    MULADD(at[39], at[50]);    MULADD(at[40], at[49]);    MULADD(at[41], at[48]); 
   COMBA_STORE(C->dp[41]);
   /* 42 */
   COMBA_FORWARD;
   MULADD(at[0], at[90]);    MULADD(at[1], at[89]);    MULADD(at[2], at[88]);    MULADD(at[3], at[87]);    MULADD(at[4], at[86]);    MULADD(at[5], at[85]);    MULADD(at[6], at[84]);    MULADD(at[7], at[83]);    MULADD(at[8], at[82]);    MULADD(at[9], at[81]);    MULADD(at[10], at[80]);    MULADD(at[11], at[79]);    MULADD(at[12], at[78]);    MULADD(at[13], at[77]);    MULADD(at[14], at[76]);    MULADD(at[15], at[75]);    MULADD(at[16], at[74]);    MULADD(at[17], at[73]);    MULADD(at[18], at[72]);    MULADD(at[19], at[71]);    MULADD(at[20], at[70]);    MULADD(at[21], at[69]);    MULADD(at[22], at[68]);    MULADD(at[23], at[67]);    MULADD(at[24], at[66]);    MULADD(at[25], at[65]);    MULADD(at[26], at[64]);    MULADD(at[27], at[63]);    MULADD(at[28], at[62]);    MULADD(at[29], at[61]);    MULADD(at[30], at[60]);    MULADD(at[31], at[59]);    MULADD(at[32], at[58]);    MULADD(at[33], at[57]);    MULADD(at[34], at[56]);    MULADD(at[35], at[55]);    MULADD(at[36], at[54]);    MULADD(at[37], at[53]);    MULADD(at[38], at[52]);    MULADD(at[39], at[51]);    MULADD(at[40], at[50]);    MULADD(at[41], at[49]);    MULADD(at[42], at[48]); 
   COMBA_STORE(C->dp[42]);
   /* 43 */
   COMBA_FORWARD;
   MULADD(at[0], at[91]);    MULADD(at[1], at[90]);    MULADD(at[2], at[89]);    MULADD(at[3], at[88]);    MULADD(at[4], at[87]);    MULADD(at[5], at[86]);    MULADD(at[6], at[85]);    MULADD(at[7], at[84]);    MULADD(at[8], at[83]);    MULADD(at[9], at[82]);    MULADD(at[10], at[81]);    MULADD(at[11], at[80]);    MULADD(at[12], at[79]);    MULADD(at[13], at[78]);    MULADD(at[14], at[77]);    MULADD(at[15], at[76]);    MULADD(at[16], at[75]);    MULADD(at[17], at[74]);    MULADD(at[18], at[73]);    MULADD(at[19], at[72]);    MULADD(at[20], at[71]);    MULADD(at[21], at[70]);    MULADD(at[22], at[69]);    MULADD(at[23], at[68]);    MULADD(at[24], at[67]);    MULADD(at[25], at[66]);    MULADD(at[26], at[65]);    MULADD(at[27], at[64]);    MULADD(at[28], at[63]);    MULADD(at[29], at[62]);    MULADD(at[30], at[61]);    MULADD(at[31], at[60]);    MULADD(at[32], at[59]);    MULADD(at[33], at[58]);    MULADD(at[34], at[57]);    MULADD(at[35], at[56]);    MULADD(at[36], at[55]);    MULADD(at[37], at[54]);    MULADD(at[38], at[53]);    MULADD(at[39], at[52]);    MULADD(at[40], at[51]);    MULADD(at[41], at[50]);    MULADD(at[42], at[49]);    MULADD(at[43], at[48]); 
   COMBA_STORE(C->dp[43]);
   /* 44 */
   COMBA_FORWARD;
   MULADD(at[0], at[92]);    MULADD(at[1], at[91]);    MULADD(at[2], at[90]);    MULADD(at[3], at[89]);    MULADD(at[4], at[88]);    MULADD(at[5], at[87]);    MULADD(at[6], at[86]);    MULADD(at[7], at[85]);    MULADD(at[8], at[84]);    MULADD(at[9], at[83]);    MULADD(at[10], at[82]);    MULADD(at[11], at[81]);    MULADD(at[12], at[80]);    MULADD(at[13], at[79]);    MULADD(at[14], at[78]);    MULADD(at[15], at[77]);    MULADD(at[16], at[76]);    MULADD(at[17], at[75]);    MULADD(at[18], at[74]);    MULADD(at[19], at[73]);    MULADD(at[20], at[72]);    MULADD(at[21], at[71]);    MULADD(at[22], at[70]);    MULADD(at[23], at[69]);    MULADD(at[24], at[68]);    MULADD(at[25], at[67]);    MULADD(at[26], at[66]);    MULADD(at[27], at[65]);    MULADD(at[28], at[64]);    MULADD(at[29], at[63]);    MULADD(at[30], at[62]);    MULADD(at[31], at[61]);    MULADD(at[32], at[60]);    MULADD(at[33], at[59]);    MULADD(at[34], at[58]);    MULADD(at[35], at[57]);    MULADD(at[36], at[56]);    MULADD(at[37], at[55]);    MULADD(at[38], at[54]);    MULADD(at[39], at[53]);    MULADD(at[40], at[52]);    MULADD(at[41], at[51]);    MULADD(at[42], at[50]);    MULADD(at[43], at[49]);    MULADD(at[44], at[48]); 
   COMBA_STORE(C->dp[44]);
   /* 45 */
   COMBA_FORWARD;
   MULADD(at[0], at[93]);    MULADD(at[1], at[92]);    MULADD(at[2], at[91]);    MULADD(at[3], at[90]);    MULADD(at[4], at[89]);    MULADD(at[5], at[88]);    MULADD(at[6], at[87]);    MULADD(at[7], at[86]);    MULADD(at[8], at[85]);    MULADD(at[9], at[84]);    MULADD(at[10], at[83]);    MULADD(at[11], at[82]);    MULADD(at[12], at[81]);    MULADD(at[13], at[80]);    MULADD(at[14], at[79]);    MULADD(at[15], at[78]);    MULADD(at[16], at[77]);    MULADD(at[17], at[76]);    MULADD(at[18], at[75]);    MULADD(at[19], at[74]);    MULADD(at[20], at[73]);    MULADD(at[21], at[72]);    MULADD(at[22], at[71]);    MULADD(at[23], at[70]);    MULADD(at[24], at[69]);    MULADD(at[25], at[68]);    MULADD(at[26], at[67]);    MULADD(at[27], at[66]);    MULADD(at[28], at[65]);    MULADD(at[29], at[64]);    MULADD(at[30], at[63]);    MULADD(at[31], at[62]);    MULADD(at[32], at[61]);    MULADD(at[33], at[60]);    MULADD(at[34], at[59]);    MULADD(at[35], at[58]);    MULADD(at[36], at[57]);    MULADD(at[37], at[56]);    MULADD(at[38], at[55]);    MULADD(at[39], at[54]);    MULADD(at[40], at[53]);    MULADD(at[41], at[52]);    MULADD(at[42], at[51]);    MULADD(at[43], at[50]);    MULADD(at[44], at[49]);    MULADD(at[45], at[48]); 
   COMBA_STORE(C->dp[45]);
   /* 46 */
   COMBA_FORWARD;
   MULADD(at[0], at[94]);    MULADD(at[1], at[93]);    MULADD(at[2], at[92]);    MULADD(at[3], at[91]);    MULADD(at[4], at[90]);    MULADD(at[5], at[89]);    MULADD(at[6], at[88]);    MULADD(at[7], at[87]);    MULADD(at[8], at[86]);    MULADD(at[9], at[85]);    MULADD(at[10], at[84]);    MULADD(at[11], at[83]);    MULADD(at[12], at[82]);    MULADD(at[13], at[81]);    MULADD(at[14], at[80]);    MULADD(at[15], at[79]);    MULADD(at[16], at[78]);    MULADD(at[17], at[77]);    MULADD(at[18], at[76]);    MULADD(at[19], at[75]);    MULADD(at[20], at[74]);    MULADD(at[21], at[73]);    MULADD(at[22], at[72]);    MULADD(at[23], at[71]);    MULADD(at[24], at[70]);    MULADD(at[25], at[69]);    MULADD(at[26], at[68]);    MULADD(at[27], at[67]);    MULADD(at[28], at[66]);    MULADD(at[29], at[65]);    MULADD(at[30], at[64]);    MULADD(at[31], at[63]);    MULADD(at[32], at[62]);    MULADD(at[33], at[61]);    MULADD(at[34], at[60]);    MULADD(at[35], at[59]);    MULADD(at[36], at[58]);    MULADD(at[37], at[57]);    MULADD(at[38], at[56]);    MULADD(at[39], at[55]);    MULADD(at[40], at[54]);    MULADD(at[41], at[53]);    MULADD(at[42], at[52]);    MULADD(at[43], at[51]);    MULADD(at[44], at[50]);    MULADD(at[45], at[49]);    MULADD(at[46], at[48]); 
   COMBA_STORE(C->dp[46]);
   /* 47 */
   COMBA_FORWARD;
   MULADD(at[0], at[95]);    MULADD(at[1], at[94]);    MULADD(at[2], at[93]);    MULADD(at[3], at[92]);    MULADD(at[4], at[91]);    MULADD(at[5], at[90]);    MULADD(at[6], at[89]);    MULADD(at[7], at[88]);    MULADD(at[8], at[87]);    MULADD(at[9], at[86]);    MULADD(at[10], at[85]);    MULADD(at[11], at[84]);    MULADD(at[12], at[83]);    MULADD(at[13], at[82]);    MULADD(at[14], at[81]);    MULADD(at[15], at[80]);    MULADD(at[16], at[79]);    MULADD(at[17], at[78]);    MULADD(at[18], at[77]);    MULADD(at[19], at[76]);    MULADD(at[20], at[75]);    MULADD(at[21], at[74]);    MULADD(at[22], at[73]);    MULADD(at[23], at[72]);    MULADD(at[24], at[71]);    MULADD(at[25], at[70]);    MULADD(at[26], at[69]);    MULADD(at[27], at[68]);    MULADD(at[28], at[67]);    MULADD(at[29], at[66]);    MULADD(at[30], at[65]);    MULADD(at[31], at[64]);    MULADD(at[32], at[63]);    MULADD(at[33], at[62]);    MULADD(at[34], at[61]);    MULADD(at[35], at[60]);    MULADD(at[36], at[59]);    MULADD(at[37], at[58]);    MULADD(at[38], at[57]);    MULADD(at[39], at[56]);    MULADD(at[40], at[55]);    MULADD(at[41], at[54]);    MULADD(at[42], at[53]);    MULADD(at[43], at[52]);    MULADD(at[44], at[51]);    MULADD(at[45], at[50]);    MULADD(at[46], at[49]);    MULADD(at[47], at[48]); 
   COMBA_STORE(C->dp[47]);
   /* 48 */
   COMBA_FORWARD;
   MULADD(at[1], at[95]);    MULADD(at[2], at[94]);    MULADD(at[3], at[93]);    MULADD(at[4], at[92]);    MULADD(at[5], at[91]);    MULADD(at[6], at[90]);    MULADD(at[7], at[89]);    MULADD(at[8], at[88]);    MULADD(at[9], at[87]);    MULADD(at[10], at[86]);    MULADD(at[11], at[85]);    MULADD(at[12], at[84]);    MULADD(at[13], at[83]);    MULADD(at[14], at[82]);    MULADD(at[15], at[81]);    MULADD(at[16], at[80]);    MULADD(at[17], at[79]);    MULADD(at[18], at[78]);    MULADD(at[19], at[77]);    MULADD(at[20], at[76]);    MULADD(at[21], at[75]);    MULADD(at[22], at[74]);    MULADD(at[23], at[73]);    MULADD(at[24], at[72]);    MULADD(at[25], at[71]);    MULADD(at[26], at[70]);    MULADD(at[27], at[69]);    MULADD(at[28], at[68]);    MULADD(at[29], at[67]);    MULADD(at[30], at[66]);    MULADD(at[31], at[65]);    MULADD(at[32], at[64]);    MULADD(at[33], at[63]);    MULADD(at[34], at[62]);    MULADD(at[35], at[61]);    MULADD(at[36], at[60]);    MULADD(at[37], at[59]);    MULADD(at[38], at[58]);    MULADD(at[39], at[57]);    MULADD(at[40], at[56]);    MULADD(at[41], at[55]);    MULADD(at[42], at[54]);    MULADD(at[43], at[53]);    MULADD(at[44], at[52]);    MULADD(at[45], at[51]);    MULADD(at[46], at[50]);    MULADD(at[47], at[49]); 
   COMBA_STORE(C->dp[48]);
   /* 49 */
   COMBA_FORWARD;
   MULADD(at[2], at[95]);    MULADD(at[3], at[94]);    MULADD(at[4], at[93]);    MULADD(at[5], at[92]);    MULADD(at[6], at[91]);    MULADD(at[7], at[90]);    MULADD(at[8], at[89]);    MULADD(at[9], at[88]);    MULADD(at[10], at[87]);    MULADD(at[11], at[86]);    MULADD(at[12], at[85]);    MULADD(at[13], at[84]);    MULADD(at[14], at[83]);    MULADD(at[15], at[82]);    MULADD(at[16], at[81]);    MULADD(at[17], at[80]);    MULADD(at[18], at[79]);    MULADD(at[19], at[78]);    MULADD(at[20], at[77]);    MULADD(at[21], at[76]);    MULADD(at[22], at[75]);    MULADD(at[23], at[74]);    MULADD(at[24], at[73]);    MULADD(at[25], at[72]);    MULADD(at[26], at[71]);    MULADD(at[27], at[70]);    MULADD(at[28], at[69]);    MULADD(at[29], at[68]);    MULADD(at[30], at[67]);    MULADD(at[31], at[66]);    MULADD(at[32], at[65]);    MULADD(at[33], at[64]);    MULADD(at[34], at[63]);    MULADD(at[35], at[62]);    MULADD(at[36], at[61]);    MULADD(at[37], at[60]);    MULADD(at[38], at[59]);    MULADD(at[39], at[58]);    MULADD(at[40], at[57]);    MULADD(at[41], at[56]);    MULADD(at[42], at[55]);    MULADD(at[43], at[54]);    MULADD(at[44], at[53]);    MULADD(at[45], at[52]);    MULADD(at[46], at[51]);    MULADD(at[47], at[50]); 
   COMBA_STORE(C->dp[49]);
   /* 50 */
   COMBA_FORWARD;
   MULADD(at[3], at[95]);    MULADD(at[4], at[94]);    MULADD(at[5], at[93]);    MULADD(at[6], at[92]);    MULADD(at[7], at[91]);    MULADD(at[8], at[90]);    MULADD(at[9], at[89]);    MULADD(at[10], at[88]);    MULADD(at[11], at[87]);    MULADD(at[12], at[86]);    MULADD(at[13], at[85]);    MULADD(at[14], at[84]);    MULADD(at[15], at[83]);    MULADD(at[16], at[82]);    MULADD(at[17], at[81]);    MULADD(at[18], at[80]);    MULADD(at[19], at[79]);    MULADD(at[20], at[78]);    MULADD(at[21], at[77]);    MULADD(at[22], at[76]);    MULADD(at[23], at[75]);    MULADD(at[24], at[74]);    MULADD(at[25], at[73]);    MULADD(at[26], at[72]);    MULADD(at[27], at[71]);    MULADD(at[28], at[70]);    MULADD(at[29], at[69]);    MULADD(at[30], at[68]);    MULADD(at[31], at[67]);    MULADD(at[32], at[66]);    MULADD(at[33], at[65]);    MULADD(at[34], at[64]);    MULADD(at[35], at[63]);    MULADD(at[36], at[62]);    MULADD(at[37], at[61]);    MULADD(at[38], at[60]);    MULADD(at[39], at[59]);    MULADD(at[40], at[58]);    MULADD(at[41], at[57]);    MULADD(at[42], at[56]);    MULADD(at[43], at[55]);    MULADD(at[44], at[54]);    MULADD(at[45], at[53]);    MULADD(at[46], at[52]);    MULADD(at[47], at[51]); 
   COMBA_STORE(C->dp[50]);
   /* 51 */
   COMBA_FORWARD;
   MULADD(at[4], at[95]);    MULADD(at[5], at[94]);    MULADD(at[6], at[93]);    MULADD(at[7], at[92]);    MULADD(at[8], at[91]);    MULADD(at[9], at[90]);    MULADD(at[10], at[89]);    MULADD(at[11], at[88]);    MULADD(at[12], at[87]);    MULADD(at[13], at[86]);    MULADD(at[14], at[85]);    MULADD(at[15], at[84]);    MULADD(at[16], at[83]);    MULADD(at[17], at[82]);    MULADD(at[18], at[81]);    MULADD(at[19], at[80]);    MULADD(at[20], at[79]);    MULADD(at[21], at[78]);    MULADD(at[22], at[77]);    MULADD(at[23], at[76]);    MULADD(at[24], at[75]);    MULADD(at[25], at[74]);    MULADD(at[26], at[73]);    MULADD(at[27], at[72]);    MULADD(at[28], at[71]);    MULADD(at[29], at[70]);    MULADD(at[30], at[69]);    MULADD(at[31], at[68]);    MULADD(at[32], at[67]);    MULADD(at[33], at[66]);    MULADD(at[34], at[65]);    MULADD(at[35], at[64]);    MULADD(at[36], at[63]);    MULADD(at[37], at[62]);    MULADD(at[38], at[61]);    MULADD(at[39], at[60]);    MULADD(at[40], at[59]);    MULADD(at[41], at[58]);    MULADD(at[42], at[57]);    MULADD(at[43], at[56]);    MULADD(at[44], at[55]);    MULADD(at[45], at[54]);    MULADD(at[46], at[53]);    MULADD(at[47], at[52]); 
   COMBA_STORE(C->dp[51]);
   /* 52 */
   COMBA_FORWARD;
   MULADD(at[5], at[95]);    MULADD(at[6], at[94]);    MULADD(at[7], at[93]);    MULADD(at[8], at[92]);    MULADD(at[9], at[91]);    MULADD(at[10], at[90]);    MULADD(at[11], at[89]);    MULADD(at[12], at[88]);    MULADD(at[13], at[87]);    MULADD(at[14], at[86]);    MULADD(at[15], at[85]);    MULADD(at[16], at[84]);    MULADD(at[17], at[83]);    MULADD(at[18], at[82]);    MULADD(at[19], at[81]);    MULADD(at[20], at[80]);    MULADD(at[21], at[79]);    MULADD(at[22], at[78]);    MULADD(at[23], at[77]);    MULADD(at[24], at[76]);    MULADD(at[25], at[75]);    MULADD(at[26], at[74]);    MULADD(at[27], at[73]);    MULADD(at[28], at[72]);    MULADD(at[29], at[71]);    MULADD(at[30], at[70]);    MULADD(at[31], at[69]);    MULADD(at[32], at[68]);    MULADD(at[33], at[67]);    MULADD(at[34], at[66]);    MULADD(at[35], at[65]);    MULADD(at[36], at[64]);    MULADD(at[37], at[63]);    MULADD(at[38], at[62]);    MULADD(at[39], at[61]);    MULADD(at[40], at[60]);    MULADD(at[41], at[59]);    MULADD(at[42], at[58]);    MULADD(at[43], at[57]);    MULADD(at[44], at[56]);    MULADD(at[45], at[55]);    MULADD(at[46], at[54]);    MULADD(at[47], at[53]); 
   COMBA_STORE(C->dp[52]);
   /* 53 */
   COMBA_FORWARD;
   MULADD(at[6], at[95]);    MULADD(at[7], at[94]);    MULADD(at[8], at[93]);    MULADD(at[9], at[92]);    MULADD(at[10], at[91]);    MULADD(at[11], at[90]);    MULADD(at[12], at[89]);    MULADD(at[13], at[88]);    MULADD(at[14], at[87]);    MULADD(at[15], at[86]);    MULADD(at[16], at[85]);    MULADD(at[17], at[84]);    MULADD(at[18], at[83]);    MULADD(at[19], at[82]);    MULADD(at[20], at[81]);    MULADD(at[21], at[80]);    MULADD(at[22], at[79]);    MULADD(at[23], at[78]);    MULADD(at[24], at[77]);    MULADD(at[25], at[76]);    MULADD(at[26], at[75]);    MULADD(at[27], at[74]);    MULADD(at[28], at[73]);    MULADD(at[29], at[72]);    MULADD(at[30], at[71]);    MULADD(at[31], at[70]);    MULADD(at[32], at[69]);    MULADD(at[33], at[68]);    MULADD(at[34], at[67]);    MULADD(at[35], at[66]);    MULADD(at[36], at[65]);    MULADD(at[37], at[64]);    MULADD(at[38], at[63]);    MULADD(at[39], at[62]);    MULADD(at[40], at[61]);    MULADD(at[41], at[60]);    MULADD(at[42], at[59]);    MULADD(at[43], at[58]);    MULADD(at[44], at[57]);    MULADD(at[45], at[56]);    MULADD(at[46], at[55]);    MULADD(at[47], at[54]); 
   COMBA_STORE(C->dp[53]);
   /* 54 */
   COMBA_FORWARD;
   MULADD(at[7], at[95]);    MULADD(at[8], at[94]);    MULADD(at[9], at[93]);    MULADD(at[10], at[92]);    MULADD(at[11], at[91]);    MULADD(at[12], at[90]);    MULADD(at[13], at[89]);    MULADD(at[14], at[88]);    MULADD(at[15], at[87]);    MULADD(at[16], at[86]);    MULADD(at[17], at[85]);    MULADD(at[18], at[84]);    MULADD(at[19], at[83]);    MULADD(at[20], at[82]);    MULADD(at[21], at[81]);    MULADD(at[22], at[80]);    MULADD(at[23], at[79]);    MULADD(at[24], at[78]);    MULADD(at[25], at[77]);    MULADD(at[26], at[76]);    MULADD(at[27], at[75]);    MULADD(at[28], at[74]);    MULADD(at[29], at[73]);    MULADD(at[30], at[72]);    MULADD(at[31], at[71]);    MULADD(at[32], at[70]);    MULADD(at[33], at[69]);    MULADD(at[34], at[68]);    MULADD(at[35], at[67]);    MULADD(at[36], at[66]);    MULADD(at[37], at[65]);    MULADD(at[38], at[64]);    MULADD(at[39], at[63]);    MULADD(at[40], at[62]);    MULADD(at[41], at[61]);    MULADD(at[42], at[60]);    MULADD(at[43], at[59]);    MULADD(at[44], at[58]);    MULADD(at[45], at[57]);    MULADD(at[46], at[56]);    MULADD(at[47], at[55]); 
   COMBA_STORE(C->dp[54]);
   /* 55 */
   COMBA_FORWARD;
   MULADD(at[8], at[95]);    MULADD(at[9], at[94]);    MULADD(at[10], at[93]);    MULADD(at[11], at[92]);    MULADD(at[12], at[91]);    MULADD(at[13], at[90]);    MULADD(at[14], at[89]);    MULADD(at[15], at[88]);    MULADD(at[16], at[87]);    MULADD(at[17], at[86]);    MULADD(at[18], at[85]);    MULADD(at[19], at[84]);    MULADD(at[20], at[83]);    MULADD(at[21], at[82]);    MULADD(at[22], at[81]);    MULADD(at[23], at[80]);    MULADD(at[24], at[79]);    MULADD(at[25], at[78]);    MULADD(at[26], at[77]);    MULADD(at[27], at[76]);    MULADD(at[28], at[75]);    MULADD(at[29], at[74]);    MULADD(at[30], at[73]);    MULADD(at[31], at[72]);    MULADD(at[32], at[71]);    MULADD(at[33], at[70]);    MULADD(at[34], at[69]);    MULADD(at[35], at[68]);    MULADD(at[36], at[67]);    MULADD(at[37], at[66]);    MULADD(at[38], at[65]);    MULADD(at[39], at[64]);    MULADD(at[40], at[63]);    MULADD(at[41], at[62]);    MULADD(at[42], at[61]);    MULADD(at[43], at[60]);    MULADD(at[44], at[59]);    MULADD(at[45], at[58]);    MULADD(at[46], at[57]);    MULADD(at[47], at[56]); 
   COMBA_STORE(C->dp[55]);
   /* 56 */
   COMBA_FORWARD;
   MULADD(at[9], at[95]);    MULADD(at[10], at[94]);    MULADD(at[11], at[93]);    MULADD(at[12], at[92]);    MULADD(at[13], at[91]);    MULADD(at[14], at[90]);    MULADD(at[15], at[89]);    MULADD(at[16], at[88]);    MULADD(at[17], at[87]);    MULADD(at[18], at[86]);    MULADD(at[19], at[85]);    MULADD(at[20], at[84]);    MULADD(at[21], at[83]);    MULADD(at[22], at[82]);    MULADD(at[23], at[81]);    MULADD(at[24], at[80]);    MULADD(at[25], at[79]);    MULADD(at[26], at[78]);    MULADD(at[27], at[77]);    MULADD(at[28], at[76]);    MULADD(at[29], at[75]);    MULADD(at[30], at[74]);    MULADD(at[31], at[73]);    MULADD(at[32], at[72]);    MULADD(at[33], at[71]);    MULADD(at[34], at[70]);    MULADD(at[35], at[69]);    MULADD(at[36], at[68]);    MULADD(at[37], at[67]);    MULADD(at[38], at[66]);    MULADD(at[39], at[65]);    MULADD(at[40], at[64]);    MULADD(at[41], at[63]);    MULADD(at[42], at[62]);    MULADD(at[43], at[61]);    MULADD(at[44], at[60]);    MULADD(at[45], at[59]);    MULADD(at[46], at[58]);    MULADD(at[47], at[57]); 
   COMBA_STORE(C->dp[56]);
   /* 57 */
   COMBA_FORWARD;
   MULADD(at[10], at[95]);    MULADD(at[11], at[94]);    MULADD(at[12], at[93]);    MULADD(at[13], at[92]);    MULADD(at[14], at[91]);    MULADD(at[15], at[90]);    MULADD(at[16], at[89]);    MULADD(at[17], at[88]);    MULADD(at[18], at[87]);    MULADD(at[19], at[86]);    MULADD(at[20], at[85]);    MULADD(at[21], at[84]);    MULADD(at[22], at[83]);    MULADD(at[23], at[82]);    MULADD(at[24], at[81]);    MULADD(at[25], at[80]);    MULADD(at[26], at[79]);    MULADD(at[27], at[78]);    MULADD(at[28], at[77]);    MULADD(at[29], at[76]);    MULADD(at[30], at[75]);    MULADD(at[31], at[74]);    MULADD(at[32], at[73]);    MULADD(at[33], at[72]);    MULADD(at[34], at[71]);    MULADD(at[35], at[70]);    MULADD(at[36], at[69]);    MULADD(at[37], at[68]);    MULADD(at[38], at[67]);    MULADD(at[39], at[66]);    MULADD(at[40], at[65]);    MULADD(at[41], at[64]);    MULADD(at[42], at[63]);    MULADD(at[43], at[62]);    MULADD(at[44], at[61]);    MULADD(at[45], at[60]);    MULADD(at[46], at[59]);    MULADD(at[47], at[58]); 
   COMBA_STORE(C->dp[57]);
   /* 58 */
   COMBA_FORWARD;
   MULADD(at[11], at[95]);    MULADD(at[12], at[94]);    MULADD(at[13], at[93]);    MULADD(at[14], at[92]);    MULADD(at[15], at[91]);    MULADD(at[16], at[90]);    MULADD(at[17], at[89]);    MULADD(at[18], at[88]);    MULADD(at[19], at[87]);    MULADD(at[20], at[86]);    MULADD(at[21], at[85]);    MULADD(at[22], at[84]);    MULADD(at[23], at[83]);    MULADD(at[24], at[82]);    MULADD(at[25], at[81]);    MULADD(at[26], at[80]);    MULADD(at[27], at[79]);    MULADD(at[28], at[78]);    MULADD(at[29], at[77]);    MULADD(at[30], at[76]);    MULADD(at[31], at[75]);    MULADD(at[32], at[74]);    MULADD(at[33], at[73]);    MULADD(at[34], at[72]);    MULADD(at[35], at[71]);    MULADD(at[36], at[70]);    MULADD(at[37], at[69]);    MULADD(at[38], at[68]);    MULADD(at[39], at[67]);    MULADD(at[40], at[66]);    MULADD(at[41], at[65]);    MULADD(at[42], at[64]);    MULADD(at[43], at[63]);    MULADD(at[44], at[62]);    MULADD(at[45], at[61]);    MULADD(at[46], at[60]);    MULADD(at[47], at[59]); 
   COMBA_STORE(C->dp[58]);
   /* 59 */
   COMBA_FORWARD;
   MULADD(at[12], at[95]);    MULADD(at[13], at[94]);    MULADD(at[14], at[93]);    MULADD(at[15], at[92]);    MULADD(at[16], at[91]);    MULADD(at[17], at[90]);    MULADD(at[18], at[89]);    MULADD(at[19], at[88]);    MULADD(at[20], at[87]);    MULADD(at[21], at[86]);    MULADD(at[22], at[85]);    MULADD(at[23], at[84]);    MULADD(at[24], at[83]);    MULADD(at[25], at[82]);    MULADD(at[26], at[81]);    MULADD(at[27], at[80]);    MULADD(at[28], at[79]);    MULADD(at[29], at[78]);    MULADD(at[30], at[77]);    MULADD(at[31], at[76]);    MULADD(at[32], at[75]);    MULADD(at[33], at[74]);    MULADD(at[34], at[73]);    MULADD(at[35], at[72]);    MULADD(at[36], at[71]);    MULADD(at[37], at[70]);    MULADD(at[38], at[69]);    MULADD(at[39], at[68]);    MULADD(at[40], at[67]);    MULADD(at[41], at[66]);    MULADD(at[42], at[65]);    MULADD(at[43], at[64]);    MULADD(at[44], at[63]);    MULADD(at[45], at[62]);    MULADD(at[46], at[61]);    MULADD(at[47], at[60]); 
   COMBA_STORE(C->dp[59]);
   /* 60 */
   COMBA_FORWARD;
   MULADD(at[13], at[95]);    MULADD(at[14], at[94]);    MULADD(at[15], at[93]);    MULADD(at[16], at[92]);    MULADD(at[17], at[91]);    MULADD(at[18], at[90]);    MULADD(at[19], at[89]);    MULADD(at[20], at[88]);    MULADD(at[21], at[87]);    MULADD(at[22], at[86]);    MULADD(at[23], at[85]);    MULADD(at[24], at[84]);    MULADD(at[25], at[83]);    MULADD(at[26], at[82]);    MULADD(at[27], at[81]);    MULADD(at[28], at[80]);    MULADD(at[29], at[79]);    MULADD(at[30], at[78]);    MULADD(at[31], at[77]);    MULADD(at[32], at[76]);    MULADD(at[33], at[75]);    MULADD(at[34], at[74]);    MULADD(at[35], at[73]);    MULADD(at[36], at[72]);    MULADD(at[37], at[71]);    MULADD(at[38], at[70]);    MULADD(at[39], at[69]);    MULADD(at[40], at[68]);    MULADD(at[41], at[67]);    MULADD(at[42], at[66]);    MULADD(at[43], at[65]);    MULADD(at[44], at[64]);    MULADD(at[45], at[63]);    MULADD(at[46], at[62]);    MULADD(at[47], at[61]); 
   COMBA_STORE(C->dp[60]);
   /* 61 */
   COMBA_FORWARD;
   MULADD(at[14], at[95]);    MULADD(at[15], at[94]);    MULADD(at[16], at[93]);    MULADD(at[17], at[92]);    MULADD(at[18], at[91]);    MULADD(at[19], at[90]);    MULADD(at[20], at[89]);    MULADD(at[21], at[88]);    MULADD(at[22], at[87]);    MULADD(at[23], at[86]);    MULADD(at[24], at[85]);    MULADD(at[25], at[84]);    MULADD(at[26], at[83]);    MULADD(at[27], at[82]);    MULADD(at[28], at[81]);    MULADD(at[29], at[80]);    MULADD(at[30], at[79]);    MULADD(at[31], at[78]);    MULADD(at[32], at[77]);    MULADD(at[33], at[76]);    MULADD(at[34], at[75]);    MULADD(at[35], at[74]);    MULADD(at[36], at[73]);    MULADD(at[37], at[72]);    MULADD(at[38], at[71]);    MULADD(at[39], at[70]);    MULADD(at[40], at[69]);    MULADD(at[41], at[68]);    MULADD(at[42], at[67]);    MULADD(at[43], at[66]);    MULADD(at[44], at[65]);    MULADD(at[45], at[64]);    MULADD(at[46], at[63]);    MULADD(at[47], at[62]); 
   COMBA_STORE(C->dp[61]);
   /* 62 */
   COMBA_FORWARD;
   MULADD(at[15], at[95]);    MULADD(at[16], at[94]);    MULADD(at[17], at[93]);    MULADD(at[18], at[92]);    MULADD(at[19], at[91]);    MULADD(at[20], at[90]);    MULADD(at[21], at[89]);    MULADD(at[22], at[88]);    MULADD(at[23], at[87]);    MULADD(at[24], at[86]);    MULADD(at[25], at[85]);    MULADD(at[26], at[84]);    MULADD(at[27], at[83]);    MULADD(at[28], at[82]);    MULADD(at[29], at[81]);    MULADD(at[30], at[80]);    MULADD(at[31], at[79]);    MULADD(at[32], at[78]);    MULADD(at[33], at[77]);    MULADD(at[34], at[76]);    MULADD(at[35], at[75]);    MULADD(at[36], at[74]);    MULADD(at[37], at[73]);    MULADD(at[38], at[72]);    MULADD(at[39], at[71]);    MULADD(at[40], at[70]);    MULADD(at[41], at[69]);    MULADD(at[42], at[68]);    MULADD(at[43], at[67]);    MULADD(at[44], at[66]);    MULADD(at[45], at[65]);    MULADD(at[46], at[64]);    MULADD(at[47], at[63]); 
   COMBA_STORE(C->dp[62]);
   /* 63 */
   COMBA_FORWARD;
   MULADD(at[16], at[95]);    MULADD(at[17], at[94]);    MULADD(at[18], at[93]);    MULADD(at[19], at[92]);    MULADD(at[20], at[91]);    MULADD(at[21], at[90]);    MULADD(at[22], at[89]);    MULADD(at[23], at[88]);    MULADD(at[24], at[87]);    MULADD(at[25], at[86]);    MULADD(at[26], at[85]);    MULADD(at[27], at[84]);    MULADD(at[28], at[83]);    MULADD(at[29], at[82]);    MULADD(at[30], at[81]);    MULADD(at[31], at[80]);    MULADD(at[32], at[79]);    MULADD(at[33], at[78]);    MULADD(at[34], at[77]);    MULADD(at[35], at[76]);    MULADD(at[36], at[75]);    MULADD(at[37], at[74]);    MULADD(at[38], at[73]);    MULADD(at[39], at[72]);    MULADD(at[40], at[71]);    MULADD(at[41], at[70]);    MULADD(at[42], at[69]);    MULADD(at[43], at[68]);    MULADD(at[44], at[67]);    MULADD(at[45], at[66]);    MULADD(at[46], at[65]);    MULADD(at[47], at[64]); 
   COMBA_STORE(C->dp[63]);
   /* 64 */
   COMBA_FORWARD;
   MULADD(at[17], at[95]);    MULADD(at[18], at[94]);    MULADD(at[19], at[93]);    MULADD(at[20], at[92]);    MULADD(at[21], at[91]);    MULADD(at[22], at[90]);    MULADD(at[23], at[89]);    MULADD(at[24], at[88]);    MULADD(at[25], at[87]);    MULADD(at[26], at[86]);    MULADD(at[27], at[85]);    MULADD(at[28], at[84]);    MULADD(at[29], at[83]);    MULADD(at[30], at[82]);    MULADD(at[31], at[81]);    MULADD(at[32], at[80]);    MULADD(at[33], at[79]);    MULADD(at[34], at[78]);    MULADD(at[35], at[77]);    MULADD(at[36], at[76]);    MULADD(at[37], at[75]);    MULADD(at[38], at[74]);    MULADD(at[39], at[73]);    MULADD(at[40], at[72]);    MULADD(at[41], at[71]);    MULADD(at[42], at[70]);    MULADD(at[43], at[69]);    MULADD(at[44], at[68]);    MULADD(at[45], at[67]);    MULADD(at[46], at[66]);    MULADD(at[47], at[65]); 
   COMBA_STORE(C->dp[64]);
   /* 65 */
   COMBA_FORWARD;
   MULADD(at[18], at[95]);    MULADD(at[19], at[94]);    MULADD(at[20], at[93]);    MULADD(at[21], at[92]);    MULADD(at[22], at[91]);    MULADD(at[23], at[90]);    MULADD(at[24], at[89]);    MULADD(at[25], at[88]);    MULADD(at[26], at[87]);    MULADD(at[27], at[86]);    MULADD(at[28], at[85]);    MULADD(at[29], at[84]);    MULADD(at[30], at[83]);    MULADD(at[31], at[82]);    MULADD(at[32], at[81]);    MULADD(at[33], at[80]);    MULADD(at[34], at[79]);    MULADD(at[35], at[78]);    MULADD(at[36], at[77]);    MULADD(at[37], at[76]);    MULADD(at[38], at[75]);    MULADD(at[39], at[74]);    MULADD(at[40], at[73]);    MULADD(at[41], at[72]);    MULADD(at[42], at[71]);    MULADD(at[43], at[70]);    MULADD(at[44], at[69]);    MULADD(at[45], at[68]);    MULADD(at[46], at[67]);    MULADD(at[47], at[66]); 
   COMBA_STORE(C->dp[65]);
   /* 66 */
   COMBA_FORWARD;
   MULADD(at[19], at[95]);    MULADD(at[20], at[94]);    MULADD(at[21], at[93]);    MULADD(at[22], at[92]);    MULADD(at[23], at[91]);    MULADD(at[24], at[90]);    MULADD(at[25], at[89]);    MULADD(at[26], at[88]);    MULADD(at[27], at[87]);    MULADD(at[28], at[86]);    MULADD(at[29], at[85]);    MULADD(at[30], at[84]);    MULADD(at[31], at[83]);    MULADD(at[32], at[82]);    MULADD(at[33], at[81]);    MULADD(at[34], at[80]);    MULADD(at[35], at[79]);    MULADD(at[36], at[78]);    MULADD(at[37], at[77]);    MULADD(at[38], at[76]);    MULADD(at[39], at[75]);    MULADD(at[40], at[74]);    MULADD(at[41], at[73]);    MULADD(at[42], at[72]);    MULADD(at[43], at[71]);    MULADD(at[44], at[70]);    MULADD(at[45], at[69]);    MULADD(at[46], at[68]);    MULADD(at[47], at[67]); 
   COMBA_STORE(C->dp[66]);
   /* 67 */
   COMBA_FORWARD;
   MULADD(at[20], at[95]);    MULADD(at[21], at[94]);    MULADD(at[22], at[93]);    MULADD(at[23], at[92]);    MULADD(at[24], at[91]);    MULADD(at[25], at[90]);    MULADD(at[26], at[89]);    MULADD(at[27], at[88]);    MULADD(at[28], at[87]);    MULADD(at[29], at[86]);    MULADD(at[30], at[85]);    MULADD(at[31], at[84]);    MULADD(at[32], at[83]);    MULADD(at[33], at[82]);    MULADD(at[34], at[81]);    MULADD(at[35], at[80]);    MULADD(at[36], at[79]);    MULADD(at[37], at[78]);    MULADD(at[38], at[77]);    MULADD(at[39], at[76]);    MULADD(at[40], at[75]);    MULADD(at[41], at[74]);    MULADD(at[42], at[73]);    MULADD(at[43], at[72]);    MULADD(at[44], at[71]);    MULADD(at[45], at[70]);    MULADD(at[46], at[69]);    MULADD(at[47], at[68]); 
   COMBA_STORE(C->dp[67]);
   /* 68 */
   COMBA_FORWARD;
   MULADD(at[21], at[95]);    MULADD(at[22], at[94]);    MULADD(at[23], at[93]);    MULADD(at[24], at[92]);    MULADD(at[25], at[91]);    MULADD(at[26], at[90]);    MULADD(at[27], at[89]);    MULADD(at[28], at[88]);    MULADD(at[29], at[87]);    MULADD(at[30], at[86]);    MULADD(at[31], at[85]);    MULADD(at[32], at[84]);    MULADD(at[33], at[83]);    MULADD(at[34], at[82]);    MULADD(at[35], at[81]);    MULADD(at[36], at[80]);    MULADD(at[37], at[79]);    MULADD(at[38], at[78]);    MULADD(at[39], at[77]);    MULADD(at[40], at[76]);    MULADD(at[41], at[75]);    MULADD(at[42], at[74]);    MULADD(at[43], at[73]);    MULADD(at[44], at[72]);    MULADD(at[45], at[71]);    MULADD(at[46], at[70]);    MULADD(at[47], at[69]); 
   COMBA_STORE(C->dp[68]);
   /* 69 */
   COMBA_FORWARD;
   MULADD(at[22], at[95]);    MULADD(at[23], at[94]);    MULADD(at[24], at[93]);    MULADD(at[25], at[92]);    MULADD(at[26], at[91]);    MULADD(at[27], at[90]);    MULADD(at[28], at[89]);    MULADD(at[29], at[88]);    MULADD(at[30], at[87]);    MULADD(at[31], at[86]);    MULADD(at[32], at[85]);    MULADD(at[33], at[84]);    MULADD(at[34], at[83]);    MULADD(at[35], at[82]);    MULADD(at[36], at[81]);    MULADD(at[37], at[80]);    MULADD(at[38], at[79]);    MULADD(at[39], at[78]);    MULADD(at[40], at[77]);    MULADD(at[41], at[76]);    MULADD(at[42], at[75]);    MULADD(at[43], at[74]);    MULADD(at[44], at[73]);    MULADD(at[45], at[72]);    MULADD(at[46], at[71]);    MULADD(at[47], at[70]); 
   COMBA_STORE(C->dp[69]);
   /* 70 */
   COMBA_FORWARD;
   MULADD(at[23], at[95]);    MULADD(at[24], at[94]);    MULADD(at[25], at[93]);    MULADD(at[26], at[92]);    MULADD(at[27], at[91]);    MULADD(at[28], at[90]);    MULADD(at[29], at[89]);    MULADD(at[30], at[88]);    MULADD(at[31], at[87]);    MULADD(at[32], at[86]);    MULADD(at[33], at[85]);    MULADD(at[34], at[84]);    MULADD(at[35], at[83]);    MULADD(at[36], at[82]);    MULADD(at[37], at[81]);    MULADD(at[38], at[80]);    MULADD(at[39], at[79]);    MULADD(at[40], at[78]);    MULADD(at[41], at[77]);    MULADD(at[42], at[76]);    MULADD(at[43], at[75]);    MULADD(at[44], at[74]);    MULADD(at[45], at[73]);    MULADD(at[46], at[72]);    MULADD(at[47], at[71]); 
   COMBA_STORE(C->dp[70]);
   /* 71 */
   COMBA_FORWARD;
   MULADD(at[24], at[95]);    MULADD(at[25], at[94]);    MULADD(at[26], at[93]);    MULADD(at[27], at[92]);    MULADD(at[28], at[91]);    MULADD(at[29], at[90]);    MULADD(at[30], at[89]);    MULADD(at[31], at[88]);    MULADD(at[32], at[87]);    MULADD(at[33], at[86]);    MULADD(at[34], at[85]);    MULADD(at[35], at[84]);    MULADD(at[36], at[83]);    MULADD(at[37], at[82]);    MULADD(at[38], at[81]);    MULADD(at[39], at[80]);    MULADD(at[40], at[79]);    MULADD(at[41], at[78]);    MULADD(at[42], at[77]);    MULADD(at[43], at[76]);    MULADD(at[44], at[75]);    MULADD(at[45], at[74]);    MULADD(at[46], at[73]);    MULADD(at[47], at[72]); 
   COMBA_STORE(C->dp[71]);
   /* 72 */
   COMBA_FORWARD;
   MULADD(at[25], at[95]);    MULADD(at[26], at[94]);    MULADD(at[27], at[93]);    MULADD(at[28], at[92]);    MULADD(at[29], at[91]);    MULADD(at[30], at[90]);    MULADD(at[31], at[89]);    MULADD(at[32], at[88]);    MULADD(at[33], at[87]);    MULADD(at[34], at[86]);    MULADD(at[35], at[85]);    MULADD(at[36], at[84]);    MULADD(at[37], at[83]);    MULADD(at[38], at[82]);    MULADD(at[39], at[81]);    MULADD(at[40], at[80]);    MULADD(at[41], at[79]);    MULADD(at[42], at[78]);    MULADD(at[43], at[77]);    MULADD(at[44], at[76]);    MULADD(at[45], at[75]);    MULADD(at[46], at[74]);    MULADD(at[47], at[73]); 
   COMBA_STORE(C->dp[72]);
   /* 73 */
   COMBA_FORWARD;
   MULADD(at[26], at[95]);    MULADD(at[27], at[94]);    MULADD(at[28], at[93]);    MULADD(at[29], at[92]);    MULADD(at[30], at[91]);    MULADD(at[31], at[90]);    MULADD(at[32], at[89]);    MULADD(at[33], at[88]);    MULADD(at[34], at[87]);    MULADD(at[35], at[86]);    MULADD(at[36], at[85]);    MULADD(at[37], at[84]);    MULADD(at[38], at[83]);    MULADD(at[39], at[82]);    MULADD(at[40], at[81]);    MULADD(at[41], at[80]);    MULADD(at[42], at[79]);    MULADD(at[43], at[78]);    MULADD(at[44], at[77]);    MULADD(at[45], at[76]);    MULADD(at[46], at[75]);    MULADD(at[47], at[74]); 
   COMBA_STORE(C->dp[73]);
   /* 74 */
   COMBA_FORWARD;
   MULADD(at[27], at[95]);    MULADD(at[28], at[94]);    MULADD(at[29], at[93]);    MULADD(at[30], at[92]);    MULADD(at[31], at[91]);    MULADD(at[32], at[90]);    MULADD(at[33], at[89]);    MULADD(at[34], at[88]);    MULADD(at[35], at[87]);    MULADD(at[36], at[86]);    MULADD(at[37], at[85]);    MULADD(at[38], at[84]);    MULADD(at[39], at[83]);    MULADD(at[40], at[82]);    MULADD(at[41], at[81]);    MULADD(at[42], at[80]);    MULADD(at[43], at[79]);    MULADD(at[44], at[78]);    MULADD(at[45], at[77]);    MULADD(at[46], at[76]);    MULADD(at[47], at[75]); 
   COMBA_STORE(C->dp[74]);
   /* 75 */
   COMBA_FORWARD;
   MULADD(at[28], at[95]);    MULADD(at[29], at[94]);    MULADD(at[30], at[93]);    MULADD(at[31], at[92]);    MULADD(at[32], at[91]);    MULADD(at[33], at[90]);    MULADD(at[34], at[89]);    MULADD(at[35], at[88]);    MULADD(at[36], at[87]);    MULADD(at[37], at[86]);    MULADD(at[38], at[85]);    MULADD(at[39], at[84]);    MULADD(at[40], at[83]);    MULADD(at[41], at[82]);    MULADD(at[42], at[81]);    MULADD(at[43], at[80]);    MULADD(at[44], at[79]);    MULADD(at[45], at[78]);    MULADD(at[46], at[77]);    MULADD(at[47], at[76]); 
   COMBA_STORE(C->dp[75]);
   /* 76 */
   COMBA_FORWARD;
   MULADD(at[29], at[95]);    MULADD(at[30], at[94]);    MULADD(at[31], at[93]);    MULADD(at[32], at[92]);    MULADD(at[33], at[91]);    MULADD(at[34], at[90]);    MULADD(at[35], at[89]);    MULADD(at[36], at[88]);    MULADD(at[37], at[87]);    MULADD(at[38], at[86]);    MULADD(at[39], at[85]);    MULADD(at[40], at[84]);    MULADD(at[41], at[83]);    MULADD(at[42], at[82]);    MULADD(at[43], at[81]);    MULADD(at[44], at[80]);    MULADD(at[45], at[79]);    MULADD(at[46], at[78]);    MULADD(at[47], at[77]); 
   COMBA_STORE(C->dp[76]);
   /* 77 */
   COMBA_FORWARD;
   MULADD(at[30], at[95]);    MULADD(at[31], at[94]);    MULADD(at[32], at[93]);    MULADD(at[33], at[92]);    MULADD(at[34], at[91]);    MULADD(at[35], at[90]);    MULADD(at[36], at[89]);    MULADD(at[37], at[88]);    MULADD(at[38], at[87]);    MULADD(at[39], at[86]);    MULADD(at[40], at[85]);    MULADD(at[41], at[84]);    MULADD(at[42], at[83]);    MULADD(at[43], at[82]);    MULADD(at[44], at[81]);    MULADD(at[45], at[80]);    MULADD(at[46], at[79]);    MULADD(at[47], at[78]); 
   COMBA_STORE(C->dp[77]);
   /* 78 */
   COMBA_FORWARD;
   MULADD(at[31], at[95]);    MULADD(at[32], at[94]);    MULADD(at[33], at[93]);    MULADD(at[34], at[92]);    MULADD(at[35], at[91]);    MULADD(at[36], at[90]);    MULADD(at[37], at[89]);    MULADD(at[38], at[88]);    MULADD(at[39], at[87]);    MULADD(at[40], at[86]);    MULADD(at[41], at[85]);    MULADD(at[42], at[84]);    MULADD(at[43], at[83]);    MULADD(at[44], at[82]);    MULADD(at[45], at[81]);    MULADD(at[46], at[80]);    MULADD(at[47], at[79]); 
   COMBA_STORE(C->dp[78]);
   /* 79 */
   COMBA_FORWARD;
   MULADD(at[32], at[95]);    MULADD(at[33], at[94]);    MULADD(at[34], at[93]);    MULADD(at[35], at[92]);    MULADD(at[36], at[91]);    MULADD(at[37], at[90]);    MULADD(at[38], at[89]);    MULADD(at[39], at[88]);    MULADD(at[40], at[87]);    MULADD(at[41], at[86]);    MULADD(at[42], at[85]);    MULADD(at[43], at[84]);    MULADD(at[44], at[83]);    MULADD(at[45], at[82]);    MULADD(at[46], at[81]);    MULADD(at[47], at[80]); 
   COMBA_STORE(C->dp[79]);
   /* 80 */
   COMBA_FORWARD;
   MULADD(at[33], at[95]);    MULADD(at[34], at[94]);    MULADD(at[35], at[93]);    MULADD(at[36], at[92]);    MULADD(at[37], at[91]);    MULADD(at[38], at[90]);    MULADD(at[39], at[89]);    MULADD(at[40], at[88]);    MULADD(at[41], at[87]);    MULADD(at[42], at[86]);    MULADD(at[43], at[85]);    MULADD(at[44], at[84]);    MULADD(at[45], at[83]);    MULADD(at[46], at[82]);    MULADD(at[47], at[81]); 
   COMBA_STORE(C->dp[80]);
   /* 81 */
   COMBA_FORWARD;
   MULADD(at[34], at[95]);    MULADD(at[35], at[94]);    MULADD(at[36], at[93]);    MULADD(at[37], at[92]);    MULADD(at[38], at[91]);    MULADD(at[39], at[90]);    MULADD(at[40], at[89]);    MULADD(at[41], at[88]);    MULADD(at[42], at[87]);    MULADD(at[43], at[86]);    MULADD(at[44], at[85]);    MULADD(at[45], at[84]);    MULADD(at[46], at[83]);    MULADD(at[47], at[82]); 
   COMBA_STORE(C->dp[81]);
   /* 82 */
   COMBA_FORWARD;
   MULADD(at[35], at[95]);    MULADD(at[36], at[94]);    MULADD(at[37], at[93]);    MULADD(at[38], at[92]);    MULADD(at[39], at[91]);    MULADD(at[40], at[90]);    MULADD(at[41], at[89]);    MULADD(at[42], at[88]);    MULADD(at[43], at[87]);    MULADD(at[44], at[86]);    MULADD(at[45], at[85]);    MULADD(at[46], at[84]);    MULADD(at[47], at[83]); 
   COMBA_STORE(C->dp[82]);
   /* 83 */
   COMBA_FORWARD;
   MULADD(at[36], at[95]);    MULADD(at[37], at[94]);    MULADD(at[38], at[93]);    MULADD(at[39], at[92]);    MULADD(at[40], at[91]);    MULADD(at[41], at[90]);    MULADD(at[42], at[89]);    MULADD(at[43], at[88]);    MULADD(at[44], at[87]);    MULADD(at[45], at[86]);    MULADD(at[46], at[85]);    MULADD(at[47], at[84]); 
   COMBA_STORE(C->dp[83]);
   /* 84 */
   COMBA_FORWARD;
   MULADD(at[37], at[95]);    MULADD(at[38], at[94]);    MULADD(at[39], at[93]);    MULADD(at[40], at[92]);    MULADD(at[41], at[91]);    MULADD(at[42], at[90]);    MULADD(at[43], at[89]);    MULADD(at[44], at[88]);    MULADD(at[45], at[87]);    MULADD(at[46], at[86]);    MULADD(at[47], at[85]); 
   COMBA_STORE(C->dp[84]);
   /* 85 */
   COMBA_FORWARD;
   MULADD(at[38], at[95]);    MULADD(at[39], at[94]);    MULADD(at[40], at[93]);    MULADD(at[41], at[92]);    MULADD(at[42], at[91]);    MULADD(at[43], at[90]);    MULADD(at[44], at[89]);    MULADD(at[45], at[88]);    MULADD(at[46], at[87]);    MULADD(at[47], at[86]); 
   COMBA_STORE(C->dp[85]);
   /* 86 */
   COMBA_FORWARD;
   MULADD(at[39], at[95]);    MULADD(at[40], at[94]);    MULADD(at[41], at[93]);    MULADD(at[42], at[92]);    MULADD(at[43], at[91]);    MULADD(at[44], at[90]);    MULADD(at[45], at[89]);    MULADD(at[46], at[88]);    MULADD(at[47], at[87]); 
   COMBA_STORE(C->dp[86]);
   /* 87 */
   COMBA_FORWARD;
   MULADD(at[40], at[95]);    MULADD(at[41], at[94]);    MULADD(at[42], at[93]);    MULADD(at[43], at[92]);    MULADD(at[44], at[91]);    MULADD(at[45], at[90]);    MULADD(at[46], at[89]);    MULADD(at[47], at[88]); 
   COMBA_STORE(C->dp[87]);
   /* 88 */
   COMBA_FORWARD;
   MULADD(at[41], at[95]);    MULADD(at[42], at[94]);    MULADD(at[43], at[93]);    MULADD(at[44], at[92]);    MULADD(at[45], at[91]);    MULADD(at[46], at[90]);    MULADD(at[47], at[89]); 
   COMBA_STORE(C->dp[88]);
   /* 89 */
   COMBA_FORWARD;
   MULADD(at[42], at[95]);    MULADD(at[43], at[94]);    MULADD(at[44], at[93]);    MULADD(at[45], at[92]);    MULADD(at[46], at[91]);    MULADD(at[47], at[90]); 
   COMBA_STORE(C->dp[89]);
   /* 90 */
   COMBA_FORWARD;
   MULADD(at[43], at[95]);    MULADD(at[44], at[94]);    MULADD(at[45], at[93]);    MULADD(at[46], at[92]);    MULADD(at[47], at[91]); 
   COMBA_STORE(C->dp[90]);
   /* 91 */
   COMBA_FORWARD;
   MULADD(at[44], at[95]);    MULADD(at[45], at[94]);    MULADD(at[46], at[93]);    MULADD(at[47], at[92]); 
   COMBA_STORE(C->dp[91]);
   /* 92 */
   COMBA_FORWARD;
   MULADD(at[45], at[95]);    MULADD(at[46], at[94]);    MULADD(at[47], at[93]); 
   COMBA_STORE(C->dp[92]);
   /* 93 */
   COMBA_FORWARD;
   MULADD(at[46], at[95]);    MULADD(at[47], at[94]); 
   COMBA_STORE(C->dp[93]);
   /* 94 */
   COMBA_FORWARD;
   MULADD(at[47], at[95]); 
   COMBA_STORE(C->dp[94]);
   COMBA_STORE2(C->dp[95]);
   C->used = 96;
   C->sign = A->sign ^ B->sign;
   fp_clamp(C);
   COMBA_FINI;
}
#endif

#ifdef TFM_MUL20
void fp_mul_comba20(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[40];
   
   memcpy(at, A->dp, 20 * sizeof(fp_digit));
   memcpy(at+20, B->dp, 20 * sizeof(fp_digit));
   COMBA_START;

   COMBA_CLEAR;
   /* 0 */
   MULADD(at[0], at[20]); 
   COMBA_STORE(C->dp[0]);
   /* 1 */
   COMBA_FORWARD;
   MULADD(at[0], at[21]);    MULADD(at[1], at[20]); 
   COMBA_STORE(C->dp[1]);
   /* 2 */
   COMBA_FORWARD;
   MULADD(at[0], at[22]);    MULADD(at[1], at[21]);    MULADD(at[2], at[20]); 
   COMBA_STORE(C->dp[2]);
   /* 3 */
   COMBA_FORWARD;
   MULADD(at[0], at[23]);    MULADD(at[1], at[22]);    MULADD(at[2], at[21]);    MULADD(at[3], at[20]); 
   COMBA_STORE(C->dp[3]);
   /* 4 */
   COMBA_FORWARD;
   MULADD(at[0], at[24]);    MULADD(at[1], at[23]);    MULADD(at[2], at[22]);    MULADD(at[3], at[21]);    MULADD(at[4], at[20]); 
   COMBA_STORE(C->dp[4]);
   /* 5 */
   COMBA_FORWARD;
   MULADD(at[0], at[25]);    MULADD(at[1], at[24]);    MULADD(at[2], at[23]);    MULADD(at[3], at[22]);    MULADD(at[4], at[21]);    MULADD(at[5], at[20]); 
   COMBA_STORE(C->dp[5]);
   /* 6 */
   COMBA_FORWARD;
   MULADD(at[0], at[26]);    MULADD(at[1], at[25]);    MULADD(at[2], at[24]);    MULADD(at[3], at[23]);    MULADD(at[4], at[22]);    MULADD(at[5], at[21]);    MULADD(at[6], at[20]); 
   COMBA_STORE(C->dp[6]);
   /* 7 */
   COMBA_FORWARD;
   MULADD(at[0], at[27]);    MULADD(at[1], at[26]);    MULADD(at[2], at[25]);    MULADD(at[3], at[24]);    MULADD(at[4], at[23]);    MULADD(at[5], at[22]);    MULADD(at[6], at[21]);    MULADD(at[7], at[20]); 
   COMBA_STORE(C->dp[7]);
   /* 8 */
   COMBA_FORWARD;
   MULADD(at[0], at[28]);    MULADD(at[1], at[27]);    MULADD(at[2], at[26]);    MULADD(at[3], at[25]);    MULADD(at[4], at[24]);    MULADD(at[5], at[23]);    MULADD(at[6], at[22]);    MULADD(at[7], at[21]);    MULADD(at[8], at[20]); 
   COMBA_STORE(C->dp[8]);
   /* 9 */
   COMBA_FORWARD;
   MULADD(at[0], at[29]);    MULADD(at[1], at[28]);    MULADD(at[2], at[27]);    MULADD(at[3], at[26]);    MULADD(at[4], at[25]);    MULADD(at[5], at[24]);    MULADD(at[6], at[23]);    MULADD(at[7], at[22]);    MULADD(at[8], at[21]);    MULADD(at[9], at[20]); 
   COMBA_STORE(C->dp[9]);
   /* 10 */
   COMBA_FORWARD;
   MULADD(at[0], at[30]);    MULADD(at[1], at[29]);    MULADD(at[2], at[28]);    MULADD(at[3], at[27]);    MULADD(at[4], at[26]);    MULADD(at[5], at[25]);    MULADD(at[6], at[24]);    MULADD(at[7], at[23]);    MULADD(at[8], at[22]);    MULADD(at[9], at[21]);    MULADD(at[10], at[20]); 
   COMBA_STORE(C->dp[10]);
   /* 11 */
   COMBA_FORWARD;
   MULADD(at[0], at[31]);    MULADD(at[1], at[30]);    MULADD(at[2], at[29]);    MULADD(at[3], at[28]);    MULADD(at[4], at[27]);    MULADD(at[5], at[26]);    MULADD(at[6], at[25]);    MULADD(at[7], at[24]);    MULADD(at[8], at[23]);    MULADD(at[9], at[22]);    MULADD(at[10], at[21]);    MULADD(at[11], at[20]); 
   COMBA_STORE(C->dp[11]);
   /* 12 */
   COMBA_FORWARD;
   MULADD(at[0], at[32]);    MULADD(at[1], at[31]);    MULADD(at[2], at[30]);    MULADD(at[3], at[29]);    MULADD(at[4], at[28]);    MULADD(at[5], at[27]);    MULADD(at[6], at[26]);    MULADD(at[7], at[25]);    MULADD(at[8], at[24]);    MULADD(at[9], at[23]);    MULADD(at[10], at[22]);    MULADD(at[11], at[21]);    MULADD(at[12], at[20]); 
   COMBA_STORE(C->dp[12]);
   /* 13 */
   COMBA_FORWARD;
   MULADD(at[0], at[33]);    MULADD(at[1], at[32]);    MULADD(at[2], at[31]);    MULADD(at[3], at[30]);    MULADD(at[4], at[29]);    MULADD(at[5], at[28]);    MULADD(at[6], at[27]);    MULADD(at[7], at[26]);    MULADD(at[8], at[25]);    MULADD(at[9], at[24]);    MULADD(at[10], at[23]);    MULADD(at[11], at[22]);    MULADD(at[12], at[21]);    MULADD(at[13], at[20]); 
   COMBA_STORE(C->dp[13]);
   /* 14 */
   COMBA_FORWARD;
   MULADD(at[0], at[34]);    MULADD(at[1], at[33]);    MULADD(at[2], at[32]);    MULADD(at[3], at[31]);    MULADD(at[4], at[30]);    MULADD(at[5], at[29]);    MULADD(at[6], at[28]);    MULADD(at[7], at[27]);    MULADD(at[8], at[26]);    MULADD(at[9], at[25]);    MULADD(at[10], at[24]);    MULADD(at[11], at[23]);    MULADD(at[12], at[22]);    MULADD(at[13], at[21]);    MULADD(at[14], at[20]); 
   COMBA_STORE(C->dp[14]);
   /* 15 */
   COMBA_FORWARD;
   MULADD(at[0], at[35]);    MULADD(at[1], at[34]);    MULADD(at[2], at[33]);    MULADD(at[3], at[32]);    MULADD(at[4], at[31]);    MULADD(at[5], at[30]);    MULADD(at[6], at[29]);    MULADD(at[7], at[28]);    MULADD(at[8], at[27]);    MULADD(at[9], at[26]);    MULADD(at[10], at[25]);    MULADD(at[11], at[24]);    MULADD(at[12], at[23]);    MULADD(at[13], at[22]);    MULADD(at[14], at[21]);    MULADD(at[15], at[20]); 
   COMBA_STORE(C->dp[15]);
   /* 16 */
   COMBA_FORWARD;
   MULADD(at[0], at[36]);    MULADD(at[1], at[35]);    MULADD(at[2], at[34]);    MULADD(at[3], at[33]);    MULADD(at[4], at[32]);    MULADD(at[5], at[31]);    MULADD(at[6], at[30]);    MULADD(at[7], at[29]);    MULADD(at[8], at[28]);    MULADD(at[9], at[27]);    MULADD(at[10], at[26]);    MULADD(at[11], at[25]);    MULADD(at[12], at[24]);    MULADD(at[13], at[23]);    MULADD(at[14], at[22]);    MULADD(at[15], at[21]);    MULADD(at[16], at[20]); 
   COMBA_STORE(C->dp[16]);
   /* 17 */
   COMBA_FORWARD;
   MULADD(at[0], at[37]);    MULADD(at[1], at[36]);    MULADD(at[2], at[35]);    MULADD(at[3], at[34]);    MULADD(at[4], at[33]);    MULADD(at[5], at[32]);    MULADD(at[6], at[31]);    MULADD(at[7], at[30]);    MULADD(at[8], at[29]);    MULADD(at[9], at[28]);    MULADD(at[10], at[27]);    MULADD(at[11], at[26]);    MULADD(at[12], at[25]);    MULADD(at[13], at[24]);    MULADD(at[14], at[23]);    MULADD(at[15], at[22]);    MULADD(at[16], at[21]);    MULADD(at[17], at[20]); 
   COMBA_STORE(C->dp[17]);
   /* 18 */
   COMBA_FORWARD;
   MULADD(at[0], at[38]);    MULADD(at[1], at[37]);    MULADD(at[2], at[36]);    MULADD(at[3], at[35]);    MULADD(at[4], at[34]);    MULADD(at[5], at[33]);    MULADD(at[6], at[32]);    MULADD(at[7], at[31]);    MULADD(at[8], at[30]);    MULADD(at[9], at[29]);    MULADD(at[10], at[28]);    MULADD(at[11], at[27]);    MULADD(at[12], at[26]);    MULADD(at[13], at[25]);    MULADD(at[14], at[24]);    MULADD(at[15], at[23]);    MULADD(at[16], at[22]);    MULADD(at[17], at[21]);    MULADD(at[18], at[20]); 
   COMBA_STORE(C->dp[18]);
   /* 19 */
   COMBA_FORWARD;
   MULADD(at[0], at[39]);    MULADD(at[1], at[38]);    MULADD(at[2], at[37]);    MULADD(at[3], at[36]);    MULADD(at[4], at[35]);    MULADD(at[5], at[34]);    MULADD(at[6], at[33]);    MULADD(at[7], at[32]);    MULADD(at[8], at[31]);    MULADD(at[9], at[30]);    MULADD(at[10], at[29]);    MULADD(at[11], at[28]);    MULADD(at[12], at[27]);    MULADD(at[13], at[26]);    MULADD(at[14], at[25]);    MULADD(at[15], at[24]);    MULADD(at[16], at[23]);    MULADD(at[17], at[22]);    MULADD(at[18], at[21]);    MULADD(at[19], at[20]); 
   COMBA_STORE(C->dp[19]);
   /* 20 */
   COMBA_FORWARD;
   MULADD(at[1], at[39]);    MULADD(at[2], at[38]);    MULADD(at[3], at[37]);    MULADD(at[4], at[36]);    MULADD(at[5], at[35]);    MULADD(at[6], at[34]);    MULADD(at[7], at[33]);    MULADD(at[8], at[32]);    MULADD(at[9], at[31]);    MULADD(at[10], at[30]);    MULADD(at[11], at[29]);    MULADD(at[12], at[28]);    MULADD(at[13], at[27]);    MULADD(at[14], at[26]);    MULADD(at[15], at[25]);    MULADD(at[16], at[24]);    MULADD(at[17], at[23]);    MULADD(at[18], at[22]);    MULADD(at[19], at[21]); 
   COMBA_STORE(C->dp[20]);
   /* 21 */
   COMBA_FORWARD;
   MULADD(at[2], at[39]);    MULADD(at[3], at[38]);    MULADD(at[4], at[37]);    MULADD(at[5], at[36]);    MULADD(at[6], at[35]);    MULADD(at[7], at[34]);    MULADD(at[8], at[33]);    MULADD(at[9], at[32]);    MULADD(at[10], at[31]);    MULADD(at[11], at[30]);    MULADD(at[12], at[29]);    MULADD(at[13], at[28]);    MULADD(at[14], at[27]);    MULADD(at[15], at[26]);    MULADD(at[16], at[25]);    MULADD(at[17], at[24]);    MULADD(at[18], at[23]);    MULADD(at[19], at[22]); 
   COMBA_STORE(C->dp[21]);
   /* 22 */
   COMBA_FORWARD;
   MULADD(at[3], at[39]);    MULADD(at[4], at[38]);    MULADD(at[5], at[37]);    MULADD(at[6], at[36]);    MULADD(at[7], at[35]);    MULADD(at[8], at[34]);    MULADD(at[9], at[33]);    MULADD(at[10], at[32]);    MULADD(at[11], at[31]);    MULADD(at[12], at[30]);    MULADD(at[13], at[29]);    MULADD(at[14], at[28]);    MULADD(at[15], at[27]);    MULADD(at[16], at[26]);    MULADD(at[17], at[25]);    MULADD(at[18], at[24]);    MULADD(at[19], at[23]); 
   COMBA_STORE(C->dp[22]);
   /* 23 */
   COMBA_FORWARD;
   MULADD(at[4], at[39]);    MULADD(at[5], at[38]);    MULADD(at[6], at[37]);    MULADD(at[7], at[36]);    MULADD(at[8], at[35]);    MULADD(at[9], at[34]);    MULADD(at[10], at[33]);    MULADD(at[11], at[32]);    MULADD(at[12], at[31]);    MULADD(at[13], at[30]);    MULADD(at[14], at[29]);    MULADD(at[15], at[28]);    MULADD(at[16], at[27]);    MULADD(at[17], at[26]);    MULADD(at[18], at[25]);    MULADD(at[19], at[24]); 
   COMBA_STORE(C->dp[23]);
   /* 24 */
   COMBA_FORWARD;
   MULADD(at[5], at[39]);    MULADD(at[6], at[38]);    MULADD(at[7], at[37]);    MULADD(at[8], at[36]);    MULADD(at[9], at[35]);    MULADD(at[10], at[34]);    MULADD(at[11], at[33]);    MULADD(at[12], at[32]);    MULADD(at[13], at[31]);    MULADD(at[14], at[30]);    MULADD(at[15], at[29]);    MULADD(at[16], at[28]);    MULADD(at[17], at[27]);    MULADD(at[18], at[26]);    MULADD(at[19], at[25]); 
   COMBA_STORE(C->dp[24]);
   /* 25 */
   COMBA_FORWARD;
   MULADD(at[6], at[39]);    MULADD(at[7], at[38]);    MULADD(at[8], at[37]);    MULADD(at[9], at[36]);    MULADD(at[10], at[35]);    MULADD(at[11], at[34]);    MULADD(at[12], at[33]);    MULADD(at[13], at[32]);    MULADD(at[14], at[31]);    MULADD(at[15], at[30]);    MULADD(at[16], at[29]);    MULADD(at[17], at[28]);    MULADD(at[18], at[27]);    MULADD(at[19], at[26]); 
   COMBA_STORE(C->dp[25]);
   /* 26 */
   COMBA_FORWARD;
   MULADD(at[7], at[39]);    MULADD(at[8], at[38]);    MULADD(at[9], at[37]);    MULADD(at[10], at[36]);    MULADD(at[11], at[35]);    MULADD(at[12], at[34]);    MULADD(at[13], at[33]);    MULADD(at[14], at[32]);    MULADD(at[15], at[31]);    MULADD(at[16], at[30]);    MULADD(at[17], at[29]);    MULADD(at[18], at[28]);    MULADD(at[19], at[27]); 
   COMBA_STORE(C->dp[26]);
   /* 27 */
   COMBA_FORWARD;
   MULADD(at[8], at[39]);    MULADD(at[9], at[38]);    MULADD(at[10], at[37]);    MULADD(at[11], at[36]);    MULADD(at[12], at[35]);    MULADD(at[13], at[34]);    MULADD(at[14], at[33]);    MULADD(at[15], at[32]);    MULADD(at[16], at[31]);    MULADD(at[17], at[30]);    MULADD(at[18], at[29]);    MULADD(at[19], at[28]); 
   COMBA_STORE(C->dp[27]);
   /* 28 */
   COMBA_FORWARD;
   MULADD(at[9], at[39]);    MULADD(at[10], at[38]);    MULADD(at[11], at[37]);    MULADD(at[12], at[36]);    MULADD(at[13], at[35]);    MULADD(at[14], at[34]);    MULADD(at[15], at[33]);    MULADD(at[16], at[32]);    MULADD(at[17], at[31]);    MULADD(at[18], at[30]);    MULADD(at[19], at[29]); 
   COMBA_STORE(C->dp[28]);
   /* 29 */
   COMBA_FORWARD;
   MULADD(at[10], at[39]);    MULADD(at[11], at[38]);    MULADD(at[12], at[37]);    MULADD(at[13], at[36]);    MULADD(at[14], at[35]);    MULADD(at[15], at[34]);    MULADD(at[16], at[33]);    MULADD(at[17], at[32]);    MULADD(at[18], at[31]);    MULADD(at[19], at[30]); 
   COMBA_STORE(C->dp[29]);
   /* 30 */
   COMBA_FORWARD;
   MULADD(at[11], at[39]);    MULADD(at[12], at[38]);    MULADD(at[13], at[37]);    MULADD(at[14], at[36]);    MULADD(at[15], at[35]);    MULADD(at[16], at[34]);    MULADD(at[17], at[33]);    MULADD(at[18], at[32]);    MULADD(at[19], at[31]); 
   COMBA_STORE(C->dp[30]);
   /* 31 */
   COMBA_FORWARD;
   MULADD(at[12], at[39]);    MULADD(at[13], at[38]);    MULADD(at[14], at[37]);    MULADD(at[15], at[36]);    MULADD(at[16], at[35]);    MULADD(at[17], at[34]);    MULADD(at[18], at[33]);    MULADD(at[19], at[32]); 
   COMBA_STORE(C->dp[31]);
   /* 32 */
   COMBA_FORWARD;
   MULADD(at[13], at[39]);    MULADD(at[14], at[38]);    MULADD(at[15], at[37]);    MULADD(at[16], at[36]);    MULADD(at[17], at[35]);    MULADD(at[18], at[34]);    MULADD(at[19], at[33]); 
   COMBA_STORE(C->dp[32]);
   /* 33 */
   COMBA_FORWARD;
   MULADD(at[14], at[39]);    MULADD(at[15], at[38]);    MULADD(at[16], at[37]);    MULADD(at[17], at[36]);    MULADD(at[18], at[35]);    MULADD(at[19], at[34]); 
   COMBA_STORE(C->dp[33]);
   /* 34 */
   COMBA_FORWARD;
   MULADD(at[15], at[39]);    MULADD(at[16], at[38]);    MULADD(at[17], at[37]);    MULADD(at[18], at[36]);    MULADD(at[19], at[35]); 
   COMBA_STORE(C->dp[34]);
   /* 35 */
   COMBA_FORWARD;
   MULADD(at[16], at[39]);    MULADD(at[17], at[38]);    MULADD(at[18], at[37]);    MULADD(at[19], at[36]); 
   COMBA_STORE(C->dp[35]);
   /* 36 */
   COMBA_FORWARD;
   MULADD(at[17], at[39]);    MULADD(at[18], at[38]);    MULADD(at[19], at[37]); 
   COMBA_STORE(C->dp[36]);
   /* 37 */
   COMBA_FORWARD;
   MULADD(at[18], at[39]);    MULADD(at[19], at[38]); 
   COMBA_STORE(C->dp[37]);
   /* 38 */
   COMBA_FORWARD;
   MULADD(at[19], at[39]); 
   COMBA_STORE(C->dp[38]);
   COMBA_STORE2(C->dp[39]);
   C->used = 40;
   C->sign = A->sign ^ B->sign;
   fp_clamp(C);
   COMBA_FINI;
}
#endif

#ifdef TFM_MUL28
void fp_mul_comba28(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[56];

   memcpy(at, A->dp, 28 * sizeof(fp_digit));
   memcpy(at+28, B->dp, 28 * sizeof(fp_digit));
   COMBA_START;

   COMBA_CLEAR;
   /* 0 */
   MULADD(at[0], at[28]); 
   COMBA_STORE(C->dp[0]);
   /* 1 */
   COMBA_FORWARD;
   MULADD(at[0], at[29]);    MULADD(at[1], at[28]); 
   COMBA_STORE(C->dp[1]);
   /* 2 */
   COMBA_FORWARD;
   MULADD(at[0], at[30]);    MULADD(at[1], at[29]);    MULADD(at[2], at[28]); 
   COMBA_STORE(C->dp[2]);
   /* 3 */
   COMBA_FORWARD;
   MULADD(at[0], at[31]);    MULADD(at[1], at[30]);    MULADD(at[2], at[29]);    MULADD(at[3], at[28]); 
   COMBA_STORE(C->dp[3]);
   /* 4 */
   COMBA_FORWARD;
   MULADD(at[0], at[32]);    MULADD(at[1], at[31]);    MULADD(at[2], at[30]);    MULADD(at[3], at[29]);    MULADD(at[4], at[28]); 
   COMBA_STORE(C->dp[4]);
   /* 5 */
   COMBA_FORWARD;
   MULADD(at[0], at[33]);    MULADD(at[1], at[32]);    MULADD(at[2], at[31]);    MULADD(at[3], at[30]);    MULADD(at[4], at[29]);    MULADD(at[5], at[28]); 
   COMBA_STORE(C->dp[5]);
   /* 6 */
   COMBA_FORWARD;
   MULADD(at[0], at[34]);    MULADD(at[1], at[33]);    MULADD(at[2], at[32]);    MULADD(at[3], at[31]);    MULADD(at[4], at[30]);    MULADD(at[5], at[29]);    MULADD(at[6], at[28]); 
   COMBA_STORE(C->dp[6]);
   /* 7 */
   COMBA_FORWARD;
   MULADD(at[0], at[35]);    MULADD(at[1], at[34]);    MULADD(at[2], at[33]);    MULADD(at[3], at[32]);    MULADD(at[4], at[31]);    MULADD(at[5], at[30]);    MULADD(at[6], at[29]);    MULADD(at[7], at[28]); 
   COMBA_STORE(C->dp[7]);
   /* 8 */
   COMBA_FORWARD;
   MULADD(at[0], at[36]);    MULADD(at[1], at[35]);    MULADD(at[2], at[34]);    MULADD(at[3], at[33]);    MULADD(at[4], at[32]);    MULADD(at[5], at[31]);    MULADD(at[6], at[30]);    MULADD(at[7], at[29]);    MULADD(at[8], at[28]); 
   COMBA_STORE(C->dp[8]);
   /* 9 */
   COMBA_FORWARD;
   MULADD(at[0], at[37]);    MULADD(at[1], at[36]);    MULADD(at[2], at[35]);    MULADD(at[3], at[34]);    MULADD(at[4], at[33]);    MULADD(at[5], at[32]);    MULADD(at[6], at[31]);    MULADD(at[7], at[30]);    MULADD(at[8], at[29]);    MULADD(at[9], at[28]); 
   COMBA_STORE(C->dp[9]);
   /* 10 */
   COMBA_FORWARD;
   MULADD(at[0], at[38]);    MULADD(at[1], at[37]);    MULADD(at[2], at[36]);    MULADD(at[3], at[35]);    MULADD(at[4], at[34]);    MULADD(at[5], at[33]);    MULADD(at[6], at[32]);    MULADD(at[7], at[31]);    MULADD(at[8], at[30]);    MULADD(at[9], at[29]);    MULADD(at[10], at[28]); 
   COMBA_STORE(C->dp[10]);
   /* 11 */
   COMBA_FORWARD;
   MULADD(at[0], at[39]);    MULADD(at[1], at[38]);    MULADD(at[2], at[37]);    MULADD(at[3], at[36]);    MULADD(at[4], at[35]);    MULADD(at[5], at[34]);    MULADD(at[6], at[33]);    MULADD(at[7], at[32]);    MULADD(at[8], at[31]);    MULADD(at[9], at[30]);    MULADD(at[10], at[29]);    MULADD(at[11], at[28]); 
   COMBA_STORE(C->dp[11]);
   /* 12 */
   COMBA_FORWARD;
   MULADD(at[0], at[40]);    MULADD(at[1], at[39]);    MULADD(at[2], at[38]);    MULADD(at[3], at[37]);    MULADD(at[4], at[36]);    MULADD(at[5], at[35]);    MULADD(at[6], at[34]);    MULADD(at[7], at[33]);    MULADD(at[8], at[32]);    MULADD(at[9], at[31]);    MULADD(at[10], at[30]);    MULADD(at[11], at[29]);    MULADD(at[12], at[28]); 
   COMBA_STORE(C->dp[12]);
   /* 13 */
   COMBA_FORWARD;
   MULADD(at[0], at[41]);    MULADD(at[1], at[40]);    MULADD(at[2], at[39]);    MULADD(at[3], at[38]);    MULADD(at[4], at[37]);    MULADD(at[5], at[36]);    MULADD(at[6], at[35]);    MULADD(at[7], at[34]);    MULADD(at[8], at[33]);    MULADD(at[9], at[32]);    MULADD(at[10], at[31]);    MULADD(at[11], at[30]);    MULADD(at[12], at[29]);    MULADD(at[13], at[28]); 
   COMBA_STORE(C->dp[13]);
   /* 14 */
   COMBA_FORWARD;
   MULADD(at[0], at[42]);    MULADD(at[1], at[41]);    MULADD(at[2], at[40]);    MULADD(at[3], at[39]);    MULADD(at[4], at[38]);    MULADD(at[5], at[37]);    MULADD(at[6], at[36]);    MULADD(at[7], at[35]);    MULADD(at[8], at[34]);    MULADD(at[9], at[33]);    MULADD(at[10], at[32]);    MULADD(at[11], at[31]);    MULADD(at[12], at[30]);    MULADD(at[13], at[29]);    MULADD(at[14], at[28]); 
   COMBA_STORE(C->dp[14]);
   /* 15 */
   COMBA_FORWARD;
   MULADD(at[0], at[43]);    MULADD(at[1], at[42]);    MULADD(at[2], at[41]);    MULADD(at[3], at[40]);    MULADD(at[4], at[39]);    MULADD(at[5], at[38]);    MULADD(at[6], at[37]);    MULADD(at[7], at[36]);    MULADD(at[8], at[35]);    MULADD(at[9], at[34]);    MULADD(at[10], at[33]);    MULADD(at[11], at[32]);    MULADD(at[12], at[31]);    MULADD(at[13], at[30]);    MULADD(at[14], at[29]);    MULADD(at[15], at[28]); 
   COMBA_STORE(C->dp[15]);
   /* 16 */
   COMBA_FORWARD;
   MULADD(at[0], at[44]);    MULADD(at[1], at[43]);    MULADD(at[2], at[42]);    MULADD(at[3], at[41]);    MULADD(at[4], at[40]);    MULADD(at[5], at[39]);    MULADD(at[6], at[38]);    MULADD(at[7], at[37]);    MULADD(at[8], at[36]);    MULADD(at[9], at[35]);    MULADD(at[10], at[34]);    MULADD(at[11], at[33]);    MULADD(at[12], at[32]);    MULADD(at[13], at[31]);    MULADD(at[14], at[30]);    MULADD(at[15], at[29]);    MULADD(at[16], at[28]); 
   COMBA_STORE(C->dp[16]);
   /* 17 */
   COMBA_FORWARD;
   MULADD(at[0], at[45]);    MULADD(at[1], at[44]);    MULADD(at[2], at[43]);    MULADD(at[3], at[42]);    MULADD(at[4], at[41]);    MULADD(at[5], at[40]);    MULADD(at[6], at[39]);    MULADD(at[7], at[38]);    MULADD(at[8], at[37]);    MULADD(at[9], at[36]);    MULADD(at[10], at[35]);    MULADD(at[11], at[34]);    MULADD(at[12], at[33]);    MULADD(at[13], at[32]);    MULADD(at[14], at[31]);    MULADD(at[15], at[30]);    MULADD(at[16], at[29]);    MULADD(at[17], at[28]); 
   COMBA_STORE(C->dp[17]);
   /* 18 */
   COMBA_FORWARD;
   MULADD(at[0], at[46]);    MULADD(at[1], at[45]);    MULADD(at[2], at[44]);    MULADD(at[3], at[43]);    MULADD(at[4], at[42]);    MULADD(at[5], at[41]);    MULADD(at[6], at[40]);    MULADD(at[7], at[39]);    MULADD(at[8], at[38]);    MULADD(at[9], at[37]);    MULADD(at[10], at[36]);    MULADD(at[11], at[35]);    MULADD(at[12], at[34]);    MULADD(at[13], at[33]);    MULADD(at[14], at[32]);    MULADD(at[15], at[31]);    MULADD(at[16], at[30]);    MULADD(at[17], at[29]);    MULADD(at[18], at[28]); 
   COMBA_STORE(C->dp[18]);
   /* 19 */
   COMBA_FORWARD;
   MULADD(at[0], at[47]);    MULADD(at[1], at[46]);    MULADD(at[2], at[45]);    MULADD(at[3], at[44]);    MULADD(at[4], at[43]);    MULADD(at[5], at[42]);    MULADD(at[6], at[41]);    MULADD(at[7], at[40]);    MULADD(at[8], at[39]);    MULADD(at[9], at[38]);    MULADD(at[10], at[37]);    MULADD(at[11], at[36]);    MULADD(at[12], at[35]);    MULADD(at[13], at[34]);    MULADD(at[14], at[33]);    MULADD(at[15], at[32]);    MULADD(at[16], at[31]);    MULADD(at[17], at[30]);    MULADD(at[18], at[29]);    MULADD(at[19], at[28]); 
   COMBA_STORE(C->dp[19]);
   /* 20 */
   COMBA_FORWARD;
   MULADD(at[0], at[48]);    MULADD(at[1], at[47]);    MULADD(at[2], at[46]);    MULADD(at[3], at[45]);    MULADD(at[4], at[44]);    MULADD(at[5], at[43]);    MULADD(at[6], at[42]);    MULADD(at[7], at[41]);    MULADD(at[8], at[40]);    MULADD(at[9], at[39]);    MULADD(at[10], at[38]);    MULADD(at[11], at[37]);    MULADD(at[12], at[36]);    MULADD(at[13], at[35]);    MULADD(at[14], at[34]);    MULADD(at[15], at[33]);    MULADD(at[16], at[32]);    MULADD(at[17], at[31]);    MULADD(at[18], at[30]);    MULADD(at[19], at[29]);    MULADD(at[20], at[28]); 
   COMBA_STORE(C->dp[20]);
   /* 21 */
   COMBA_FORWARD;
   MULADD(at[0], at[49]);    MULADD(at[1], at[48]);    MULADD(at[2], at[47]);    MULADD(at[3], at[46]);    MULADD(at[4], at[45]);    MULADD(at[5], at[44]);    MULADD(at[6], at[43]);    MULADD(at[7], at[42]);    MULADD(at[8], at[41]);    MULADD(at[9], at[40]);    MULADD(at[10], at[39]);    MULADD(at[11], at[38]);    MULADD(at[12], at[37]);    MULADD(at[13], at[36]);    MULADD(at[14], at[35]);    MULADD(at[15], at[34]);    MULADD(at[16], at[33]);    MULADD(at[17], at[32]);    MULADD(at[18], at[31]);    MULADD(at[19], at[30]);    MULADD(at[20], at[29]);    MULADD(at[21], at[28]); 
   COMBA_STORE(C->dp[21]);
   /* 22 */
   COMBA_FORWARD;
   MULADD(at[0], at[50]);    MULADD(at[1], at[49]);    MULADD(at[2], at[48]);    MULADD(at[3], at[47]);    MULADD(at[4], at[46]);    MULADD(at[5], at[45]);    MULADD(at[6], at[44]);    MULADD(at[7], at[43]);    MULADD(at[8], at[42]);    MULADD(at[9], at[41]);    MULADD(at[10], at[40]);    MULADD(at[11], at[39]);    MULADD(at[12], at[38]);    MULADD(at[13], at[37]);    MULADD(at[14], at[36]);    MULADD(at[15], at[35]);    MULADD(at[16], at[34]);    MULADD(at[17], at[33]);    MULADD(at[18], at[32]);    MULADD(at[19], at[31]);    MULADD(at[20], at[30]);    MULADD(at[21], at[29]);    MULADD(at[22], at[28]); 
   COMBA_STORE(C->dp[22]);
   /* 23 */
   COMBA_FORWARD;
   MULADD(at[0], at[51]);    MULADD(at[1], at[50]);    MULADD(at[2], at[49]);    MULADD(at[3], at[48]);    MULADD(at[4], at[47]);    MULADD(at[5], at[46]);    MULADD(at[6], at[45]);    MULADD(at[7], at[44]);    MULADD(at[8], at[43]);    MULADD(at[9], at[42]);    MULADD(at[10], at[41]);    MULADD(at[11], at[40]);    MULADD(at[12], at[39]);    MULADD(at[13], at[38]);    MULADD(at[14], at[37]);    MULADD(at[15], at[36]);    MULADD(at[16], at[35]);    MULADD(at[17], at[34]);    MULADD(at[18], at[33]);    MULADD(at[19], at[32]);    MULADD(at[20], at[31]);    MULADD(at[21], at[30]);    MULADD(at[22], at[29]);    MULADD(at[23], at[28]); 
   COMBA_STORE(C->dp[23]);
   /* 24 */
   COMBA_FORWARD;
   MULADD(at[0], at[52]);    MULADD(at[1], at[51]);    MULADD(at[2], at[50]);    MULADD(at[3], at[49]);    MULADD(at[4], at[48]);    MULADD(at[5], at[47]);    MULADD(at[6], at[46]);    MULADD(at[7], at[45]);    MULADD(at[8], at[44]);    MULADD(at[9], at[43]);    MULADD(at[10], at[42]);    MULADD(at[11], at[41]);    MULADD(at[12], at[40]);    MULADD(at[13], at[39]);    MULADD(at[14], at[38]);    MULADD(at[15], at[37]);    MULADD(at[16], at[36]);    MULADD(at[17], at[35]);    MULADD(at[18], at[34]);    MULADD(at[19], at[33]);    MULADD(at[20], at[32]);    MULADD(at[21], at[31]);    MULADD(at[22], at[30]);    MULADD(at[23], at[29]);    MULADD(at[24], at[28]); 
   COMBA_STORE(C->dp[24]);
   /* 25 */
   COMBA_FORWARD;
   MULADD(at[0], at[53]);    MULADD(at[1], at[52]);    MULADD(at[2], at[51]);    MULADD(at[3], at[50]);    MULADD(at[4], at[49]);    MULADD(at[5], at[48]);    MULADD(at[6], at[47]);    MULADD(at[7], at[46]);    MULADD(at[8], at[45]);    MULADD(at[9], at[44]);    MULADD(at[10], at[43]);    MULADD(at[11], at[42]);    MULADD(at[12], at[41]);    MULADD(at[13], at[40]);    MULADD(at[14], at[39]);    MULADD(at[15], at[38]);    MULADD(at[16], at[37]);    MULADD(at[17], at[36]);    MULADD(at[18], at[35]);    MULADD(at[19], at[34]);    MULADD(at[20], at[33]);    MULADD(at[21], at[32]);    MULADD(at[22], at[31]);    MULADD(at[23], at[30]);    MULADD(at[24], at[29]);    MULADD(at[25], at[28]); 
   COMBA_STORE(C->dp[25]);
   /* 26 */
   COMBA_FORWARD;
   MULADD(at[0], at[54]);    MULADD(at[1], at[53]);    MULADD(at[2], at[52]);    MULADD(at[3], at[51]);    MULADD(at[4], at[50]);    MULADD(at[5], at[49]);    MULADD(at[6], at[48]);    MULADD(at[7], at[47]);    MULADD(at[8], at[46]);    MULADD(at[9], at[45]);    MULADD(at[10], at[44]);    MULADD(at[11], at[43]);    MULADD(at[12], at[42]);    MULADD(at[13], at[41]);    MULADD(at[14], at[40]);    MULADD(at[15], at[39]);    MULADD(at[16], at[38]);    MULADD(at[17], at[37]);    MULADD(at[18], at[36]);    MULADD(at[19], at[35]);    MULADD(at[20], at[34]);    MULADD(at[21], at[33]);    MULADD(at[22], at[32]);    MULADD(at[23], at[31]);    MULADD(at[24], at[30]);    MULADD(at[25], at[29]);    MULADD(at[26], at[28]); 
   COMBA_STORE(C->dp[26]);
   /* 27 */
   COMBA_FORWARD;
   MULADD(at[0], at[55]);    MULADD(at[1], at[54]);    MULADD(at[2], at[53]);    MULADD(at[3], at[52]);    MULADD(at[4], at[51]);    MULADD(at[5], at[50]);    MULADD(at[6], at[49]);    MULADD(at[7], at[48]);    MULADD(at[8], at[47]);    MULADD(at[9], at[46]);    MULADD(at[10], at[45]);    MULADD(at[11], at[44]);    MULADD(at[12], at[43]);    MULADD(at[13], at[42]);    MULADD(at[14], at[41]);    MULADD(at[15], at[40]);    MULADD(at[16], at[39]);    MULADD(at[17], at[38]);    MULADD(at[18], at[37]);    MULADD(at[19], at[36]);    MULADD(at[20], at[35]);    MULADD(at[21], at[34]);    MULADD(at[22], at[33]);    MULADD(at[23], at[32]);    MULADD(at[24], at[31]);    MULADD(at[25], at[30]);    MULADD(at[26], at[29]);    MULADD(at[27], at[28]); 
   COMBA_STORE(C->dp[27]);
   /* 28 */
   COMBA_FORWARD;
   MULADD(at[1], at[55]);    MULADD(at[2], at[54]);    MULADD(at[3], at[53]);    MULADD(at[4], at[52]);    MULADD(at[5], at[51]);    MULADD(at[6], at[50]);    MULADD(at[7], at[49]);    MULADD(at[8], at[48]);    MULADD(at[9], at[47]);    MULADD(at[10], at[46]);    MULADD(at[11], at[45]);    MULADD(at[12], at[44]);    MULADD(at[13], at[43]);    MULADD(at[14], at[42]);    MULADD(at[15], at[41]);    MULADD(at[16], at[40]);    MULADD(at[17], at[39]);    MULADD(at[18], at[38]);    MULADD(at[19], at[37]);    MULADD(at[20], at[36]);    MULADD(at[21], at[35]);    MULADD(at[22], at[34]);    MULADD(at[23], at[33]);    MULADD(at[24], at[32]);    MULADD(at[25], at[31]);    MULADD(at[26], at[30]);    MULADD(at[27], at[29]); 
   COMBA_STORE(C->dp[28]);
   /* 29 */
   COMBA_FORWARD;
   MULADD(at[2], at[55]);    MULADD(at[3], at[54]);    MULADD(at[4], at[53]);    MULADD(at[5], at[52]);    MULADD(at[6], at[51]);    MULADD(at[7], at[50]);    MULADD(at[8], at[49]);    MULADD(at[9], at[48]);    MULADD(at[10], at[47]);    MULADD(at[11], at[46]);    MULADD(at[12], at[45]);    MULADD(at[13], at[44]);    MULADD(at[14], at[43]);    MULADD(at[15], at[42]);    MULADD(at[16], at[41]);    MULADD(at[17], at[40]);    MULADD(at[18], at[39]);    MULADD(at[19], at[38]);    MULADD(at[20], at[37]);    MULADD(at[21], at[36]);    MULADD(at[22], at[35]);    MULADD(at[23], at[34]);    MULADD(at[24], at[33]);    MULADD(at[25], at[32]);    MULADD(at[26], at[31]);    MULADD(at[27], at[30]); 
   COMBA_STORE(C->dp[29]);
   /* 30 */
   COMBA_FORWARD;
   MULADD(at[3], at[55]);    MULADD(at[4], at[54]);    MULADD(at[5], at[53]);    MULADD(at[6], at[52]);    MULADD(at[7], at[51]);    MULADD(at[8], at[50]);    MULADD(at[9], at[49]);    MULADD(at[10], at[48]);    MULADD(at[11], at[47]);    MULADD(at[12], at[46]);    MULADD(at[13], at[45]);    MULADD(at[14], at[44]);    MULADD(at[15], at[43]);    MULADD(at[16], at[42]);    MULADD(at[17], at[41]);    MULADD(at[18], at[40]);    MULADD(at[19], at[39]);    MULADD(at[20], at[38]);    MULADD(at[21], at[37]);    MULADD(at[22], at[36]);    MULADD(at[23], at[35]);    MULADD(at[24], at[34]);    MULADD(at[25], at[33]);    MULADD(at[26], at[32]);    MULADD(at[27], at[31]); 
   COMBA_STORE(C->dp[30]);
   /* 31 */
   COMBA_FORWARD;
   MULADD(at[4], at[55]);    MULADD(at[5], at[54]);    MULADD(at[6], at[53]);    MULADD(at[7], at[52]);    MULADD(at[8], at[51]);    MULADD(at[9], at[50]);    MULADD(at[10], at[49]);    MULADD(at[11], at[48]);    MULADD(at[12], at[47]);    MULADD(at[13], at[46]);    MULADD(at[14], at[45]);    MULADD(at[15], at[44]);    MULADD(at[16], at[43]);    MULADD(at[17], at[42]);    MULADD(at[18], at[41]);    MULADD(at[19], at[40]);    MULADD(at[20], at[39]);    MULADD(at[21], at[38]);    MULADD(at[22], at[37]);    MULADD(at[23], at[36]);    MULADD(at[24], at[35]);    MULADD(at[25], at[34]);    MULADD(at[26], at[33]);    MULADD(at[27], at[32]); 
   COMBA_STORE(C->dp[31]);
   /* 32 */
   COMBA_FORWARD;
   MULADD(at[5], at[55]);    MULADD(at[6], at[54]);    MULADD(at[7], at[53]);    MULADD(at[8], at[52]);    MULADD(at[9], at[51]);    MULADD(at[10], at[50]);    MULADD(at[11], at[49]);    MULADD(at[12], at[48]);    MULADD(at[13], at[47]);    MULADD(at[14], at[46]);    MULADD(at[15], at[45]);    MULADD(at[16], at[44]);    MULADD(at[17], at[43]);    MULADD(at[18], at[42]);    MULADD(at[19], at[41]);    MULADD(at[20], at[40]);    MULADD(at[21], at[39]);    MULADD(at[22], at[38]);    MULADD(at[23], at[37]);    MULADD(at[24], at[36]);    MULADD(at[25], at[35]);    MULADD(at[26], at[34]);    MULADD(at[27], at[33]); 
   COMBA_STORE(C->dp[32]);
   /* 33 */
   COMBA_FORWARD;
   MULADD(at[6], at[55]);    MULADD(at[7], at[54]);    MULADD(at[8], at[53]);    MULADD(at[9], at[52]);    MULADD(at[10], at[51]);    MULADD(at[11], at[50]);    MULADD(at[12], at[49]);    MULADD(at[13], at[48]);    MULADD(at[14], at[47]);    MULADD(at[15], at[46]);    MULADD(at[16], at[45]);    MULADD(at[17], at[44]);    MULADD(at[18], at[43]);    MULADD(at[19], at[42]);    MULADD(at[20], at[41]);    MULADD(at[21], at[40]);    MULADD(at[22], at[39]);    MULADD(at[23], at[38]);    MULADD(at[24], at[37]);    MULADD(at[25], at[36]);    MULADD(at[26], at[35]);    MULADD(at[27], at[34]); 
   COMBA_STORE(C->dp[33]);
   /* 34 */
   COMBA_FORWARD;
   MULADD(at[7], at[55]);    MULADD(at[8], at[54]);    MULADD(at[9], at[53]);    MULADD(at[10], at[52]);    MULADD(at[11], at[51]);    MULADD(at[12], at[50]);    MULADD(at[13], at[49]);    MULADD(at[14], at[48]);    MULADD(at[15], at[47]);    MULADD(at[16], at[46]);    MULADD(at[17], at[45]);    MULADD(at[18], at[44]);    MULADD(at[19], at[43]);    MULADD(at[20], at[42]);    MULADD(at[21], at[41]);    MULADD(at[22], at[40]);    MULADD(at[23], at[39]);    MULADD(at[24], at[38]);    MULADD(at[25], at[37]);    MULADD(at[26], at[36]);    MULADD(at[27], at[35]); 
   COMBA_STORE(C->dp[34]);
   /* 35 */
   COMBA_FORWARD;
   MULADD(at[8], at[55]);    MULADD(at[9], at[54]);    MULADD(at[10], at[53]);    MULADD(at[11], at[52]);    MULADD(at[12], at[51]);    MULADD(at[13], at[50]);    MULADD(at[14], at[49]);    MULADD(at[15], at[48]);    MULADD(at[16], at[47]);    MULADD(at[17], at[46]);    MULADD(at[18], at[45]);    MULADD(at[19], at[44]);    MULADD(at[20], at[43]);    MULADD(at[21], at[42]);    MULADD(at[22], at[41]);    MULADD(at[23], at[40]);    MULADD(at[24], at[39]);    MULADD(at[25], at[38]);    MULADD(at[26], at[37]);    MULADD(at[27], at[36]); 
   COMBA_STORE(C->dp[35]);
   /* 36 */
   COMBA_FORWARD;
   MULADD(at[9], at[55]);    MULADD(at[10], at[54]);    MULADD(at[11], at[53]);    MULADD(at[12], at[52]);    MULADD(at[13], at[51]);    MULADD(at[14], at[50]);    MULADD(at[15], at[49]);    MULADD(at[16], at[48]);    MULADD(at[17], at[47]);    MULADD(at[18], at[46]);    MULADD(at[19], at[45]);    MULADD(at[20], at[44]);    MULADD(at[21], at[43]);    MULADD(at[22], at[42]);    MULADD(at[23], at[41]);    MULADD(at[24], at[40]);    MULADD(at[25], at[39]);    MULADD(at[26], at[38]);    MULADD(at[27], at[37]); 
   COMBA_STORE(C->dp[36]);
   /* 37 */
   COMBA_FORWARD;
   MULADD(at[10], at[55]);    MULADD(at[11], at[54]);    MULADD(at[12], at[53]);    MULADD(at[13], at[52]);    MULADD(at[14], at[51]);    MULADD(at[15], at[50]);    MULADD(at[16], at[49]);    MULADD(at[17], at[48]);    MULADD(at[18], at[47]);    MULADD(at[19], at[46]);    MULADD(at[20], at[45]);    MULADD(at[21], at[44]);    MULADD(at[22], at[43]);    MULADD(at[23], at[42]);    MULADD(at[24], at[41]);    MULADD(at[25], at[40]);    MULADD(at[26], at[39]);    MULADD(at[27], at[38]); 
   COMBA_STORE(C->dp[37]);
   /* 38 */
   COMBA_FORWARD;
   MULADD(at[11], at[55]);    MULADD(at[12], at[54]);    MULADD(at[13], at[53]);    MULADD(at[14], at[52]);    MULADD(at[15], at[51]);    MULADD(at[16], at[50]);    MULADD(at[17], at[49]);    MULADD(at[18], at[48]);    MULADD(at[19], at[47]);    MULADD(at[20], at[46]);    MULADD(at[21], at[45]);    MULADD(at[22], at[44]);    MULADD(at[23], at[43]);    MULADD(at[24], at[42]);    MULADD(at[25], at[41]);    MULADD(at[26], at[40]);    MULADD(at[27], at[39]); 
   COMBA_STORE(C->dp[38]);
   /* 39 */
   COMBA_FORWARD;
   MULADD(at[12], at[55]);    MULADD(at[13], at[54]);    MULADD(at[14], at[53]);    MULADD(at[15], at[52]);    MULADD(at[16], at[51]);    MULADD(at[17], at[50]);    MULADD(at[18], at[49]);    MULADD(at[19], at[48]);    MULADD(at[20], at[47]);    MULADD(at[21], at[46]);    MULADD(at[22], at[45]);    MULADD(at[23], at[44]);    MULADD(at[24], at[43]);    MULADD(at[25], at[42]);    MULADD(at[26], at[41]);    MULADD(at[27], at[40]); 
   COMBA_STORE(C->dp[39]);
   /* 40 */
   COMBA_FORWARD;
   MULADD(at[13], at[55]);    MULADD(at[14], at[54]);    MULADD(at[15], at[53]);    MULADD(at[16], at[52]);    MULADD(at[17], at[51]);    MULADD(at[18], at[50]);    MULADD(at[19], at[49]);    MULADD(at[20], at[48]);    MULADD(at[21], at[47]);    MULADD(at[22], at[46]);    MULADD(at[23], at[45]);    MULADD(at[24], at[44]);    MULADD(at[25], at[43]);    MULADD(at[26], at[42]);    MULADD(at[27], at[41]); 
   COMBA_STORE(C->dp[40]);
   /* 41 */
   COMBA_FORWARD;
   MULADD(at[14], at[55]);    MULADD(at[15], at[54]);    MULADD(at[16], at[53]);    MULADD(at[17], at[52]);    MULADD(at[18], at[51]);    MULADD(at[19], at[50]);    MULADD(at[20], at[49]);    MULADD(at[21], at[48]);    MULADD(at[22], at[47]);    MULADD(at[23], at[46]);    MULADD(at[24], at[45]);    MULADD(at[25], at[44]);    MULADD(at[26], at[43]);    MULADD(at[27], at[42]); 
   COMBA_STORE(C->dp[41]);
   /* 42 */
   COMBA_FORWARD;
   MULADD(at[15], at[55]);    MULADD(at[16], at[54]);    MULADD(at[17], at[53]);    MULADD(at[18], at[52]);    MULADD(at[19], at[51]);    MULADD(at[20], at[50]);    MULADD(at[21], at[49]);    MULADD(at[22], at[48]);    MULADD(at[23], at[47]);    MULADD(at[24], at[46]);    MULADD(at[25], at[45]);    MULADD(at[26], at[44]);    MULADD(at[27], at[43]); 
   COMBA_STORE(C->dp[42]);
   /* 43 */
   COMBA_FORWARD;
   MULADD(at[16], at[55]);    MULADD(at[17], at[54]);    MULADD(at[18], at[53]);    MULADD(at[19], at[52]);    MULADD(at[20], at[51]);    MULADD(at[21], at[50]);    MULADD(at[22], at[49]);    MULADD(at[23], at[48]);    MULADD(at[24], at[47]);    MULADD(at[25], at[46]);    MULADD(at[26], at[45]);    MULADD(at[27], at[44]); 
   COMBA_STORE(C->dp[43]);
   /* 44 */
   COMBA_FORWARD;
   MULADD(at[17], at[55]);    MULADD(at[18], at[54]);    MULADD(at[19], at[53]);    MULADD(at[20], at[52]);    MULADD(at[21], at[51]);    MULADD(at[22], at[50]);    MULADD(at[23], at[49]);    MULADD(at[24], at[48]);    MULADD(at[25], at[47]);    MULADD(at[26], at[46]);    MULADD(at[27], at[45]); 
   COMBA_STORE(C->dp[44]);
   /* 45 */
   COMBA_FORWARD;
   MULADD(at[18], at[55]);    MULADD(at[19], at[54]);    MULADD(at[20], at[53]);    MULADD(at[21], at[52]);    MULADD(at[22], at[51]);    MULADD(at[23], at[50]);    MULADD(at[24], at[49]);    MULADD(at[25], at[48]);    MULADD(at[26], at[47]);    MULADD(at[27], at[46]); 
   COMBA_STORE(C->dp[45]);
   /* 46 */
   COMBA_FORWARD;
   MULADD(at[19], at[55]);    MULADD(at[20], at[54]);    MULADD(at[21], at[53]);    MULADD(at[22], at[52]);    MULADD(at[23], at[51]);    MULADD(at[24], at[50]);    MULADD(at[25], at[49]);    MULADD(at[26], at[48]);    MULADD(at[27], at[47]); 
   COMBA_STORE(C->dp[46]);
   /* 47 */
   COMBA_FORWARD;
   MULADD(at[20], at[55]);    MULADD(at[21], at[54]);    MULADD(at[22], at[53]);    MULADD(at[23], at[52]);    MULADD(at[24], at[51]);    MULADD(at[25], at[50]);    MULADD(at[26], at[49]);    MULADD(at[27], at[48]); 
   COMBA_STORE(C->dp[47]);
   /* 48 */
   COMBA_FORWARD;
   MULADD(at[21], at[55]);    MULADD(at[22], at[54]);    MULADD(at[23], at[53]);    MULADD(at[24], at[52]);    MULADD(at[25], at[51]);    MULADD(at[26], at[50]);    MULADD(at[27], at[49]); 
   COMBA_STORE(C->dp[48]);
   /* 49 */
   COMBA_FORWARD;
   MULADD(at[22], at[55]);    MULADD(at[23], at[54]);    MULADD(at[24], at[53]);    MULADD(at[25], at[52]);    MULADD(at[26], at[51]);    MULADD(at[27], at[50]); 
   COMBA_STORE(C->dp[49]);
   /* 50 */
   COMBA_FORWARD;
   MULADD(at[23], at[55]);    MULADD(at[24], at[54]);    MULADD(at[25], at[53]);    MULADD(at[26], at[52]);    MULADD(at[27], at[51]); 
   COMBA_STORE(C->dp[50]);
   /* 51 */
   COMBA_FORWARD;
   MULADD(at[24], at[55]);    MULADD(at[25], at[54]);    MULADD(at[26], at[53]);    MULADD(at[27], at[52]); 
   COMBA_STORE(C->dp[51]);
   /* 52 */
   COMBA_FORWARD;
   MULADD(at[25], at[55]);    MULADD(at[26], at[54]);    MULADD(at[27], at[53]); 
   COMBA_STORE(C->dp[52]);
   /* 53 */
   COMBA_FORWARD;
   MULADD(at[26], at[55]);    MULADD(at[27], at[54]); 
   COMBA_STORE(C->dp[53]);
   /* 54 */
   COMBA_FORWARD;
   MULADD(at[27], at[55]); 
   COMBA_STORE(C->dp[54]);
   COMBA_STORE2(C->dp[55]);
   C->used = 56;
   C->sign = A->sign ^ B->sign;
   fp_clamp(C);
   COMBA_FINI;
}
#endif

#ifdef TFM_MUL24
void fp_mul_comba24(fp_int *A, fp_int *B, fp_int *C)
{
   fp_digit c0, c1, c2, at[48];

   memcpy(at, A->dp, 24 * sizeof(fp_digit));
   memcpy(at+24, B->dp, 24 * sizeof(fp_digit));
   COMBA_START;

   COMBA_CLEAR;
   /* 0 */
   MULADD(at[0], at[24]); 
   COMBA_STORE(C->dp[0]);
   /* 1 */
   COMBA_FORWARD;
   MULADD(at[0], at[25]);    MULADD(at[1], at[24]); 
   COMBA_STORE(C->dp[1]);
   /* 2 */
   COMBA_FORWARD;
   MULADD(at[0], at[26]);    MULADD(at[1], at[25]);    MULADD(at[2], at[24]); 
   COMBA_STORE(C->dp[2]);
   /* 3 */
   COMBA_FORWARD;
   MULADD(at[0], at[27]);    MULADD(at[1], at[26]);    MULADD(at[2], at[25]);    MULADD(at[3], at[24]); 
   COMBA_STORE(C->dp[3]);
   /* 4 */
   COMBA_FORWARD;
   MULADD(at[0], at[28]);    MULADD(at[1], at[27]);    MULADD(at[2], at[26]);    MULADD(at[3], at[25]);    MULADD(at[4], at[24]); 
   COMBA_STORE(C->dp[4]);
   /* 5 */
   COMBA_FORWARD;
   MULADD(at[0], at[29]);    MULADD(at[1], at[28]);    MULADD(at[2], at[27]);    MULADD(at[3], at[26]);    MULADD(at[4], at[25]);    MULADD(at[5], at[24]); 
   COMBA_STORE(C->dp[5]);
   /* 6 */
   COMBA_FORWARD;
   MULADD(at[0], at[30]);    MULADD(at[1], at[29]);    MULADD(at[2], at[28]);    MULADD(at[3], at[27]);    MULADD(at[4], at[26]);    MULADD(at[5], at[25]);    MULADD(at[6], at[24]); 
   COMBA_STORE(C->dp[6]);
   /* 7 */
   COMBA_FORWARD;
   MULADD(at[0], at[31]);    MULADD(at[1], at[30]);    MULADD(at[2], at[29]);    MULADD(at[3], at[28]);    MULADD(at[4], at[27]);    MULADD(at[5], at[26]);    MULADD(at[6], at[25]);    MULADD(at[7], at[24]); 
   COMBA_STORE(C->dp[7]);
   /* 8 */
   COMBA_FORWARD;
   MULADD(at[0], at[32]);    MULADD(at[1], at[31]);    MULADD(at[2], at[30]);    MULADD(at[3], at[29]);    MULADD(at[4], at[28]);    MULADD(at[5], at[27]);    MULADD(at[6], at[26]);    MULADD(at[7], at[25]);    MULADD(at[8], at[24]); 
   COMBA_STORE(C->dp[8]);
   /* 9 */
   COMBA_FORWARD;
   MULADD(at[0], at[33]);    MULADD(at[1], at[32]);    MULADD(at[2], at[31]);    MULADD(at[3], at[30]);    MULADD(at[4], at[29]);    MULADD(at[5], at[28]);    MULADD(at[6], at[27]);    MULADD(at[7], at[26]);    MULADD(at[8], at[25]);    MULADD(at[9], at[24]); 
   COMBA_STORE(C->dp[9]);
   /* 10 */
   COMBA_FORWARD;
   MULADD(at[0], at[34]);    MULADD(at[1], at[33]);    MULADD(at[2], at[32]);    MULADD(at[3], at[31]);    MULADD(at[4], at[30]);    MULADD(at[5], at[29]);    MULADD(at[6], at[28]);    MULADD(at[7], at[27]);    MULADD(at[8], at[26]);    MULADD(at[9], at[25]);    MULADD(at[10], at[24]); 
   COMBA_STORE(C->dp[10]);
   /* 11 */
   COMBA_FORWARD;
   MULADD(at[0], at[35]);    MULADD(at[1], at[34]);    MULADD(at[2], at[33]);    MULADD(at[3], at[32]);    MULADD(at[4], at[31]);    MULADD(at[5], at[30]);    MULADD(at[6], at[29]);    MULADD(at[7], at[28]);    MULADD(at[8], at[27]);    MULADD(at[9], at[26]);    MULADD(at[10], at[25]);    MULADD(at[11], at[24]); 
   COMBA_STORE(C->dp[11]);
   /* 12 */
   COMBA_FORWARD;
   MULADD(at[0], at[36]);    MULADD(at[1], at[35]);    MULADD(at[2], at[34]);    MULADD(at[3], at[33]);    MULADD(at[4], at[32]);    MULADD(at[5], at[31]);    MULADD(at[6], at[30]);    MULADD(at[7], at[29]);    MULADD(at[8], at[28]);    MULADD(at[9], at[27]);    MULADD(at[10], at[26]);    MULADD(at[11], at[25]);    MULADD(at[12], at[24]); 
   COMBA_STORE(C->dp[12]);
   /* 13 */
   COMBA_FORWARD;
   MULADD(at[0], at[37]);    MULADD(at[1], at[36]);    MULADD(at[2], at[35]);    MULADD(at[3], at[34]);    MULADD(at[4], at[33]);    MULADD(at[5], at[32]);    MULADD(at[6], at[31]);    MULADD(at[7], at[30]);    MULADD(at[8], at[29]);    MULADD(at[9], at[28]);    MULADD(at[10], at[27]);    MULADD(at[11], at[26]);    MULADD(at[12], at[25]);    MULADD(at[13], at[24]); 
   COMBA_STORE(C->dp[13]);
   /* 14 */
   COMBA_FORWARD;
   MULADD(at[0], at[38]);    MULADD(at[1], at[37]);    MULADD(at[2], at[36]);    MULADD(at[3], at[35]);    MULADD(at[4], at[34]);    MULADD(at[5], at[33]);    MULADD(at[6], at[32]);    MULADD(at[7], at[31]);    MULADD(at[8], at[30]);    MULADD(at[9], at[29]);    MULADD(at[10], at[28]);    MULADD(at[11], at[27]);    MULADD(at[12], at[26]);    MULADD(at[13], at[25]);    MULADD(at[14], at[24]); 
   COMBA_STORE(C->dp[14]);
   /* 15 */
   COMBA_FORWARD;
   MULADD(at[0], at[39]);    MULADD(at[1], at[38]);    MULADD(at[2], at[37]);    MULADD(at[3], at[36]);    MULADD(at[4], at[35]);    MULADD(at[5], at[34]);    MULADD(at[6], at[33]);    MULADD(at[7], at[32]);    MULADD(at[8], at[31]);    MULADD(at[9], at[30]);    MULADD(at[10], at[29]);    MULADD(at[11], at[28]);    MULADD(at[12], at[27]);    MULADD(at[13], at[26]);    MULADD(at[14], at[25]);    MULADD(at[15], at[24]); 
   COMBA_STORE(C->dp[15]);
   /* 16 */
   COMBA_FORWARD;
   MULADD(at[0], at[40]);    MULADD(at[1], at[39]);    MULADD(at[2], at[38]);    MULADD(at[3], at[37]);    MULADD(at[4], at[36]);    MULADD(at[5], at[35]);    MULADD(at[6], at[34]);    MULADD(at[7], at[33]);    MULADD(at[8], at[32]);    MULADD(at[9], at[31]);    MULADD(at[10], at[30]);    MULADD(at[11], at[29]);    MULADD(at[12], at[28]);    MULADD(at[13], at[27]);    MULADD(at[14], at[26]);    MULADD(at[15], at[25]);    MULADD(at[16], at[24]); 
   COMBA_STORE(C->dp[16]);
   /* 17 */
   COMBA_FORWARD;
   MULADD(at[0], at[41]);    MULADD(at[1], at[40]);    MULADD(at[2], at[39]);    MULADD(at[3], at[38]);    MULADD(at[4], at[37]);    MULADD(at[5], at[36]);    MULADD(at[6], at[35]);    MULADD(at[7], at[34]);    MULADD(at[8], at[33]);    MULADD(at[9], at[32]);    MULADD(at[10], at[31]);    MULADD(at[11], at[30]);    MULADD(at[12], at[29]);    MULADD(at[13], at[28]);    MULADD(at[14], at[27]);    MULADD(at[15], at[26]);    MULADD(at[16], at[25]);    MULADD(at[17], at[24]); 
   COMBA_STORE(C->dp[17]);
   /* 18 */
   COMBA_FORWARD;
   MULADD(at[0], at[42]);    MULADD(at[1], at[41]);    MULADD(at[2], at[40]);    MULADD(at[3], at[39]);    MULADD(at[4], at[38]);    MULADD(at[5], at[37]);    MULADD(at[6], at[36]);    MULADD(at[7], at[35]);    MULADD(at[8], at[34]);    MULADD(at[9], at[33]);    MULADD(at[10], at[32]);    MULADD(at[11], at[31]);    MULADD(at[12], at[30]);    MULADD(at[13], at[29]);    MULADD(at[14], at[28]);    MULADD(at[15], at[27]);    MULADD(at[16], at[26]);    MULADD(at[17], at[25]);    MULADD(at[18], at[24]); 
   COMBA_STORE(C->dp[18]);
   /* 19 */
   COMBA_FORWARD;
   MULADD(at[0], at[43]);    MULADD(at[1], at[42]);    MULADD(at[2], at[41]);    MULADD(at[3], at[40]);    MULADD(at[4], at[39]);    MULADD(at[5], at[38]);    MULADD(at[6], at[37]);    MULADD(at[7], at[36]);    MULADD(at[8], at[35]);    MULADD(at[9], at[34]);    MULADD(at[10], at[33]);    MULADD(at[11], at[32]);    MULADD(at[12], at[31]);    MULADD(at[13], at[30]);    MULADD(at[14], at[29]);    MULADD(at[15], at[28]);    MULADD(at[16], at[27]);    MULADD(at[17], at[26]);    MULADD(at[18], at[25]);    MULADD(at[19], at[24]); 
   COMBA_STORE(C->dp[19]);
   /* 20 */
   COMBA_FORWARD;
   MULADD(at[0], at[44]);    MULADD(at[1], at[43]);    MULADD(at[2], at[42]);    MULADD(at[3], at[41]);    MULADD(at[4], at[40]);    MULADD(at[5], at[39]);    MULADD(at[6], at[38]);    MULADD(at[7], at[37]);    MULADD(at[8], at[36]);    MULADD(at[9], at[35]);    MULADD(at[10], at[34]);    MULADD(at[11], at[33]);    MULADD(at[12], at[32]);    MULADD(at[13], at[31]);    MULADD(at[14], at[30]);    MULADD(at[15], at[29]);    MULADD(at[16], at[28]);    MULADD(at[17], at[27]);    MULADD(at[18], at[26]);    MULADD(at[19], at[25]);    MULADD(at[20], at[24]); 
   COMBA_STORE(C->dp[20]);
   /* 21 */
   COMBA_FORWARD;
   MULADD(at[0], at[45]);    MULADD(at[1], at[44]);    MULADD(at[2], at[43]);    MULADD(at[3], at[42]);    MULADD(at[4], at[41]);    MULADD(at[5], at[40]);    MULADD(at[6], at[39]);    MULADD(at[7], at[38]);    MULADD(at[8], at[37]);    MULADD(at[9], at[36]);    MULADD(at[10], at[35]);    MULADD(at[11], at[34]);    MULADD(at[12], at[33]);    MULADD(at[13], at[32]);    MULADD(at[14], at[31]);    MULADD(at[15], at[30]);    MULADD(at[16], at[29]);    MULADD(at[17], at[28]);    MULADD(at[18], at[27]);    MULADD(at[19], at[26]);    MULADD(at[20], at[25]);    MULADD(at[21], at[24]); 
   COMBA_STORE(C->dp[21]);
   /* 22 */
   COMBA_FORWARD;
   MULADD(at[0], at[46]);    MULADD(at[1], at[45]);    MULADD(at[2], at[44]);    MULADD(at[3], at[43]);    MULADD(at[4], at[42]);    MULADD(at[5], at[41]);    MULADD(at[6], at[40]);    MULADD(at[7], at[39]);    MULADD(at[8], at[38]);    MULADD(at[9], at[37]);    MULADD(at[10], at[36]);    MULADD(at[11], at[35]);    MULADD(at[12], at[34]);    MULADD(at[13], at[33]);    MULADD(at[14], at[32]);    MULADD(at[15], at[31]);    MULADD(at[16], at[30]);    MULADD(at[17], at[29]);    MULADD(at[18], at[28]);    MULADD(at[19], at[27]);    MULADD(at[20], at[26]);    MULADD(at[21], at[25]);    MULADD(at[22], at[24]); 
   COMBA_STORE(C->dp[22]);
   /* 23 */
   COMBA_FORWARD;
   MULADD(at[0], at[47]);    MULADD(at[1], at[46]);    MULADD(at[2], at[45]);    MULADD(at[3], at[44]);    MULADD(at[4], at[43]);    MULADD(at[5], at[42]);    MULADD(at[6], at[41]);    MULADD(at[7], at[40]);    MULADD(at[8], at[39]);    MULADD(at[9], at[38]);    MULADD(at[10], at[37]);    MULADD(at[11], at[36]);    MULADD(at[12], at[35]);    MULADD(at[13], at[34]);    MULADD(at[14], at[33]);    MULADD(at[15], at[32]);    MULADD(at[16], at[31]);    MULADD(at[17], at[30]);    MULADD(at[18], at[29]);    MULADD(at[19], at[28]);    MULADD(at[20], at[27]);    MULADD(at[21], at[26]);    MULADD(at[22], at[25]);    MULADD(at[23], at[24]); 
   COMBA_STORE(C->dp[23]);
   /* 24 */
   COMBA_FORWARD;
   MULADD(at[1], at[47]);    MULADD(at[2], at[46]);    MULADD(at[3], at[45]);    MULADD(at[4], at[44]);    MULADD(at[5], at[43]);    MULADD(at[6], at[42]);    MULADD(at[7], at[41]);    MULADD(at[8], at[40]);    MULADD(at[9], at[39]);    MULADD(at[10], at[38]);    MULADD(at[11], at[37]);    MULADD(at[12], at[36]);    MULADD(at[13], at[35]);    MULADD(at[14], at[34]);    MULADD(at[15], at[33]);    MULADD(at[16], at[32]);    MULADD(at[17], at[31]);    MULADD(at[18], at[30]);    MULADD(at[19], at[29]);    MULADD(at[20], at[28]);    MULADD(at[21], at[27]);    MULADD(at[22], at[26]);    MULADD(at[23], at[25]); 
   COMBA_STORE(C->dp[24]);
   /* 25 */
   COMBA_FORWARD;
   MULADD(at[2], at[47]);    MULADD(at[3], at[46]);    MULADD(at[4], at[45]);    MULADD(at[5], at[44]);    MULADD(at[6], at[43]);    MULADD(at[7], at[42]);    MULADD(at[8], at[41]);    MULADD(at[9], at[40]);    MULADD(at[10], at[39]);    MULADD(at[11], at[38]);    MULADD(at[12], at[37]);    MULADD(at[13], at[36]);    MULADD(at[14], at[35]);    MULADD(at[15], at[34]);    MULADD(at[16], at[33]);    MULADD(at[17], at[32]);    MULADD(at[18], at[31]);    MULADD(at[19], at[30]);    MULADD(at[20], at[29]);    MULADD(at[21], at[28]);    MULADD(at[22], at[27]);    MULADD(at[23], at[26]); 
   COMBA_STORE(C->dp[25]);
   /* 26 */
   COMBA_FORWARD;
   MULADD(at[3], at[47]);    MULADD(at[4], at[46]);    MULADD(at[5], at[45]);    MULADD(at[6], at[44]);    MULADD(at[7], at[43]);    MULADD(at[8], at[42]);    MULADD(at[9], at[41]);    MULADD(at[10], at[40]);    MULADD(at[11], at[39]);    MULADD(at[12], at[38]);    MULADD(at[13], at[37]);    MULADD(at[14], at[36]);    MULADD(at[15], at[35]);    MULADD(at[16], at[34]);    MULADD(at[17], at[33]);    MULADD(at[18], at[32]);    MULADD(at[19], at[31]);    MULADD(at[20], at[30]);    MULADD(at[21], at[29]);    MULADD(at[22], at[28]);    MULADD(at[23], at[27]); 
   COMBA_STORE(C->dp[26]);
   /* 27 */
   COMBA_FORWARD;
   MULADD(at[4], at[47]);    MULADD(at[5], at[46]);    MULADD(at[6], at[45]);    MULADD(at[7], at[44]);    MULADD(at[8], at[43]);    MULADD(at[9], at[42]);    MULADD(at[10], at[41]);    MULADD(at[11], at[40]);    MULADD(at[12], at[39]);    MULADD(at[13], at[38]);    MULADD(at[14], at[37]);    MULADD(at[15], at[36]);    MULADD(at[16], at[35]);    MULADD(at[17], at[34]);    MULADD(at[18], at[33]);    MULADD(at[19], at[32]);    MULADD(at[20], at[31]);    MULADD(at[21], at[30]);    MULADD(at[22], at[29]);    MULADD(at[23], at[28]); 
   COMBA_STORE(C->dp[27]);
   /* 28 */
   COMBA_FORWARD;
   MULADD(at[5], at[47]);    MULADD(at[6], at[46]);    MULADD(at[7], at[45]);    MULADD(at[8], at[44]);    MULADD(at[9], at[43]);    MULADD(at[10], at[42]);    MULADD(at[11], at[41]);    MULADD(at[12], at[40]);    MULADD(at[13], at[39]);    MULADD(at[14], at[38]);    MULADD(at[15], at[37]);    MULADD(at[16], at[36]);    MULADD(at[17], at[35]);    MULADD(at[18], at[34]);    MULADD(at[19], at[33]);    MULADD(at[20], at[32]);    MULADD(at[21], at[31]);    MULADD(at[22], at[30]);    MULADD(at[23], at[29]); 
   COMBA_STORE(C->dp[28]);
   /* 29 */
   COMBA_FORWARD;
   MULADD(at[6], at[47]);    MULADD(at[7], at[46]);    MULADD(at[8], at[45]);    MULADD(at[9], at[44]);    MULADD(at[10], at[43]);    MULADD(at[11], at[42]);    MULADD(at[12], at[41]);    MULADD(at[13], at[40]);    MULADD(at[14], at[39]);    MULADD(at[15], at[38]);    MULADD(at[16], at[37]);    MULADD(at[17], at[36]);    MULADD(at[18], at[35]);    MULADD(at[19], at[34]);    MULADD(at[20], at[33]);    MULADD(at[21], at[32]);    MULADD(at[22], at[31]);    MULADD(at[23], at[30]); 
   COMBA_STORE(C->dp[29]);
   /* 30 */
   COMBA_FORWARD;
   MULADD(at[7], at[47]);    MULADD(at[8], at[46]);    MULADD(at[9], at[45]);    MULADD(at[10], at[44]);    MULADD(at[11], at[43]);    MULADD(at[12], at[42]);    MULADD(at[13], at[41]);    MULADD(at[14], at[40]);    MULADD(at[15], at[39]);    MULADD(at[16], at[38]);    MULADD(at[17], at[37]);    MULADD(at[18], at[36]);    MULADD(at[19], at[35]);    MULADD(at[20], at[34]);    MULADD(at[21], at[33]);    MULADD(at[22], at[32]);    MULADD(at[23], at[31]); 
   COMBA_STORE(C->dp[30]);
   /* 31 */
   COMBA_FORWARD;
   MULADD(at[8], at[47]);    MULADD(at[9], at[46]);    MULADD(at[10], at[45]);    MULADD(at[11], at[44]);    MULADD(at[12], at[43]);    MULADD(at[13], at[42]);    MULADD(at[14], at[41]);    MULADD(at[15], at[40]);    MULADD(at[16], at[39]);    MULADD(at[17], at[38]);    MULADD(at[18], at[37]);    MULADD(at[19], at[36]);    MULADD(at[20], at[35]);    MULADD(at[21], at[34]);    MULADD(at[22], at[33]);    MULADD(at[23], at[32]); 
   COMBA_STORE(C->dp[31]);
   /* 32 */
   COMBA_FORWARD;
   MULADD(at[9], at[47]);    MULADD(at[10], at[46]);    MULADD(at[11], at[45]);    MULADD(at[12], at[44]);    MULADD(at[13], at[43]);    MULADD(at[14], at[42]);    MULADD(at[15], at[41]);    MULADD(at[16], at[40]);    MULADD(at[17], at[39]);    MULADD(at[18], at[38]);    MULADD(at[19], at[37]);    MULADD(at[20], at[36]);    MULADD(at[21], at[35]);    MULADD(at[22], at[34]);    MULADD(at[23], at[33]); 
   COMBA_STORE(C->dp[32]);
   /* 33 */
   COMBA_FORWARD;
   MULADD(at[10], at[47]);    MULADD(at[11], at[46]);    MULADD(at[12], at[45]);    MULADD(at[13], at[44]);    MULADD(at[14], at[43]);    MULADD(at[15], at[42]);    MULADD(at[16], at[41]);    MULADD(at[17], at[40]);    MULADD(at[18], at[39]);    MULADD(at[19], at[38]);    MULADD(at[20], at[37]);    MULADD(at[21], at[36]);    MULADD(at[22], at[35]);    MULADD(at[23], at[34]); 
   COMBA_STORE(C->dp[33]);
   /* 34 */
   COMBA_FORWARD;
   MULADD(at[11], at[47]);    MULADD(at[12], at[46]);    MULADD(at[13], at[45]);    MULADD(at[14], at[44]);    MULADD(at[15], at[43]);    MULADD(at[16], at[42]);    MULADD(at[17], at[41]);    MULADD(at[18], at[40]);    MULADD(at[19], at[39]);    MULADD(at[20], at[38]);    MULADD(at[21], at[37]);    MULADD(at[22], at[36]);    MULADD(at[23], at[35]); 
   COMBA_STORE(C->dp[34]);
   /* 35 */
   COMBA_FORWARD;
   MULADD(at[12], at[47]);    MULADD(at[13], at[46]);    MULADD(at[14], at[45]);    MULADD(at[15], at[44]);    MULADD(at[16], at[43]);    MULADD(at[17], at[42]);    MULADD(at[18], at[41]);    MULADD(at[19], at[40]);    MULADD(at[20], at[39]);    MULADD(at[21], at[38]);    MULADD(at[22], at[37]);    MULADD(at[23], at[36]); 
   COMBA_STORE(C->dp[35]);
   /* 36 */
   COMBA_FORWARD;
   MULADD(at[13], at[47]);    MULADD(at[14], at[46]);    MULADD(at[15], at[45]);    MULADD(at[16], at[44]);    MULADD(at[17], at[43]);    MULADD(at[18], at[42]);    MULADD(at[19], at[41]);    MULADD(at[20], at[40]);    MULADD(at[21], at[39]);    MULADD(at[22], at[38]);    MULADD(at[23], at[37]); 
   COMBA_STORE(C->dp[36]);
   /* 37 */
   COMBA_FORWARD;
   MULADD(at[14], at[47]);    MULADD(at[15], at[46]);    MULADD(at[16], at[45]);    MULADD(at[17], at[44]);    MULADD(at[18], at[43]);    MULADD(at[19], at[42]);    MULADD(at[20], at[41]);    MULADD(at[21], at[40]);    MULADD(at[22], at[39]);    MULADD(at[23], at[38]); 
   COMBA_STORE(C->dp[37]);
   /* 38 */
   COMBA_FORWARD;
   MULADD(at[15], at[47]);    MULADD(at[16], at[46]);    MULADD(at[17], at[45]);    MULADD(at[18], at[44]);    MULADD(at[19], at[43]);    MULADD(at[20], at[42]);    MULADD(at[21], at[41]);    MULADD(at[22], at[40]);    MULADD(at[23], at[39]); 
   COMBA_STORE(C->dp[38]);
   /* 39 */
   COMBA_FORWARD;
   MULADD(at[16], at[47]);    MULADD(at[17], at[46]);    MULADD(at[18], at[45]);    MULADD(at[19], at[44]);    MULADD(at[20], at[43]);    MULADD(at[21], at[42]);    MULADD(at[22], at[41]);    MULADD(at[23], at[40]); 
   COMBA_STORE(C->dp[39]);
   /* 40 */
   COMBA_FORWARD;
   MULADD(at[17], at[47]);    MULADD(at[18], at[46]);    MULADD(at[19], at[45]);    MULADD(at[20], at[44]);    MULADD(at[21], at[43]);    MULADD(at[22], at[42]);    MULADD(at[23], at[41]); 
   COMBA_STORE(C->dp[40]);
   /* 41 */
   COMBA_FORWARD;
   MULADD(at[18], at[47]);    MULADD(at[19], at[46]);    MULADD(at[20], at[45]);    MULADD(at[21], at[44]);    MULADD(at[22], at[43]);    MULADD(at[23], at[42]); 
   COMBA_STORE(C->dp[41]);
   /* 42 */
   COMBA_FORWARD;
   MULADD(at[19], at[47]);    MULADD(at[20], at[46]);    MULADD(at[21], at[45]);    MULADD(at[22], at[44]);    MULADD(at[23], at[43]); 
   COMBA_STORE(C->dp[42]);
   /* 43 */
   COMBA_FORWARD;
   MULADD(at[20], at[47]);    MULADD(at[21], at[46]);    MULADD(at[22], at[45]);    MULADD(at[23], at[44]); 
   COMBA_STORE(C->dp[43]);
   /* 44 */
   COMBA_FORWARD;
   MULADD(at[21], at[47]);    MULADD(at[22], at[46]);    MULADD(at[23], at[45]); 
   COMBA_STORE(C->dp[44]);
   /* 45 */
   COMBA_FORWARD;
   MULADD(at[22], at[47]);    MULADD(at[23], at[46]); 
   COMBA_STORE(C->dp[45]);
   /* 46 */
   COMBA_FORWARD;
   MULADD(at[23], at[47]); 
   COMBA_STORE(C->dp[46]);
   COMBA_STORE2(C->dp[47]);
   C->used = 48;
   C->sign = A->sign ^ B->sign;
   fp_clamp(C);
   COMBA_FINI;
}
#endif
/* $Source$ */
/* $Revision$ */
/* $Date$ */

