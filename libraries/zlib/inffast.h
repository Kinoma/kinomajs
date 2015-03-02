/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

void ZLIB_INTERNAL inflate_fast OF((z_streamp strm, unsigned start));

#ifdef KINOMA_ZLIB_OPT
extern void inflate_fast_copy_neon(unsigned len, unsigned char **out, unsigned char *from);
void ZLIB_INTERNAL inffast_copy_c(unsigned len, unsigned char **out, unsigned char *from);
void ZLIB_INTERNAL inffast_copy_c_low_dist(unsigned len, unsigned char **out, unsigned char *from);
#endif
