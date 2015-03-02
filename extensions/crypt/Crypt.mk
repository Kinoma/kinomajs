<?xml version="1.0" encoding="UTF-8"?>
<!--
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
-->
<makefile>
<include name="/makefiles/xsFskDefaults.mk"/>

<!-- use LTC -->
<wrap name="primitives_ltc.c"/>
<wrap name="arith_ltc.c"/>
<wrap name="fips180.c"/>	<!-- for SHA1 in x509.c -->

<input name="sources/libs/libtomcrypt-1.13/src/headers"/>
<input name="sources/libs/libtomcrypt-1.13/src/ciphers"/>
<wrap name="des.c"/>
<input name="sources/libs/libtomcrypt-1.13/src/ciphers/aes"/>
<wrap name="aes.c"/>
<input name="sources/libs/libtomcrypt-1.13/src/hashes"/>
<!--wrap name="md5_ltc.c"/-->	<!-- md5 is in the core -->
<wrap name="sha1.c"/>
<input name="sources/libs/libtomcrypt-1.13/src/hashes/sha2"/>
<wrap name="sha256.c"/>
<wrap name="sha512.c"/>
<input name="sources/libs/libtomcrypt-1.13/src/prngs"/>
<wrap name="rc4.c"/>
<input name="sources/libs/libtomcrypt-1.13/src/math"/>
<wrap name="ltm_desc.c"/>
<input name="sources/libs/libtommath-0.39"/>
<wrap name="bncore.c"/>
<wrap name="bn_mp_init.c"/>
<wrap name="bn_mp_clear.c"/>
<wrap name="bn_mp_exch.c"/>
<wrap name="bn_mp_grow.c"/>
<wrap name="bn_mp_shrink.c"/>
<wrap name="bn_mp_clamp.c"/>
<wrap name="bn_mp_zero.c"/>
<wrap name="bn_mp_set.c"/>
<wrap name="bn_mp_set_int.c"/>
<wrap name="bn_mp_init_size.c"/>
<wrap name="bn_mp_copy.c"/>
<wrap name="bn_mp_init_copy.c"/>
<wrap name="bn_mp_abs.c"/>
<wrap name="bn_mp_neg.c"/>
<wrap name="bn_mp_cmp_mag.c"/>
<wrap name="bn_mp_cmp.c"/>
<wrap name="bn_mp_cmp_d.c"/>
<wrap name="bn_mp_rshd.c"/>
<wrap name="bn_mp_lshd.c"/>
<wrap name="bn_mp_mod_2d.c"/>
<wrap name="bn_mp_div_2d.c"/>
<wrap name="bn_mp_mul_2d.c"/>
<wrap name="bn_mp_div_2.c"/>
<wrap name="bn_mp_mul_2.c"/>
<wrap name="bn_s_mp_add.c"/>
<wrap name="bn_s_mp_sub.c"/>
<wrap name="bn_fast_s_mp_mul_digs.c"/>
<wrap name="bn_s_mp_mul_digs.c"/>
<wrap name="bn_fast_s_mp_mul_high_digs.c"/>
<wrap name="bn_s_mp_mul_high_digs.c"/>
<wrap name="bn_fast_s_mp_sqr.c"/>
<wrap name="bn_s_mp_sqr.c"/>
<wrap name="bn_mp_add.c"/>
<wrap name="bn_mp_sub.c"/>
<wrap name="bn_mp_karatsuba_mul.c"/>
<wrap name="bn_mp_mul.c"/>
<wrap name="bn_mp_karatsuba_sqr.c"/>
<wrap name="bn_mp_sqr.c"/>
<wrap name="bn_mp_div.c"/>
<wrap name="bn_mp_mod.c"/>
<wrap name="bn_mp_add_d.c"/>
<wrap name="bn_mp_sub_d.c"/>
<wrap name="bn_mp_mul_d.c"/>
<wrap name="bn_mp_div_d.c"/>
<wrap name="bn_mp_mod_d.c"/>
<wrap name="bn_mp_expt_d.c"/>
<wrap name="bn_mp_addmod.c"/>
<wrap name="bn_mp_submod.c"/>
<wrap name="bn_mp_mulmod.c"/>
<wrap name="bn_mp_sqrmod.c"/>
<wrap name="bn_mp_gcd.c"/>
<wrap name="bn_mp_lcm.c"/>
<wrap name="bn_fast_mp_invmod.c"/>
<wrap name="bn_mp_invmod.c"/>
<wrap name="bn_mp_reduce.c"/>
<wrap name="bn_mp_montgomery_setup.c"/>
<wrap name="bn_fast_mp_montgomery_reduce.c"/>
<wrap name="bn_mp_montgomery_reduce.c"/>
<wrap name="bn_mp_exptmod_fast.c"/>
<wrap name="bn_mp_exptmod.c"/>
<wrap name="bn_mp_2expt.c"/>
<wrap name="bn_mp_n_root.c"/>
<wrap name="bn_mp_jacobi.c"/>
<wrap name="bn_reverse.c"/>
<wrap name="bn_mp_count_bits.c"/>
<wrap name="bn_mp_read_unsigned_bin.c"/>
<wrap name="bn_mp_read_signed_bin.c"/>
<wrap name="bn_mp_to_unsigned_bin.c"/>
<wrap name="bn_mp_to_signed_bin.c"/>
<wrap name="bn_mp_unsigned_bin_size.c"/>
<wrap name="bn_mp_signed_bin_size.c"/>
<wrap name="bn_mp_xor.c"/>
<wrap name="bn_mp_and.c"/>
<wrap name="bn_mp_or.c"/>
<wrap name="bn_mp_rand.c"/>
<wrap name="bn_mp_montgomery_calc_normalization.c"/>
<wrap name="bn_mp_prime_is_divisible.c"/>
<wrap name="bn_prime_tab.c"/>
<wrap name="bn_mp_prime_fermat.c"/>
<wrap name="bn_mp_prime_miller_rabin.c"/>
<wrap name="bn_mp_prime_is_prime.c"/>
<wrap name="bn_mp_prime_next_prime.c"/>
<wrap name="bn_mp_dr_reduce.c"/>
<wrap name="bn_mp_dr_is_modulus.c"/>
<wrap name="bn_mp_dr_setup.c"/>
<wrap name="bn_mp_reduce_setup.c"/>
<wrap name="bn_mp_toom_mul.c"/>
<wrap name="bn_mp_toom_sqr.c"/>
<wrap name="bn_mp_div_3.c"/>
<wrap name="bn_s_mp_exptmod.c"/>
<wrap name="bn_mp_reduce_2k.c"/>
<wrap name="bn_mp_reduce_is_2k.c"/>
<wrap name="bn_mp_reduce_2k_setup.c"/>
<wrap name="bn_mp_reduce_2k_l.c"/>
<wrap name="bn_mp_reduce_is_2k_l.c"/>
<wrap name="bn_mp_reduce_2k_setup_l.c"/>
<wrap name="bn_mp_radix_smap.c"/>
<wrap name="bn_mp_read_radix.c"/>
<wrap name="bn_mp_toradix.c"/>
<wrap name="bn_mp_radix_size.c"/>
<wrap name="bn_mp_fread.c"/>
<wrap name="bn_mp_fwrite.c"/>
<wrap name="bn_mp_cnt_lsb.c"/>
<wrap name="bn_error.c"/>
<wrap name="bn_mp_init_multi.c"/>
<wrap name="bn_mp_clear_multi.c"/>
<wrap name="bn_mp_exteuclid.c"/>
<wrap name="bn_mp_toradix_n.c"/>
<wrap name="bn_mp_prime_random_ex.c"/>
<wrap name="bn_mp_get_int.c"/>
<wrap name="bn_mp_sqrt.c"/>
<wrap name="bn_mp_is_square.c"/>
<wrap name="bn_mp_init_set.c"/>
<wrap name="bn_mp_init_set_int.c"/>
<wrap name="bn_mp_invmod_slow.c"/>
<wrap name="bn_mp_prime_rabin_miller_trials.c"/>
<wrap name="bn_mp_to_signed_bin_n.c"/>
<wrap name="bn_mp_to_unsigned_bin_n.c"/>

<platform name="mac-cmake,android-cmake">
<debug>
	list(APPEND XSC_OPTIONS -t debug)
</debug>
<common>
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DARGTYPE=4 -DLTM_DESC -DLTC_NO_ASM -DLTC_NO_PK -DLTC_NO_PKCS -DLTC_NO_TEST")
</common>
</platform>

<platform name="Linux,MacOSX,Solaris,iPhone,android,threadx">
<debug>
XSC_OPTIONS += -t debug
</debug>
<common>
C_OPTIONS += -DARGTYPE=4 -DLTM_DESC -DLTC_NO_ASM -DLTC_NO_PK -DLTC_NO_PKCS -DLTC_NO_TEST
</common>
</platform>

<platform name="Windows">
<debug>
XSC_OPTIONS = $(XSC_OPTIONS) -t debug
</debug>
<common>
C_OPTIONS = $(C_OPTIONS) -DARGTYPE=4 -DLTM_DESC -DLTC_NO_ASM -DLTC_NO_PK -DLTC_NO_PKCS -DLTC_NO_TEST
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

<platform name="Linux,MacOSX,Solaris,Windows,android,threadx">
<common>
$(OBJECTS): sources/common.h
</common>
</platform>

</makefile>