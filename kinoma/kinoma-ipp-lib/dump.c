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
#include "dump.h"
#include <stdio.h>
#include <string.h>
//#include <math.h>

/* debugging functions*/
void set_func_info()
{
	int i,j;
	func_info.count = 0;
	for (j = 0; j < 100; j++)
	{
		for (i = 0; i < 20; i++)
		{
			func_info.value[j][i] = -99;
		}
	}
}

void dump(char *szInfo, int value)
{
	int i,j;
	int bFlag1 = 0;
	int bFlag2 = 0;
	static int count1 = 0;
	static int count2[100] ={ 0};
	char szTmp[256];

	sprintf(szTmp, "%s", szInfo);
	for (i = 0; i < 100; i++)
	{
		if (strcmp(func_info.szInfoList[i], szTmp) == 0)
		{
			bFlag1 = 1;
			j = i;
			break;
		}
	}
	if (bFlag1 == 0)
	{
		strcpy(func_info.szInfoList[count1],szTmp);
		func_info.value[count1][0] = value;
		printf("%s : %d\n",func_info.szInfoList[count1],func_info.value[count1][0]);
		count1++;
		count2[count1] = 1;
		func_info.count = count1;
	}
	else
	{
		for (i = 0; i < 20; i++)
		{
			if (value == func_info.value[j][i])
			{
				bFlag2 = 1;
				break;
			}
		}
		if (bFlag2 == 0)
		{
			func_info.value[j][count2[j]] = value;
			count2[j]++;
		}

	}
}

void print_ref_info(char *src_sln_path,DEC_INFO dec_info)
{
	int i,j;
	FILE * fRef;
	char szRefFile[256];

	sprintf(szRefFile,"%s.ref",src_sln_path);
	fRef = fopen(szRefFile,"w+");
	if (fRef == NULL)
		return;
	for (i = 0; i < func_info.count; i++)
	{
		fprintf(fRef,"%s	", func_info.szInfoList[i]);
		for (j = 0; j < 20; j++)
		{
			if (func_info.value[i][j] > -1)
				fprintf(fRef,"%d, ", func_info.value[i][j]);

		}
		fprintf(fRef,"\n");

	}

	fprintf(fRef,"\nframe_count:%d\n",dec_info.frame_count);

	fclose(fRef);

}

#ifdef FUNCS_TIME
// following for testing every function's performance
void TimeZero()
{
	int i;
	for (i = 0; i < FUNC_NUM; i++)
	{
		t_dur[i] = 0;
		calls[i] = 0;
	}

	strcpy(func_name[0], "ippsZero_8u_c");
	strcpy(func_name[1],"ippsSet_8u_c");
	strcpy(func_name[2],"ippiCopy_8u_C1R_c");
	strcpy(func_name[3],"ippiSet_8u_C1R_c");
	strcpy(func_name[4],"ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c");
	strcpy(func_name[5],"ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c");
	strcpy(func_name[6],"ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c");
	strcpy(func_name[7],"ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c");
	strcpy(func_name[8],"ippiInterpolateBlock_H264_8u_P2P1R_c");
	strcpy(func_name[9],"ippiInterpolateLuma_H264_8u_C1R_c");
	strcpy(func_name[10],"ippiInterpolateChroma_H264_8u_C1R_c");
	strcpy(func_name[11],"ippiDecodeExpGolombOne_H264_1u16s_c");
	strcpy(func_name[12],"ippiDecodeCAVLCCoeffs_H264_1u16s_c");
	strcpy(func_name[13],"ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c");
	strcpy(func_name[14],"ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c");
	strcpy(func_name[15],"ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c");
	strcpy(func_name[16],"ippiInterpolateLumaTop_H264_8u_C1R_c");
	strcpy(func_name[17],"ippiInterpolateLumaBottom_H264_8u_C1R_c");
	strcpy(func_name[18],"ippiInterpolateChromaTop_H264_8u_C1R_c");
	strcpy(func_name[19],"ippiInterpolateChromaBottom_H264_8u_C1R_c");
	strcpy(func_name[20],"ippiInterpolateBlock_H264_8u_P3P1R_c");
	strcpy(func_name[21],"ippiUniDirWeightBlock_H264_8u_C1R_c");
	strcpy(func_name[22],"ippiBiDirWeightBlock_H264_8u_P2P1R_c");
	strcpy(func_name[23],"ippiBiDirWeightBlock_H264_8u_P3P1R_c");
	strcpy(func_name[24],"ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c");
	strcpy(func_name[25],"ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c");
	strcpy(func_name[26],"ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c");
	strcpy(func_name[27],"ippiReconstructChromaIntraMB_H264_16s8u_P2R_c");
	strcpy(func_name[28],"ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c");
	strcpy(func_name[29],"ippiReconstructLumaIntraMB_H264_16s8u_C1R_c");
	strcpy(func_name[30],"ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c");
	strcpy(func_name[31],"ippiReconstructLumaInterMB_H264_16s8u_C1R_c");
	strcpy(func_name[32],"ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c");
	strcpy(func_name[33],"ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c");
	strcpy(func_name[34],"ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c");
	strcpy(func_name[35],"ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c");
	strcpy(func_name[36],"ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c");
	strcpy(func_name[37],"ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c");
	strcpy(func_name[38],"ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c");
	strcpy(func_name[39],"ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c");
	strcpy(func_name[40],"ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c");
	strcpy(func_name[41],"ippiReconstructChromaInterMB_H264_16s8u_P2R_c");
	strcpy(func_name[42],"ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c");
}

void TimeStart(int func)
{
	t_time = vm_time_get_tick();
}
void TimeEnd(int func)
{
	t_dur[func] += (vm_time_get_tick() - t_time);
	calls[func]++;
}

void TimePrint(char *filename)
{
	int i;
	FILE *ft;
	t_time = vm_time_get_frequency();
	ft = fopen(filename, "a+");

	for (i = 0; i < FUNC_NUM; i++)
	{
		if( calls[i] != 0 )
		//fprintf(ft, "Function:%55s,    calls:%9d,    Time: [%10.2f]ms\n", func_name[i], calls[i],(float)t_dur[i]/t_time*1.0e3f);
		  fprintf(stderr, "Function:%s,    calls:%d,    Time: [%10.2f]ms\n", func_name[i], calls[i],(float)t_dur[i]/t_time*1.0e3f);
	}
	fclose(ft);
}
#endif
