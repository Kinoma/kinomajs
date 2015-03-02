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
#include "ippvc.h"
#include "kinoma_ipp_lib.h"

IppStatus (__STDCALL *ippiHuffmanRunLevelTableInitAlloc_32s_universal)					(const Ipp32s*    pSrcTable,     IppVCHuffmanSpec_32s** ppDstSpec)=NULL;
IppStatus (__STDCALL *ippiHuffmanTableFree_32s_universal)								(IppVCHuffmanSpec_32s *pDecodeTable)=NULL;
IppStatus (__STDCALL *ippiHuffmanTableInitAlloc_32s_universal)							(const Ipp32s*    pSrcTable,       IppVCHuffmanSpec_32s** ppDstSpec)=NULL;

#ifdef __KINOMA_IPP__

#ifdef __INTEL_IPP__
IppStatus __STDCALL ippiHuffmanRunLevelTableInitAlloc_32s_c(
	const Ipp32s*                pSrcTable,
        IppVCHuffmanSpec_32s** ppDstSpec)
{

	// EMPTY!!!!!!! --- WWD
	return ippStsNoErr;
}

IppStatus __STDCALL ippiHuffmanTableFree_32s_c(IppVCHuffmanSpec_32s *pDecodeTable)
{
	// EMPTY!!!!!!! --- WWD
	return ippStsNoErr;
}

IppStatus __STDCALL ippiHuffmanTableInitAlloc_32s_c(
     const Ipp32s*                pSrcTable,
        IppVCHuffmanSpec_32s** ppDstSpec)
{
	// EMPTY!!!!!!! --- WWD
	return ippStsNoErr;
}
#endif

#endif
