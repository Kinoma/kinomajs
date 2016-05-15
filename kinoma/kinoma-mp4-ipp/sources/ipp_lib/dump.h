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
#ifndef DUMP_H_CSQ_20060506
#define DUMP_H_CSQ_20060506

/*------------------------------------------------------------
*	dump the calling funtion's name and record some info
*/
#ifdef __cplusplus
extern "C" {
#endif
typedef struct  
{
	char szInfoList[100][256];
	int value[100][20];
	int count;
}FINFO;
FINFO func_info;

typedef struct
{
	int frame_count;
	int valid_frame_count;
	int width;
	int height;
} DEC_INFO;

/* debugging functions */

void print(const unsigned char *pIn, int width, int height, int step);
void print_s(const short *pIn, int width, int height, int step);
void print_d(const double *pIn, int width, int height, int step);

int compare(const unsigned char *pIn1, int step1, const unsigned char *pIn2, int step2, int width,int height, int diff);

void set_func_info();
void dump(char *szInfo,int value);
void print_ref_info(char *src_sln_path,DEC_INFO dec_info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
