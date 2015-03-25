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
#ifndef __FSKDHCLIPPOLYGON__
#define __FSKDHCLIPPOLYGON__


#ifndef __FSKMATRIX__
	#include "FskMatrix.h"
#endif /* __FSKMATRIX__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





#define FSKDHCLIP_PLANES	4			/* The number of clipping planes */
#define FSKDHCLIP_ALLIN	0
#define FSKDHCLIP_ALLOUT	((1<<FSKDHCLIP_PLANES)-1)	/* 4 bits on */

/* Determine vertex location in 3D space */
#define FSKDHCLIP_LEFT	0
#define FSKDHCLIP_RIGHT	1
#define FSKDHCLIP_TOP	2
#define FSKDHCLIP_BOTTOM	3
#define FSKDHCLIP_OUTLFT	(1<<FSKDHCLIP_LEFT)
#define FSKDHCLIP_OUTRGT	(1<<FSKDHCLIP_RIGHT)
#define FSKDHCLIP_OUTTOP	(1<<FSKDHCLIP_TOP)
#define FSKDHCLIP_OUTBOT	(1<<FSKDHCLIP_BOTTOM)


/* Clipping rectangle */
typedef struct	FskRect	{	int xMin, xMax, yMin, yMax;	}	FskRect;


/* This generates the trivial clip codes inCode and outCode */
void	FskDHTrivialClipPolygon(int nPts, FskDVector3D *p, int *inCode, int *outCode);

void	FskDHClipPolygon(int *nPts, FskDVector3D *pts, FskDVector3D *temp, const FskRect *r);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKDHCLIPPOLYGON__ */

