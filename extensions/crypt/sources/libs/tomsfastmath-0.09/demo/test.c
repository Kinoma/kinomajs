/* TFM demo program */
#include <tfm.h>

void draw(fp_int *a)
{
  int x;
  printf("%d, %d, ", a->used, a->sign);
  for (x = a->used - 1; x >= 0; x--) {
      printf("%08lx ", a->dp[x]);
  }
  printf("\n");
}

int myrng(unsigned char *dst, int len, void *dat)
{
   int x;
   for (x = 0; x < len; x++) dst[x] = rand() & 0xFF;
   return len;
}

/* RDTSC from Scott Duplichan */
static ulong64 TIMFUNC (void)
   {
   #if defined __GNUC__
      #if defined(INTEL_CC)
	 ulong64 a;
         asm ("rdtsc":"=A"(a));
         return a;
      #elif defined(__i386__) || defined(__x86_64__)
         ulong64 a;
         __asm__ __volatile__ ("rdtsc\nmovl %%eax,%0\nmovl %%edx,4+%0\n"::"m"(a):"%eax","%edx");
         return a;
      #elif defined(TFM_PPC32) 
         unsigned long a, b;
         __asm__ __volatile__ ("mftbu %1 \nmftb %0\n":"=r"(a), "=r"(b));
         return (((ulong64)b) << 32ULL) | ((ulong64)a);
      #else /* gcc-IA64 version */
         unsigned long result;
         __asm__ __volatile__("mov %0=ar.itc" : "=r"(result) :: "memory");
         while (__builtin_expect ((int) result == -1, 0))
         __asm__ __volatile__("mov %0=ar.itc" : "=r"(result) :: "memory");
         return result;
      #endif

   // Microsoft and Intel Windows compilers
   #elif defined _M_IX86
     __asm rdtsc
   #elif defined _M_AMD64
     return __rdtsc ();
   #elif defined _M_IA64
     #if defined __INTEL_COMPILER
       #include <ia64intrin.h>
     #endif
      return __getReg (3116);
   #else
     #error need rdtsc function for this build
   #endif
   }

   char cmd[4096], buf[4096];

int main(void)
{
  fp_int a,b,c,d,e,f;
  fp_digit fp;
  int n, err;
   unsigned long expt_n, add_n, sub_n, mul_n, div_n, sqr_n, mul2d_n, div2d_n, gcd_n, lcm_n, inv_n,
                 div2_n, mul2_n, add_d_n, sub_d_n, mul_d_n, t, cnt, rr, ix;
   ulong64 t1, t2;

  srand(time(NULL));
  printf("TFM Ident string:\n%s\n\n", fp_ident());
  fp_zero(&b); fp_zero(&c); fp_zero(&d); fp_zero(&e); fp_zero(&f); 
  fp_zero(&a); draw(&a);

  /* test set and simple shifts */
  printf("Testing mul/div 2\n");
  fp_set(&a, 1); draw(&a);
  for (n = 0; n <= DIGIT_BIT; n++) {
      fp_mul_2(&a, &a); printf("(%d) ", fp_count_bits(&a));
      draw(&a);
      
  }
  for (n = 0; n <= (DIGIT_BIT + 1); n++) {
      fp_div_2(&a, &a);
      draw(&a);
  }
  fp_set(&a, 1);

  /* test lshd/rshd */
  printf("testing lshd/rshd\n");
  fp_lshd(&a, 3); draw(&a);
  fp_rshd(&a, 3); draw(&a);

  /* test more complicated shifts */
  printf("Testing mul/div 2d\n");
  fp_mul_2d(&a, DIGIT_BIT/2, &a); draw(&a);
  fp_div_2d(&a, DIGIT_BIT/2, &a, NULL); draw(&a);

  fp_mul_2d(&a, DIGIT_BIT + DIGIT_BIT/2, &a); draw(&a);
  fp_div_2d(&a, DIGIT_BIT + DIGIT_BIT/2, &a, NULL); draw(&a);

  /* test neg/abs  */
  printf("testing neg/abs\n");
  fp_neg(&a, &a); draw(&a);
  fp_neg(&a, &a); draw(&a);
  fp_neg(&a, &a); draw(&a);
  fp_abs(&a, &a); draw(&a);

  /* test comparisons */
  fp_set(&b, 3);
  fp_set(&c, 4); fp_neg(&c, &c);
  fp_set(&d, 1);
  printf("Testing compares\n%d, %d, %d, %d\n", fp_cmp(&a, &b), fp_cmp(&a, &c), fp_cmp(&a, &d), fp_cmp(&b, &c));

  /* test add/sub */
  printf("Testing add/sub \n");
  fp_set(&a, ((fp_digit)1)<<(DIGIT_BIT-1)); draw(&a);
  fp_set(&b, ((fp_digit)1)<<(DIGIT_BIT-2));
  fp_add(&a, &b, &a); draw(&a);
  fp_add(&a, &b, &a); draw(&a);
  fp_add(&a, &b, &a); draw(&a);
  printf("sub...\n");
  printf("cmp returns: %d, ", fp_cmp(&a, &b)); fp_sub(&a, &b, &a); draw(&a);
  printf("cmp returns: %d, ", fp_cmp(&a, &b)); fp_sub(&a, &b, &a); draw(&a);
  printf("cmp returns: %d, ", fp_cmp(&a, &b)); fp_sub(&a, &b, &a); draw(&a);
  printf("cmp returns: %d, ", fp_cmp(&a, &b)); fp_sub(&a, &b, &a); draw(&a);
  printf("cmp returns: %d, ", fp_cmp(&a, &b)); fp_sub(&a, &b, &a); draw(&a);
  printf("cmp returns: %d, ", fp_cmp(&a, &b)); fp_sub(&a, &b, &a); draw(&a);

  /* test mul_d */
  printf("Testing mul_d and div_d\n");
  fp_set(&a, 1);
  fp_mul_d(&a, ((fp_digit)1)<<(DIGIT_BIT/2), &a); draw(&a);
  fp_mul_d(&a, ((fp_digit)1)<<(DIGIT_BIT/2), &a); draw(&a);
  fp_mul_d(&a, ((fp_digit)1)<<(DIGIT_BIT/2), &a); draw(&a);
  printf("div_d\n");
  fp_div_d(&a, ((fp_digit)1)<<(DIGIT_BIT/2), &a, NULL); draw(&a);
  fp_div_d(&a, ((fp_digit)1)<<(DIGIT_BIT/2), &a, NULL); draw(&a);
  fp_div_d(&a, ((fp_digit)1)<<(DIGIT_BIT/2), &a, NULL); draw(&a);

  /* testing read radix */
  printf("Testing read_radix\n");
  fp_read_radix(&a, "123456789012345678901234567890", 16); draw(&a);

#if 0
  /* test mont */
  printf("Montgomery test #1\n");
  fp_set(&a, 0x1234567ULL);
  fp_montgomery_setup(&a, &fp);
  fp_montgomery_calc_normalization(&b, &a);

  fp_read_radix(&d, "123456789123", 16);
  for (n = 0; n < 1000000; n++) {
      fp_add_d(&d, 1, &d); fp_sqrmod(&d, &a, &d); 
      fp_mul(&d, &b, &c);
      fp_montgomery_reduce(&c, &a, fp);
      if (fp_cmp(&c, &d) != FP_EQ) {
         printf("Failed mont %d\n", n);
         draw(&a);
         draw(&d);
         draw(&c);
         return EXIT_FAILURE;
      }
  }
  printf("Passed.\n");

  printf("Montgomery test #2\n");
  fp_set(&a, 0x1234567ULL);
  fp_lshd(&a, 4);
  fp_add_d(&a, 1, &a);
  fp_montgomery_setup(&a, &fp);
  fp_montgomery_calc_normalization(&b, &a);

  fp_read_radix(&d, "123456789123", 16);
  for (n = 0; n < 1000000; n++) {
      fp_add_d(&d, 1, &d); fp_sqrmod(&d, &a, &d); 
      fp_mul(&d, &b, &c);
      fp_montgomery_reduce(&c, &a, fp);
      if (fp_cmp(&c, &d) != FP_EQ) {
         printf("Failed mont %d\n", n);
         draw(&a);
         draw(&d);
         draw(&c);
         return EXIT_FAILURE;
      }
  }
  printf("Passed.\n");

   /* test for size */
   for (ix = 8*DIGIT_BIT; ix < 10*DIGIT_BIT; ix++) {
       printf("Testing (not safe-prime): %9lu bits    \r", ix); fflush(stdout);
       err = fp_prime_random_ex(&a, 8, ix, (rand()&1)?TFM_PRIME_2MSB_OFF:TFM_PRIME_2MSB_ON, myrng, NULL);
       if (err != FP_OKAY) {
          printf("failed with err code %d\n", err);
          return EXIT_FAILURE;
       }
       if ((unsigned long)fp_count_bits(&a) != ix) {
          printf("Prime is %d not %lu bits!!!\n", fp_count_bits(&a), ix);
          return EXIT_FAILURE;
       }
   }
   printf("\n\n");
#endif

#ifdef TESTING
goto testing;
#endif

#if 1

t1 = TIMFUNC();
sleep(1);
printf("Ticks per second: %llu\n", TIMFUNC() - t1);

goto monttime;
 /* do some timings... */
  printf("Addition:\n");
  for (t = 2; t <= FP_SIZE/2; t += 2) {
      fp_zero(&a);
      fp_zero(&b);
      fp_zero(&c);
      for (ix = 0; ix < t; ix++) {
          a.dp[ix] = ix;
          b.dp[ix] = ix;
      }
      a.used = t;
      b.used = t;
      t2 = -1;
      for (ix = 0; ix < 25000; ++ix) {
          t1 = TIMFUNC();
          fp_add(&a, &b, &c); fp_add(&a, &b, &c);
          fp_add(&a, &b, &c); fp_add(&a, &b, &c);
          fp_add(&a, &b, &c); fp_add(&a, &b, &c);
          fp_add(&a, &b, &c); fp_add(&a, &b, &c);
          t2 = (TIMFUNC() - t1)>>3;
          if (t1<t2) { --ix; t2 = t1; }
      }
      printf("%5lu-bit: %9llu\n", t * DIGIT_BIT, t2);
  }
multtime:
  printf("Multiplication:\n");
  for (t = 2; t <= FP_SIZE/2; t += 2) {
      fp_zero(&a);
      fp_zero(&b);
      fp_zero(&c);
      for (ix = 0; ix < t; ix++) {
          a.dp[ix] = ix;
          b.dp[ix] = ix;
      }
      a.used = t;
      b.used = t;
      t2 = -1;
      for (ix = 0; ix < 10000; ++ix) {
          t1 = TIMFUNC();
          fp_mul(&a, &b, &c); fp_mul(&a, &b, &c);
          fp_mul(&a, &b, &c); fp_mul(&a, &b, &c);
          t2 = (TIMFUNC() - t1)>>2;
          if (t1<t2) { --ix; t2 = t1; }
      }
      printf("%5lu-bit: %9llu\n", t * DIGIT_BIT, t2);
  }
//#else
sqrtime:
  printf("Squaring:\n");
  for (t = 2; t <= FP_SIZE/2; t += 2) {
      fp_zero(&a);
      fp_zero(&b);
      for (ix = 0; ix < t; ix++) {
          a.dp[ix] = ix;
      }
      a.used = t;
      t2 = -1;
      for (ix = 0; ix < 10000; ++ix) {
          t1 = TIMFUNC();
          fp_sqr(&a, &b); fp_sqr(&a, &b);
          fp_sqr(&a, &b); fp_sqr(&a, &b);
          t2 = (TIMFUNC() - t1)>>2;
          if (t1<t2) { --ix; t2 = t1; }
      }
      printf("%5lu-bit: %9llu\n", t * DIGIT_BIT, t2);
  }
//#else
monttime:
  printf("Montgomery:\n");
  for (t = 2; t <= (FP_SIZE/2)-4; t += 2) {
//      printf("%5lu-bit: %9llu\n", t * DIGIT_BIT, t2);
      fp_zero(&a);
      for (ix = 0; ix < t; ix++) {
          a.dp[ix] = ix | 1;
      }
      a.used = t;

     fp_montgomery_setup(&a, &fp);
     fp_sub_d(&a, 3, &b);
     fp_sqr(&b, &b);      
     fp_copy(&b, &c);      
     fp_copy(&b, &d);      

     t2 = -1;
     for (ix = 0; ix < 10000; ++ix) {
          t1 = TIMFUNC();
          fp_montgomery_reduce(&c, &a, &fp);
          fp_montgomery_reduce(&d, &a, &fp);
          t2 = (TIMFUNC() - t1)>>1;
          fp_copy(&b, &c);      
          fp_copy(&b, &d);      
          if (t1<t2) { --ix; t2 = t1; }
      }
      printf("%5lu-bit: %9llu\n", t * DIGIT_BIT, t2);
  }
//#else
expttime:
  printf("Exptmod:\n");
 
  for (t = 512/DIGIT_BIT; t <= (FP_SIZE/2)-2; t += 256/DIGIT_BIT) {
      fp_zero(&a);
      fp_zero(&b);
      fp_zero(&c);
      for (ix = 0; ix < t; ix++) {
          a.dp[ix] = ix+1;
          b.dp[ix] = (fp_digit)rand() * (fp_digit)rand();
          c.dp[ix] = ix;
      }
      a.used = t;
      b.used = t;
      c.used = t;

     t2 = -1;
     for (ix = 0; ix < 500; ++ix) {
          t1 = TIMFUNC();
          fp_exptmod(&c, &b, &a, &d);
          fp_exptmod(&c, &b, &a, &d);
          t2 = (TIMFUNC() - t1)>>1;
          fp_copy(&b, &c);      
          fp_copy(&b, &d);      
          if (t1<t2) { t2 = t1; --ix; }
     }
     printf("%5lu-bit: %9llu\n", t * DIGIT_BIT, t2);
  }
  return;
#endif

return;
testing:

  fp_zero(&b); fp_zero(&c); fp_zero(&d); fp_zero(&e); fp_zero(&f); fp_zero(&a);


   div2_n = mul2_n = inv_n = expt_n = lcm_n = gcd_n = add_n =
   sub_n = mul_n = div_n = sqr_n = mul2d_n = div2d_n = cnt = add_d_n = sub_d_n= mul_d_n = 0;

   for (;;) {
       printf("%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu/%4lu ", add_n, sub_n, mul_n, div_n, sqr_n, mul2d_n, div2d_n, gcd_n, lcm_n, expt_n, inv_n, div2_n, mul2_n, add_d_n, sub_d_n, mul_d_n);
       fgets(cmd, 4095, stdin);
       cmd[strlen(cmd)-1] = 0;
       printf("%s  ]\r",cmd); fflush(stdout);
       if (!strcmp(cmd, "mul2d")) { ++mul2d_n;
          fgets(buf, 4095, stdin); fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin); sscanf(buf, "%lu", &rr);
          fgets(buf, 4095, stdin); fp_read_radix(&b, buf, 64);

          fp_mul_2d(&a, rr, &a);
          a.sign = b.sign;
          if (fp_cmp(&a, &b) != FP_EQ) {
             printf("mul2d failed, rr == %lu\n",rr);
             draw(&a);
             draw(&b);
             return 0;
          }
       } else if (!strcmp(cmd, "div2d")) { ++div2d_n;
          fgets(buf, 4095, stdin); fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin); sscanf(buf, "%lu", &rr);
          fgets(buf, 4095, stdin); fp_read_radix(&b, buf, 64);

          fp_div_2d(&a, rr, &a, &e);
          a.sign = b.sign;
          if (a.used == b.used && a.used == 0) { a.sign = b.sign = FP_ZPOS; }
          if (fp_cmp(&a, &b) != FP_EQ) {
             printf("div2d failed, rr == %lu\n",rr);
             draw(&a);
             draw(&b);
             return 0;
          }
       } else if (!strcmp(cmd, "add")) { ++add_n;
          fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
          fp_copy(&a, &d);
          fp_add(&d, &b, &d);
          if (fp_cmp(&c, &d) != FP_EQ) {
             printf("add %lu failure!\n", add_n);
draw(&a);draw(&b);draw(&c);draw(&d);
             return 0;
          }

          /* test the sign/unsigned storage functions */

          rr = fp_signed_bin_size(&c);
          fp_to_signed_bin(&c, (unsigned char *)cmd);
          memset(cmd+rr, rand()&255, sizeof(cmd)-rr);
          fp_read_signed_bin(&d, (unsigned char *)cmd, rr);
          if (fp_cmp(&c, &d) != FP_EQ) {
             printf("fp_signed_bin failure!\n");
             draw(&c);
             draw(&d);
             return 0;
          }

          rr = fp_unsigned_bin_size(&c);
          fp_to_unsigned_bin(&c, (unsigned char *)cmd);
          memset(cmd+rr, rand()&255, sizeof(cmd)-rr);
          fp_read_unsigned_bin(&d, (unsigned char *)cmd, rr);
          if (fp_cmp_mag(&c, &d) != FP_EQ) {
             printf("fp_unsigned_bin failure!\n");
             draw(&c);
             draw(&d);
             return 0;
          }

       } else if (!strcmp(cmd, "sub")) { ++sub_n;
          fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
          fp_copy(&a, &d);
          fp_sub(&d, &b, &d);
          if (fp_cmp(&c, &d) != FP_EQ) {
             printf("sub %lu failure!\n", sub_n);
draw(&a);draw(&b);draw(&c);draw(&d);
             return 0;
          }
       } else if (!strcmp(cmd, "mul")) { 
          fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
//continue;
          fp_copy(&a, &d);
          fp_mul(&d, &b, &d); ++mul_n;
          if (fp_cmp(&c, &d) != FP_EQ) {
             printf("mul %lu failure!\n", mul_n);
draw(&a);draw(&b);draw(&c);draw(&d);
             return 0;
          }
       } else if (!strcmp(cmd, "div")) { 
          fgets(buf, 4095, stdin); fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin); fp_read_radix(&b, buf, 64);
          fgets(buf, 4095, stdin); fp_read_radix(&c, buf, 64);
          fgets(buf, 4095, stdin); fp_read_radix(&d, buf, 64);
