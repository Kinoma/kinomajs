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

#ifndef __KPPARSEQTVROBJECT__
#define __KPPARSEQTVROBJECT__

#ifndef __QTREADER__
# include "QTReader.h"
#endif /* __QTREADER__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* Forward declarations */
struct KPObjectController;

#ifndef __KPPARSEQTVRCUBICPANO__
typedef QTErr (*KPProgressProc)(QTMovie movie, UInt16 percent);
#endif /* __KPPARSEQTVRCUBICPANO__ */


/********************************************************************************
 * KPNewInteractiveObjectMovieFromFile
 ********************************************************************************/

long
KPNewInteractiveObjectMovieFromFile(
	QTMovie					movie,				/* If NULL, it is opened and closed herein */
	void					*fRef,				/* File reference */
	QTMovieReadProc			readProc,			/* Read proc */
	QTMovieAllocProc		allocProc,			/* Alloc proc */
	QTMovieFreeProc			freeProc,			/* Free proc */
	KPProgressProc			progressProc,		/* Progress proc */
	
	SInt32					nodeIndex,			/* The desired node */
	
	UInt32					*numCols,			/* The number of columns */
	UInt32					*numRows,			/* The number of rows */
	QTTrack					*videoTrack,		/* The track that holds the video */
	UInt32					*startTime,			/* The start time of the video track samples */
	UInt32					*sampleDuration,	/* The duration of each frame */
	
	struct KPObjectController	*ctlr		/* If not NULL, sets the port width & height, pan, tilt, & fov */
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPPARSEQTVROBJECT__ */


