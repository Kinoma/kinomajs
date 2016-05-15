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
#include <math.h>

/* debugging functions*/
void print(const unsigned char *pIn, int width, int height, int step)
{
	int i,j,len;
	for (i = 0; i < height; i++)
	{
		len = i*step;
		for (j = 0; j < width; j++)
		{
			printf("%3d, ",pIn[len+j]);
		}
		printf("\n");
	}
	printf("\n");
}

void print_s(const short *pIn, int width, int height, int step)
{
	int i,j,len;
	for (i = 0; i < height; i++)
	{
		len = i*step;
		for (j = 0; j < width; j++)
		{
			printf("%3d, ",pIn[len+j]);
		}
		printf("\n");
	}
	printf("\n");
}

void print_d(const double *pIn, int width, int height, int step)
{
	int i,j,len;
	for (i = 0; i < height; i++)
	{
		len = i*step;
		for (j = 0; j < width; j++)
		{
			printf("%3.2f, ",pIn[len+j]);
		}
		printf("\n");
	}
	printf("\n");
}

int compare(const unsigned char *pIn1, int step1, const unsigned char *pIn2, int step2, int width, int height, int diff)
{
	int i,j;
	unsigned char *p1, *p2;
	p1 = (unsigned char *)pIn1;
	p2 = (unsigned char *)pIn2;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			if (abs(p1[i] - p2[i]) > diff)
				return (p1[i]-p2[i]);
		}
		p1 += step1;
		p2 += step2;
	}
	return 0;
}

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
	fprintf(fRef,"valid_frame_count:%d\n",dec_info.frame_count);
	fprintf(fRef,"bitstream width: %d\n", dec_info.width);
	fprintf(fRef,"bitstream height: %d\n", dec_info.height);
	fclose(fRef);

	fclose(fRef);

}