// continue;
          fp_div(&a, &b, &e, &f); ++div_n;
          if (fp_cmp(&c, &e) != FP_EQ || fp_cmp(&d, &f) != FP_EQ) {
             printf("div %lu failure!\n", div_n);
draw(&a);draw(&b);draw(&c);draw(&d); draw(&e); draw(&f);
             return 0;
          }

       } else if (!strcmp(cmd, "sqr")) { 
          fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
// continue;
          fp_copy(&a, &c);
          fp_sqr(&c, &c); ++sqr_n;
          if (fp_cmp(&b, &c) != FP_EQ) {
             printf("sqr %lu failure!\n", sqr_n);
draw(&a);draw(&b);draw(&c);
             return 0;
          }
       } else if (!strcmp(cmd, "gcd")) { 
          fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
          fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
// continue;
          fp_copy(&a, &d);
          fp_gcd(&d, &b, &d); ++gcd_n;
          d.sign = c.sign;
          if (fp_cmp(&c, &d) != FP_EQ) {
             printf("gcd %lu failure!\n", gcd_n);
draw(&a);draw(&b);draw(&c);draw(&d);
             return 0;
          }
       } else if (!strcmp(cmd, "lcm")) { 
             fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
//continue;
             fp_copy(&a, &d);
             fp_lcm(&d, &b, &d); ++lcm_n;
             d.sign = c.sign;
             if (fp_cmp(&c, &d) != FP_EQ) {
                printf("lcm %lu failure!\n", lcm_n);
   draw(&a);draw(&b);draw(&c);draw(&d);
                return 0;
             }
       } else if (!strcmp(cmd, "expt")) {  
             fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&d, buf, 64);
