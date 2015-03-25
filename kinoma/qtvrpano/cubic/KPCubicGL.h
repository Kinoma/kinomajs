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

#ifndef __KPCUBICGL__
#define __KPCUBICGL__

#ifndef __KPQTVRCONTROLLER__
# include "KPCubicPanoController.h"
#endif /* __KPQTVRCONTROLLER__ */

#ifndef __KPPROJECTIMAGE__
# include "KPProjectImage.h"
#endif /* __KPPROJECTIMAGE__ */

#ifndef __KPQTVRFORMAT__
# include "KPQTVRFormat.h"
#endif /* __KPQTVRFORMAT__ */

#ifndef __gl_h_
# include <gl.h>
#endif /* __gl_h_ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * Create an Open GL view matrix from the given controller matrix
 ********************************************************************************/

void	KPControllerViewMatrixToGLMatrix(const float V[3][3], float GL[4][4]);


/********************************************************************************
 * Set the GL Projection and Model matrices from a KPPanoController 
 ********************************************************************************/

void	KPSetGLCameraFromCubicPanoController(const KPPanoController *ctlr);


/********************************************************************************
 * Determine the 3D coordinates for each tile from a KPImage3D array.
 ********************************************************************************/

void	KPSetGLGeometryFromImage3D(
			long			numSrcs,	/* The number of quadrilateral tiles */
			const KPImage3D	*srcs,		/* The quadrilater tiles in 3 space */
			GLfloat			*xyz		/* pre-allocated numSrcs * 4 * 3 floats */
		);


/********************************************************************************
 * Assemble textures into a standard cube. Texture is glDim wide and 6*glDim high
 ********************************************************************************/

long	KPConvertImages3DToCanonicalGLTextures(
			long			numSrcs,
			const KPImage3D	*srcs,
			long			*glDim,
			unsigned char	**glRGB
		);




/********************************************************************************
 * KPCubicFaceDataToCaconicalGLTexture
 *	Usage:
 *		long			glDim	= 0;
 *		unsigned char	*glRGB	= NULL;
 *		for (cufa = FirstCufa(), tile = FirstTile(); cufa && tile; cufa = NextCufa(cufa), tile = NextTile(tile))
 *			KPCubicFaceDataToGLTexture(cufa, tile, tileRowBytes, tileWidth, tileHeight, &glDim, &glRGB);
 *		...
 *		KPDisposeCanonicalGLTextures(glRGB);
 * On the first call, storage is allocated for six faces stored contiguously in 24 bit RGB format.
 ********************************************************************************/

long	KPCubicFaceDataToGLTexture(
			const QTVRCubicFaceData	*cufa,				/* Either the actual address of a cufa, or the integers {0,1,2,3,4,5} indicating which face */
			const unsigned char		*tile,
			long					tileRowBytes,
			long					tilePixelFormat,
			long					tileWidth,
			long					tileHeight,
			long					*glDim,
			unsigned char			**glRGB
		);



/********************************************************************************
 * KKPDisposeCanonicalGLTextures
 *		Dispose of texture memory.
 ********************************************************************************/

void	KPDisposeCanonicalGLTextures(unsigned char *rgb);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPCUBICGL__ */

