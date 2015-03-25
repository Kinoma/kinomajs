/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#undef FskBilerp32ARGB
#define FskBilerp32ARGB						FskBilerp32Inline
#undef FskBilerp32BGRA
#define FskBilerp32BGRA						FskBilerp32Inline
#undef FskBilerp32ABGR
#define FskBilerp32ABGR						FskBilerp32Inline
#undef FskBilerp32RGBA
#define FskBilerp32RGBA						FskBilerp32Inline
#undef FskAlphaA32
#define FskAlphaA32							FskAlphaA32Inline
#undef FskAlpha32A
#define FskAlpha32A							FskAlpha32AInline
#undef FskAlphaBlend32ARGB
#define FskAlphaBlend32ARGB					FskAlphaBlendA32Inline
#undef FskAlphaBlend32RGBA
#define FskAlphaBlend32RGBA					FskAlphaBlend32AInline
#undef FskBlend32ARGB
#define FskBlend32ARGB						FskBlend32Inline
#undef FskBlend32ABGR
#define FskBlend32ABGR						FskBlend32Inline
#undef FskBlend32BGRA
#define FskBlend32BGRA						FskBlend32Inline
#undef FskBlend32RGBA
#define FskBlend32RGBA						FskBlend32Inline

#define AlphaNScale32(c, a, b)					(c *= a, c += (1 << (b - 1)), c += c >> b, c >>= b)
#define AlphaScale32(c, a)						AlphaNScale32(c, a, 8)
#define AlphaLerp32(c0, c1, a)					(c1 -= c0, c1 *= a, c1 -= c0, c0 <<= 8, c0 += c1, c0 += 128, c0 += c0 >> 8, c0 >>= 8)
#define AlphaLerpComp32(p0, p1, pc, a, n, m)	(c0 = (p0 >> n) & m, c1 = (p1 >> n) & m, AlphaLerp32(c0, c1, a), pc |= (SInt32)c0 << n)
#define AlphaLerpAlphaComp32(p0, pc, a, n, m)	(c0 = (p0 >> n) & m, pc |= ((AlphaScale32(c0, (m - a)) + a) << n))

#define GetField(p, n, b)					(((p) >> (n)) & ((1 << (b)) - 1))
#define AlphaLerpCField(b, f, r, a, n, cz, az) do { SInt32 p, q; p = GetField(f, n, cz); q = GetField(b, n, cz); p = (p-q)*(SInt32)(a) + (q<<(az))-q+(1<<((az)-1)); p += p>>(az); r &= ~(((1 << (cz)) - 1) << (n)); r |= ((p>>(az))<<(n)); } while(0)
#define AlphaLerpAField(b,    r, a, n, cz, az) do { SInt32 q = GetField(b, n, cz);       q = q * (SInt32)(((1<<(az))-1) - (a))  + ((a)<<(cz)) - (a) +(1<<((az)-1)); q += q>>(az); r &= ~(((1 << (cz)) - 1) << (n)); r |= ((q>>(az))<<(n)); } while(0)

static inline UInt32
FskBilerp32Inline(UInt32 di, UInt32 dj, const UInt32 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11, p;
	UInt32 di0, dj0, mask;

	p00 = *s;
	p01 = sizeof(UInt32);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt32*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt32*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt32*)(((const char*)(s)) + (SInt32)p11));

	di0 = (1 << 4)  - di;
	dj0 = (1 << 4) - dj;

	mask = 0x00FF00FF;
	p   = ((((((p00 >> 0) & mask) * di0) + (((p01 >> 0) & mask) * di)) * dj0
		+   ((((p10 >> 0) & mask) * di0) + (((p11 >> 0) & mask) * di)) * dj
		+   0x00800080
		) >> 8) & mask;
	p  |= ((((((p00 >> 8) & mask) * di0) + (((p01 >> 8) & mask) * di)) * dj0
		+   ((((p10 >> 8) & mask) * di0) + (((p11 >> 8) & mask) * di)) * dj
		+   0x00800080
		) >> 0) & (~mask);

	return(p);
}

