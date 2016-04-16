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
#ifndef __FSKBLIT_H__
#define __FSKBLIT_H__

#ifndef __FSK__
# include "Fsk.h"
#endif /* __FSK__ */

#ifndef __FSKGRAPHICS_H__
# include "FskGraphics.h"
#endif /* __FSKGRAPHICS_H__ */

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#define CALL_COPYPROC(__proc, __height, __dstRowBump, __src, __dst, __width, __bpp, __state) \
	(*__proc)(__height, __dstRowBump, __src, __dst, __width, __bpp, __state)

typedef void (*FskBitmapCopyProc)(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state);

#ifdef __FSKBITMAP_PRIV__
	typedef struct {
		unsigned char		*y;
		unsigned char		*u;
		unsigned char		*v;
		UInt32				width;
	} YUV420BlitRecord, *YUV420Blit;

	FskAPI(YUV420Blit) makeYUV420BlitInfo(FskBitmap bits);

	void FskBlitInitialize(void);
	void FskBlitTerminate(void);
#endif

#endif
