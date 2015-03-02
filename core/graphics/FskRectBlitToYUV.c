/*
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
*/
/* Only generate the procs if we're going to use them and this file is appropriate */
#if		DST_YUV420																	\
	&&	FskName2(SRC_,FskName3(fsk,SrcPixelKind,KindFormat))						\
	&&	(FskName3(fsk,SrcPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)	\
	&&	COMPILER_CAN_GENERATE_CONST_PROC_POINTERS

	static const FskRectTransferProc FskName3(FskCopy,SrcPixelKind,YUV420)					= NULL;
	static const FskRectTransferProc FskName3(FskBilinearCopy,SrcPixelKind,YUV420)			= NULL;
	static const FskRectTransferProc FskName3(FskBlend,SrcPixelKind,YUV420)					= NULL;
	static const FskRectTransferProc FskName3(FskBilinearBlend,SrcPixelKind,YUV420)			= NULL;
	static const FskRectTransferProc FskName3(FskAlpha,SrcPixelKind,YUV420)					= NULL;
	static const FskRectTransferProc FskName3(FskBilinearAlpha,SrcPixelKind,YUV420)			= NULL;
	static const FskRectTransferProc FskName3(FskAlphaBlend,SrcPixelKind,YUV420)			= NULL;
	static const FskRectTransferProc FskName3(FskBilinearAlphaBlend,SrcPixelKind,YUV420)	= NULL;
	static const FskRectTransferProc FskName3(FskTintCopy,SrcPixelKind,YUV420)				= NULL;
	static const FskRectTransferProc FskName3(FskBilinearTintCopy,SrcPixelKind,YUV420)		= NULL;

#else /* It will be necessary to #define XXXXXX NULL in the calling program */
#endif /* COMPILER_CAN_GENERATE_CONST_PROC_POINTERS */

#undef SrcPixelKind