static inline void
FskAlphaA32Inline(UInt32 *d, UInt32 p)
{
	SInt32 alpha = p >> 24;

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32	c0, c1;
			SInt32	p0, p1, pc;
			SInt32	mask		= 0xFF;

			p0 = *d;
			p1 = p;
			pc = 0;
			AlphaLerpComp32( p0, p1, pc, alpha,  0, mask);
			AlphaLerpComp32( p0, p1, pc, alpha,  8, mask);
			AlphaLerpComp32( p0, p1, pc, alpha, 16, mask);
			AlphaLerpAlphaComp32(p0, pc, alpha, 24, mask);
			*d = pc;
		}
	}
}

static inline void
FskAlpha32AInline(UInt32 *d, UInt32 p)
{
	SInt32 alpha = p & 0xFF;

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32 p0 = *d;
			SInt32 p1 = p;
			SInt32 pc = 0;
			AlphaLerpAField(p0,     pc, alpha,  0, 8, 8);
			AlphaLerpCField(p0, p1, pc, alpha,  8, 8, 8);
			AlphaLerpCField(p0, p1, pc, alpha, 16, 8, 8);
			AlphaLerpCField(p0, p1, pc, alpha, 24, 8, 8);
			*d = pc;
		}
	}
}

static inline void
FskAlphaBlendA32Inline(UInt32 *d, UInt32 p, UInt8 opacity)
{
	SInt32 a, b;

	a = p >> 24;
	b = opacity;
	AlphaScale32(a, b);	/* Scale alpha by opacity */
	p &= 0x00FFFFFF;	/* Clear old alpha */
	p |= a << 24;		/* Insert scaled alpha */
	FskAlphaA32Inline(d, p);
}

static inline void
FskAlphaBlend32AInline(UInt32 *d, UInt32 p, UInt8 opacity)
{
	SInt32 a, b;

	a = p & 0xFF;
	b = opacity;
	AlphaScale32(a, b);	/* Scale alpha by opacity */
	p &= 0xFFFFFF00;	/* Clear old alpha */
	p |= a;				/* Insert scaled alpha */
	FskAlpha32AInline(d, p);
}

static inline void
FskBlend32Inline(UInt32 *d, UInt32 p, UInt8 alpha)
{
#if 0
	SInt32	c0, c1;
	SInt32	p0, p1, pc;
	SInt32	sAlpha		= alpha;
	SInt32	mask		= 0xFF;

	p0 = *d;
	p1 = p;
	pc = 0;
	AlphaLerpComp(p0, p1, pc, sAlpha,  0, mask);
	AlphaLerpComp(p0, p1, pc, sAlpha,  8, mask);
	AlphaLerpComp(p0, p1, pc, sAlpha, 16, mask);
	AlphaLerpComp(p0, p1, pc, sAlpha, 24, mask);
	*d = pc;
#elif 0
	const UInt32 mask = 0x00FF00FF;
	UInt32 p0 = *d;
	UInt32 dstrb =  p0       & mask;
	UInt32 dstag = (p0 >> 8) & mask;

	UInt32 srcrb =  p       & mask;
	UInt32 srcag = (p >> 8) & mask;

	UInt32 drb = (srcrb - dstrb) * alpha;
 	UInt32 dag = (srcag - dstag) * alpha;

	p0  =  ((drb >> 8) + dstrb) & mask;
	p   = (((dag >> 8) + dstag) & mask) << 8;

	*d = p0 | p;
#else
	const UInt32 mask = 0x00FF00FF;
	UInt32 p0 = *d;
	UInt32 dstrb =  p0       & mask;
	UInt32 dstag = (p0 >> 8) & mask;

	UInt32 srcrb =  p       & mask;
	UInt32 srcag = (p >> 8) & mask;

	UInt32 drb = (srcrb - dstrb) * alpha + (dstrb << 8) - dstrb + 0x00800080;
 	UInt32 dag = (srcag - dstag) * alpha + (dstag << 8) - dstag + 0x00800080;

	p0 = ((drb + ((drb >> 8) & mask)) >> 8) & mask;
	p  = ((dag + ((dag >> 8) & mask)) >> 8) & mask;

	*d = p0 | (p << 8);
#endif
}
