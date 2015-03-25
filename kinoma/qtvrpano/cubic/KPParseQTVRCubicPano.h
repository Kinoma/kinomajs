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

#ifndef __KPPARSEQTVRCUBICPANO__
#define __KPPARSEQTVRCUBICPANO__

#ifndef __QTREADER__
# include "QTReader.h"
#endif /* __QTREADER__ */

#ifndef __KPCUBICGL__
# include "KPQTVRFormat.h"
#endif /* __KPCUBICGL__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* Forward declarations */
struct KPImage3D;
struct KPPanoController;

#ifndef __KPPARSEQTVROBJECT__
typedef QTErr (*KPProgressProc)(QTMovie movie, UInt16 percent);
#endif /* __KPPARSEQTVROBJECT__ */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Cubic Panoramas								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * KPQTVRCubicFaceDataToMatrix
 *		Compute the immersion matrix from the QTVR cubic face data.
 ********************************************************************************/

void	KPQTVRCubicFaceDataToMatrix(
			const struct QTVRCubicFaceData	*cufa,		/* The 'cufa' atom associated with this face */
			long							width,		/* The width of the face */
			long							height,		/* The height of the face */
			float							M[3][3]		/* The resultant matrix */
		);


/********************************************************************************
 * KPNewCubicPanoramaFromFile
 *	returns:
 *		0  if all went well
 *		-1 if there was insufficient memory
 *		-2 if there were QuickTime parsing problems
 ********************************************************************************/

long	KPNewCubicPanoramaFromFile(
			QTMovie					movie,
			void					*fRef,			/* File reference */
			QTMovieReadProc			readProc,		/* Read proc */
			QTMovieAllocProc		allocProc,		/* Alloc proc */
			QTMovieFreeProc			freeProc,		/* Free proc */
			KPProgressProc			progressProc,	/* Progress proc */
			
			long					nodeID,			/* The desired node */
			
			long					*numFaces,		/* The number of faces */
			struct KPImage3D		**faces,		/* The faces - allocated here */
			
			struct KPPanoController	*ctlr			/* If not NULL, sets the port width & height, pan, tilt, & fov */
		);


/********************************************************************************
 * KPDeleteCubicPanorama
 ********************************************************************************/

void	KPDeleteCubicPanorama(struct KPImage3D *faces);


/********************************************************************************
 * KPUpdateBaseAddrOfCubicPanorama
 ********************************************************************************/

void	KPUpdateBaseAddrOfCubicPanorama(struct KPImage3D *faces);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPPARSEQTVRCUBICPANO__ */


