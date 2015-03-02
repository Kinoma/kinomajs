/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////
// Macro difinition
/////////////////////////////////////////////////////////////////////////////////
#ifndef MISC_H
#define MISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "codecDef.h"

#ifndef NULL
#define NULL 0
#endif

#define IPP_OK			0
#define IPP_FAIL		-1

int CodecTest(int argc,char** argv);

/////////////////////////////////////////////////////////////////////////////////
// Part 1 Memory operation
// IPP sample code should use these APIs for memory operation with memory test.
/////////////////////////////////////////////////////////////////////////////////

/* Initialize for the memory test */
void IPP_InitMemCheck();

/* Deinitialize for the memory test */
int IPP_DeinitMemCheck();
/*do physical continuous memory test   */
int IPP_PysicalMemTest();

/* Aligned malloc
// return IPP_OK if success
// return IPP_FAIL if failure
*/
int IPP_MemMalloc(void **ppDstBuf, int size, unsigned char align);

/* Aligned calloc
// return IPP_OK if success
// return IPP_FAIL if failure
*/
int IPP_MemCalloc(void **ppDstBuf, int size, unsigned char align);

/* Memory reallocation without alignment
// If newsize > oldsize, will realloc
// If newsize < oldsize, no need to realloc
// return IPP_OK if success
// return IPP_FAIL if failure
*/
int IPP_MemRealloc(void **ppStream, int oldsize, int newsize);

/* Free for IPP_MemMalloc, IPP_MemCalloc and IPP_MemRealloc
// return IPP_OK if success
// return IPP_FAIL if failure
*/
int IPP_MemFree(void ** ppSrcBuf);

/////////////////////////////////////////////////////////////////////////////////
// Part 2 Performance test
// IPP sample code should use following APIs for performance test.
/////////////////////////////////////////////////////////////////////////////////

#define MAX_PERFORMANCE_INDEX 8
typedef long long (*IPP_COUNTER_FUNC)();
#define DEFAULT_TIMINGFUNC_START        IPP_TimeGetTickCount
#define DEFAULT_TIMINGFUNC_STOP         IPP_TimeGetTickCount
/* Initialize for performance counter */
void IPP_InitPerfCounter();

/* Get an index for performance counting,return -1 means fail */
//void IPP_GetPerfCounter(int* index);
void IPP_GetPerfCounter(int* index, IPP_COUNTER_FUNC pStart, IPP_COUNTER_FUNC pStop);

/* Release an index for performance counting */
void IPP_FreePerfCounter(int index);

/* Reset the specified performance counter to 0 */
void IPP_ResetPerfCounter(int index);

/* Start performance counting for specified counter */
void IPP_StartPerfCounter(int index);

/* Stop performance counting for specified counter */
void IPP_StopPerfCounter(int index);

/* Get the performance value from specified counter */
long long IPP_GetPerfData(int index);

/* DeInitialize for performance counter */
void IPP_DeinitPerfCounter();

/////////////////////////////////////////////////////////////////////////////////
// Part 3 Logging
// IPP sample code should use IPP_Log to log message
/////////////////////////////////////////////////////////////////////////////////

/* Logging function
// If set logfile to NULL will log message to a test system default log stream
// If set logfile to other names, they will treat as output files
*/
void IPP_Log(char* logfile, char* mode, char* message, ...);

/////////////////////////////////////////////////////////////////////////////////
// Part 4 File operation
// IPP sample code should use these APIs for file operation
/////////////////////////////////////////////////////////////////////////////////

typedef void IPP_FILE;

/* Macro for file IPP_Fseek */
#define IPP_SEEK_CUR    1
#define IPP_SEEK_END    2
#define IPP_SEEK_SET    0

/* Open a file */
IPP_FILE* IPP_Fopen(const char* filename, const char* mode);

/* Close a file */
int IPP_Fclose(IPP_FILE* file);

/* Read data from a file */
int IPP_Fread(void * buffer, int size, int count, IPP_FILE* file);

/* Write data to a file */
int IPP_Fwrite(const void * buffer, int size, int count, IPP_FILE* file);