// continue;
             fp_copy(&a, &e);
             fp_exptmod(&e, &b, &c, &e); ++expt_n;
             if (fp_cmp(&d, &e) != FP_EQ) {
                printf("expt %lu failure!\n", expt_n);
   draw(&a);draw(&b);draw(&c);draw(&d); draw(&e);
                return 0;
             }
       } else if (!strcmp(cmd, "invmod")) {  
             fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&c, buf, 64);
//continue;
             fp_invmod(&a, &b, &d);
#if 1
             fp_mulmod(&d,&a,&b,&e); ++inv_n;
             if (fp_cmp_d(&e, 1) != FP_EQ) {
#else
             if (fp_cmp(&d, &c) != FP_EQ) {
#endif
                printf("inv [wrong value from MPI?!] failure\n");
                draw(&a);draw(&b);draw(&c);draw(&d);
                return 0;
             }

       } else if (!strcmp(cmd, "div2")) { ++div2_n;
             fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
             fp_div_2(&a, &c);
             if (fp_cmp(&c, &b) != FP_EQ) {
                 printf("div_2 %lu failure\n", div2_n);
                 draw(&a);
                 draw(&b);
                 draw(&c);
                 return 0;
             }
       } else if (!strcmp(cmd, "mul2")) { ++mul2_n;
             fgets(buf, 4095, stdin);  fp_read_radix(&a, buf, 64);
             fgets(buf, 4095, stdin);  fp_read_radix(&b, buf, 64);
             fp_mul_2(&a, &c);
             if (fp_cmp(&c, &b) != FP_EQ) {
                 printf("mul_2 %lu failure\n", mul2_n);
                 draw(&a);
                 draw(&b);
                 draw(&c);
                 return 0;
             }
       } else if (!strcmp(cmd, "add_d")) { ++add_d_n;
              fgets(buf, 4095, stdin); fp_read_radix(&a, buf, 64);
              fgets(buf, 4095, stdin); sscanf(buf, "%lu", &ix);
              fgets(buf, 4095, stdin); fp_read_radix(&b, buf, 64);
              fp_add_d(&a, ix, &c);
              if (fp_cmp(&b, &c) != FP_EQ) {
                 printf("add_d %lu failure\n", add_d_n);
                 draw(&a);
                 draw(&b);
                 draw(&c);
                 printf("d == %lu\n", ix);
                 return 0;
              }
       } else if (!strcmp(cmd, "sub_d")) { ++sub_d_n;
              fgets(buf, 4095, stdin); fp_read_radix(&a, buf, 64);
              fgets(buf, 4095, stdin); sscanf(buf, "%lu", &ix);
              fgets(buf, 4095, stdin); fp_read_radix(&b, buf, 64);
              fp_sub_d(&a, ix, &c);
              if (fp_cmp(&b, &c) != FP_EQ) {
                 printf("sub_d %lu failure\n", sub_d_n);
                 draw(&a);
                 draw(&b);
                 draw(&c);
                 printf("d == %lu\n", ix);
                 return 0;
              }
       } else if (!strcmp(cmd, "mul_d")) { ++mul_d_n;
              fgets(buf, 4095, stdin); fp_read_radix(&a, buf, 64);
              fgets(buf, 4095, stdin); sscanf(buf, "%lu", &ix);
              fgets(buf, 4095, stdin); fp_read_radix(&b, buf, 64);
              fp_mul_d(&a, ix, &c);
              if (fp_cmp(&b, &c) != FP_EQ) {
                 printf("mul_d %lu failure\n", sub_d_n);
                 draw(&a);
                 draw(&b);
                 draw(&c);
                 printf("d == %lu\n", ix);
                 return 0;
              }
       }

   }
}
  
  

/* $Source$ */
/* $Revision$ */
/* $Date$ */