/* Tests for end-of-file on a file */
int IPP_Feof(IPP_FILE* file);

/* Moves the file pointer to a specified location */
int IPP_Fseek(IPP_FILE* file, long offset, int origin);

/* Gets the current position of a file pointer */
long IPP_Ftell(IPP_FILE* file);

/*Get a string from a file pointer */
char *IPP_Fgets(char *str, int n, IPP_FILE* file);

/*Get a char from a file pointer */
int IPP_Fgetc(IPP_FILE* file);

/*Push a char back to the file */
int IPP_Fungetc(char c, IPP_FILE* file);

/////////////////////////////////////////////////////////////////////////////////
// Part 5 extension for more performanc test
/////////////////////////////////////////////////////////////////////////////////
void IPP_Fprintf(IPP_FILE* file, const char *format, ...);
void IPP_Fscanf(IPP_FILE* file, const char *format, ...);

void IPP_Printf(const char* format, ...);

int IPP_Strcmp(const char *s1, const char *s2);

char *IPP_Strcpy(char *dst, const char *src);

char *IPP_Strstr(char* s1, char* s2);

int IPP_Strlen(char* s);

char *IPP_Strncpy(char *dst, const char *src, int n);

int IPP_Strncmp(char * s1, char * s2, int n );

int IPP_Atoi(const char *s);

float IPP_Atof(const char *s);

float IPP_Log10(float x);

/* Thread */
int IPP_ThreadCreate(void *phThread, int nPriority, void *pFunc, void *pParam);

int IPP_ThreadDestroy(void *phThread, int bWait);

/* Mutex */
int IPP_MutexCreate( void* *phMutex); 

int IPP_MutexDestroy( void* hMutex); 

int IPP_MutexLock( void* hMutex,  unsigned int mSec,  int *pbTimedOut);

int IPP_MutexUnlock( void* hMutex); 

/* Event */
#define INFINITE_WAIT 0xFFFFFFFF

int IPP_EventCreate( void* *phEvent); 

int IPP_EventDestroy( void* hEvent);

int IPP_EventReset( void* hEvent);

int IPP_EventSet( void* hEvent);

/* Memory and String */
void *IPP_Memset(void *buffer, int c, int count); 

void *IPP_Memmove(void *dst, const void *src, int n);

int IPP_Memcmp(void* src1, void* src2, int len);

void* IPP_Memcpy(void* dst, void* src, int len);

char *IPP_Strcat(char *dst,const char *src);

long long IPP_TimeGetTickCount();

long long IPP_TimeGetThreadTime();

/*misc callback table*/
int miscgStreamFlush(void *pStream, void *pStreamHandle, int nAvailBytes, int extMode);
int miscInitGeneralCallbackTable(MiscGeneralCallbackTable **ppDstCBTable);
int miscFreeGeneralCallbackTable(MiscGeneralCallbackTable **ppSrcCBTable);

#define MAX_MESG_LEN	256
void IppStatusMessage(IppCodecStatus status, char* message);

/*global variable which are used to counter decoded frame number and decoding time.*/
#define IPP_AUDIO_INDEX 0
#define IPP_VIDEO_INDEX 1

extern int g_Frame_Num[2];
extern long long g_Tot_Time[2];
extern long long g_Tot_DeviceTime[2];
extern long long g_Tot_CPUTime[2];

typedef struct _DISPLAY_CB {
	int	bDisplayEnable;
        int     screen_width;
        int     screen_height;
        int     x;
        int     y;
        int     nPlayTickPerFrame;     /* play frame rate control, 0 for full speed */
} DISPLAY_CB;

//display
IppCodecStatus display_open(DISPLAY_CB* hDispCB, int imageWidth, int imageHeight);
void display_frame(DISPLAY_CB* hDispCB, const IppPicture * pappPic);
void display_JPEG(DISPLAY_CB* hDispCB, const IppPicture * pappPic);
void display_close ();

//waveout
IppCodecStatus audio_open(int sampleRate, int numChannels);
void audio_render(short *pcm, int len, int channels);
void audio_close(void);

#ifdef __cplusplus
}
#endif

#endif
